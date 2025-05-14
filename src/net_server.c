#include "../include/net_server.h"

// --- Internal Structures ---

/**
 * @brief Represents the connection status of a client from the server's perspective.
 */
typedef enum ServerClientStatus
{
    CLIENT_STATE_INACTIVE, /**< Slot is free. */
    CLIENT_STATE_ACCEPTED, /**< Connection accepted, waiting for C_HELLO. */
    CLIENT_STATE_WELCOMED  /**< C_HELLO received, S_WELCOME sent, client is fully active. */
} ServerClientStatus;

/**
 * @brief Holds information about a connected client on the server.
 */
typedef struct ServerClientInfo
{
    SDLNet_StreamSocket *socket; /**< The communication socket for this client. */
    ServerClientStatus status;   /**< The current status of this client connection. */
    uint8_t client_id;           /**< The unique ID assigned to this client. */
} ServerClientInfo;

/**
 * @brief Internal state for the NetServer module.
 */
struct NetServerState_s
{
    SDLNet_Server *listen_socket;          /**< The main server socket listening for new connections. */
    ServerClientInfo clients[MAX_CLIENTS]; /**< Array holding information for each potential client slot. */
    int connected_clients_count;           /**< Current number of clients in ACCEPTED or WELCOMED state. */
};

// --- Static Helper Functions ---

/**
 * @brief Finds the first inactive slot in the clients array.
 * @param ns_state The NetServerState instance.
 * @return The index of an inactive slot, or -1 if the server is full.
 */
static int find_inactive_client_slot(NetServerState ns_state)
{
    if (!ns_state)
        return -1;
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (ns_state->clients[i].status == CLIENT_STATE_INACTIVE)
        {
            return i;
        }
    }
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Max clients (%d) reached.", MAX_CLIENTS);
    return -1; // Server full
}

/**
 * @brief Sends a data buffer to a specific client.
 * @param client_info Pointer to the ServerClientInfo for the target client.
 * @param buffer Pointer to the data buffer to send.
 * @param length The number of bytes to send from the buffer.
 * @return True if the send was successful, false on failure.
 */
static bool send_to_client(ServerClientInfo *client_info, const void *buffer, int length)
{
    if (!client_info || client_info->status == CLIENT_STATE_INACTIVE || !client_info->socket)
    {
        return false;
    }
    if (!SDLNet_WriteToStreamSocket(client_info->socket, buffer, length))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Server] Send failed to client ID %u: %s.", (unsigned int)client_info->client_id, SDL_GetError());
        return false;
    }
    return true;
}

/**
 * @brief Internal implementation for broadcasting messages to all relevant clients.
 * Sends the provided buffer to all clients in the WELCOMED state, optionally excluding one.
 * Marks clients for disconnection if sending fails.
 * @param ns_state The NetServerState instance.
 * @param buffer Pointer to the data buffer to broadcast.
 * @param length Number of bytes to broadcast.
 * @param exclude_client_index Index of a client to skip sending to (-1 to send to all).
 */
static void internal_broadcast_message_impl(NetServerState ns_state, const void *buffer, int length, int exclude_client_index); // Forward declaration

/**
 * @brief Disconnects a specific client, cleans up their slot, and notifies others.
 * @param ns_state The NetServerState instance.
 * @param client_index The index of the client in the clients array to disconnect.
 */
static void disconnect_client(NetServerState ns_state, int client_index)
{
    if (!ns_state || client_index < 0 || client_index >= MAX_CLIENTS || ns_state->clients[client_index].status == CLIENT_STATE_INACTIVE)
    {
        return;
    }

    ServerClientInfo *client_info = &ns_state->clients[client_index];
    uint8_t disconnected_id = client_info->client_id;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Server] Disconnecting client ID %u at index %d.", (unsigned int)disconnected_id, client_index);

    ServerClientStatus old_status = client_info->status;
    client_info->status = CLIENT_STATE_INACTIVE;
    if (old_status != CLIENT_STATE_INACTIVE)
    {
        ns_state->connected_clients_count--;
    }

    if (client_info->socket)
    {
        SDLNet_DestroyStreamSocket(client_info->socket);
        client_info->socket = NULL;
    }

    // Only notify others if the client was fully connected (WELCOMED)
    if (old_status == CLIENT_STATE_WELCOMED)
    {
        Msg_PlayerDisconnectData disconnect_msg;
        disconnect_msg.message_type = MSG_TYPE_S_PLAYER_DISCONNECT;
        disconnect_msg.client_id = disconnected_id;
        internal_broadcast_message_impl(ns_state, &disconnect_msg, sizeof(disconnect_msg), client_index); // Use internal impl to avoid infinite loop
    }
}

static void internal_broadcast_message_impl(NetServerState ns_state, const void *buffer, int length, int exclude_client_index)
{
    if (!ns_state)
        return;
    bool disconnect_flags[MAX_CLIENTS] = {false}; // Track clients failing to receive

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (i == exclude_client_index || ns_state->clients[i].status != CLIENT_STATE_WELCOMED)
        {
            continue; // Skip excluded client or non-welcomed clients
        }
        if (!send_to_client(&ns_state->clients[i], buffer, length))
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Broadcast failed for client ID %u, marking for disconnect.", (unsigned int)ns_state->clients[i].client_id);
            disconnect_flags[i] = true;
        }
    }
    // Disconnect clients that failed the broadcast after attempting all sends
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (disconnect_flags[i])
        {
            // Double-check status before disconnecting, might have changed
            if (ns_state->clients[i].status != CLIENT_STATE_INACTIVE)
            {
                disconnect_client(ns_state, i);
            }
        }
    }
}

/**
 * @brief Processes a message received from a specific client based on its type.
 * Handles C_HELLO, C_PLAYER_STATE, and C_SPAWN_ATTACK messages.
 * @param ns_state The NetServerState instance.
 * @param client_index The index of the sending client.
 * @param buffer Pointer to the received data buffer.
 * @param bytesReceived Number of bytes in the buffer.
 * @param state Pointer to the main AppState.
 */
static void internal_process_client_message(NetServerState ns_state, int client_index, char *buffer, int bytesReceived, AppState *state)
{
    if (!ns_state || client_index < 0 || client_index >= MAX_CLIENTS || ns_state->clients[client_index].status == CLIENT_STATE_INACTIVE || bytesReceived < (int)sizeof(uint8_t) || !state)
    {
        return;
    }

    ServerClientInfo *client_info = &ns_state->clients[client_index];
    uint8_t msg_type_byte = (uint8_t)buffer[0];
    uint8_t sender_id = client_info->client_id;

    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[Server] Processing msg type %u from client %u (status: %d)", (unsigned int)msg_type_byte, (unsigned int)sender_id, client_info->status);

    switch ((MessageType)msg_type_byte)
    {
    case MSG_TYPE_C_HELLO:
        if (client_info->status != CLIENT_STATE_ACCEPTED)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Received C_HELLO from client ID %u in unexpected state (%d). Ignoring.", (unsigned int)sender_id, client_info->status);
            break;
        }
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Server] Received C_HELLO from client ID %u. Sending S_WELCOME.", (unsigned int)sender_id);
        Msg_WelcomeData welcome_msg;
        welcome_msg.message_type = MSG_TYPE_S_WELCOME;
        welcome_msg.assigned_client_id = sender_id;

        if (send_to_client(client_info, &welcome_msg, sizeof(welcome_msg)))
        {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Server] S_WELCOME sent successfully to client ID %u. Setting state to WELCOMED.", (unsigned int)sender_id);
            client_info->status = CLIENT_STATE_WELCOMED;
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Failed to send S_WELCOME to client ID %u after C_HELLO. Disconnecting.", (unsigned int)sender_id);
            disconnect_client(ns_state, client_index);
        }
        break;

    case MSG_TYPE_C_PLAYER_STATE:
        if (client_info->status != CLIENT_STATE_WELCOMED)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Received C_PLAYER_STATE from client ID %u not in WELCOMED state (%d). Ignoring.", (unsigned int)sender_id, client_info->status);
            break;
        }
        if (bytesReceived >= (int)sizeof(Msg_PlayerStateData))
        {
            Msg_PlayerStateData state_data;
            memcpy(&state_data, buffer, sizeof(Msg_PlayerStateData));

            if (state_data.client_id != sender_id)
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Received PLAYER_STATE from client %u claiming to be %u. Ignoring.", (unsigned int)sender_id, (unsigned int)state_data.client_id);
                break;
            }
            state_data.message_type = MSG_TYPE_S_PLAYER_STATE; // Change type for broadcast
            NetServer_BroadcastMessage(ns_state, &state_data, sizeof(Msg_PlayerStateData), client_index);
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Rcvd incomplete C_PLAYER_STATE msg from client %u (%d bytes, needed %lu)", (unsigned int)sender_id, bytesReceived, (unsigned long)sizeof(Msg_PlayerStateData));
        }
        break;

    case MSG_TYPE_C_SPAWN_ATTACK:
        if (client_info->status != CLIENT_STATE_WELCOMED)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Received C_SPAWN_ATTACK from client ID %u not in WELCOMED state (%d). Ignoring.", (unsigned int)sender_id, client_info->status);
            break;
        }
        if (bytesReceived >= (int)sizeof(Msg_ClientSpawnAttackData))
        {
            Msg_ClientSpawnAttackData req_data;
            memcpy(&req_data, buffer, sizeof(Msg_ClientSpawnAttackData));

            if (state->attack_manager)
            {
                AttackManager_HandleClientSpawnRequest(state->attack_manager, state, sender_id, req_data);
            }
            else
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Server] AttackManager is NULL when processing C_SPAWN_ATTACK.");
            }
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Rcvd incomplete C_SPAWN_ATTACK msg from client %u (%d bytes, needed %lu)", (unsigned int)sender_id, bytesReceived, (unsigned long)sizeof(Msg_ClientSpawnAttackData));
        }
        break;

    case MSG_TYPE_C_DAMAGE_PLAYER:
        if (client_info->status != CLIENT_STATE_WELCOMED)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Received C_DAMAGE_PLAYER from client ID %u not in WELCOMED state (%d). Ignoring.", (unsigned int)sender_id, client_info->status);
            break;
        }
        if (bytesReceived >= (int)sizeof(Msg_DamagePlayer))
        {
            Msg_DamagePlayer state_data;
            memcpy(&state_data, buffer, sizeof(Msg_DamagePlayer));

            state_data.message_type = MSG_TYPE_S_DAMAGE_PLAYER; // Change type for broadcast
            NetServer_BroadcastMessage(ns_state, &state_data, sizeof(Msg_DamagePlayer), client_index);
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Rcvd incomplete C_DAMAGE_PLAYER msg from client %u (%d bytes, needed %lu)", (unsigned int)sender_id, bytesReceived, (unsigned long)sizeof(Msg_DamagePlayer));
        }
        break;

    case MSG_TYPE_C_DAMAGE_MINION:
        if (client_info->status != CLIENT_STATE_WELCOMED)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Received C_DAMAGE_MINION from client ID %u not in WELCOMED state (%d). Ignoring.", (unsigned int)sender_id, client_info->status);
            break;
        }
        if (bytesReceived >= (int)sizeof(Msg_DamageMinion))
        {
            Msg_DamageMinion state_data;
            memcpy(&state_data, buffer, sizeof(Msg_DamageMinion));

            state_data.message_type = MSG_TYPE_S_DAMAGE_MINION;     // Change type for broadcast
            NetServer_BroadcastMessage(ns_state, &state_data, sizeof(Msg_DamageMinion), client_index);
        }
        else 
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Rcvd incomplete C_DAMAGE_MINION msg from client %u (%d bytes, needed %lu)", (unsigned int)sender_id, bytesReceived, (unsigned long)sizeof(Msg_DamageMinion));
        }
        break;


    case MSG_TYPE_C_DAMAGE_TOWER:
        if (client_info->status != CLIENT_STATE_WELCOMED)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Received C_DAMAGE_TOWER from client ID %u not in WELCOMED state (%d). Ignoring.", (unsigned int)sender_id, client_info->status);
            break;
        }
        if (bytesReceived >= (int)sizeof(Msg_DamageTower))
        {
            Msg_DamageTower state_data;
            memcpy(&state_data, buffer, sizeof(Msg_DamageTower));

            state_data.message_type = MSG_TYPE_S_DAMAGE_TOWER; // Change type for broadcast
            NetServer_BroadcastMessage(ns_state, &state_data, sizeof(Msg_DamageTower), client_index);
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Rcvd incomplete C_DAMAGE_TOWER msg from client %u (%d bytes, needed %lu)", (unsigned int)sender_id, bytesReceived, (unsigned long)sizeof(Msg_DamageTower));
        }
        break;

    case MSG_TYPE_C_DAMAGE_BASE:
        if (client_info->status != CLIENT_STATE_WELCOMED)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Received C_DAMAGE_BASE from client ID %u not in WELCOMED state (%d). Ignoring.", (unsigned int)sender_id, client_info->status);
            break;
        }
        if (bytesReceived >= (int)sizeof(Msg_DamageBase))
        {
            Msg_DamageBase state_data;
            memcpy(&state_data, buffer, sizeof(Msg_DamageBase));

            state_data.message_type = MSG_TYPE_S_DAMAGE_BASE; // Change type for broadcast
            NetServer_BroadcastMessage(ns_state, &state_data, sizeof(Msg_DamageBase), client_index);
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Rcvd incomplete C_DAMAGE_BASE msg from client %u (%d bytes, needed %lu)", (unsigned int)sender_id, bytesReceived, (unsigned long)sizeof(Msg_DamageBase));
        }
        break;

    case MSG_TYPE_C_MATCH_RESULT:
        if (client_info->status != CLIENT_STATE_WELCOMED)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Received C_MATCH_RESULT from client ID %u not in WELCOMED state (%d). Ignoring.", (unsigned int)sender_id, client_info->status);
            break;
        }
        if (bytesReceived >= (int)sizeof(Msg_MatchResult))
        {
            Msg_MatchResult state_data;
            memcpy(&state_data, buffer, sizeof(Msg_MatchResult));
            state_data.message_type = MSG_TYPE_S_GAME_RESULT; // Change type for broadcast
            NetServer_BroadcastMessage(ns_state, &state_data, sizeof(Msg_MatchResult), client_index);
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Rcvd incomplete C_MATCH_RESULT msg from client %u (%d bytes, needed %lu)", (unsigned int)sender_id, bytesReceived, (unsigned long)sizeof(Msg_MatchResult));
        }
        break;

    default:
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Rcvd unknown message type (%u) from client %u", (unsigned int)msg_type_byte, (unsigned int)sender_id);
        break;
    }
}

/**
 * @brief Checks for and accepts a new client connection if a slot is available.
 * Assigns a client ID and sets the initial state to ACCEPTED.
 * @param ns_state The NetServerState instance.
 * @param state The main AppState instance (unused in this function).
 */
static void accept_new_client(NetServerState ns_state, AppState *state)
{
    (void)state; // state is not directly used here but might be needed in future extensions
    if (!ns_state || !ns_state->listen_socket)
        return;

    SDLNet_StreamSocket *new_client_socket = NULL;

    if (SDLNet_AcceptClient(ns_state->listen_socket, &new_client_socket))
    {
        if (new_client_socket != NULL)
        {
            int client_index = find_inactive_client_slot(ns_state);
            if (client_index != -1)
            {
                ServerClientInfo *client_info = &ns_state->clients[client_index];
                client_info->socket = new_client_socket;
                client_info->status = CLIENT_STATE_ACCEPTED;
                client_info->client_id = (uint8_t)client_index; // Use index as ID for simplicity
                ns_state->connected_clients_count++;
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Server] Accepted new client connection, assigned ID %u at index %d. Waiting for C_HELLO.", (unsigned int)client_info->client_id, client_index);
            }
            else
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Rejected new client connection: Server full.");
                SDLNet_DestroyStreamSocket(new_client_socket);
            }
        }
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Server] SDLNet_AcceptClient failed: %s", SDL_GetError());
    }
}

/**
 * @brief Reads data from all active clients and processes received messages.
 * Handles disconnects if reads fail or indicate a closed connection.
 * @param ns_state The NetServerState instance.
 * @param state The main AppState instance.
 */
static void receive_from_all_clients(NetServerState ns_state, AppState *state)
{
    if (!ns_state)
        return;

    char buffer[BUFFER_SIZE];
    bool client_disconnected[MAX_CLIENTS] = {false}; // Track disconnects during read loop

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (ns_state->clients[i].status == CLIENT_STATE_INACTIVE)
            continue;

        ServerClientInfo *client_info = &ns_state->clients[i];
        int bytesReceived = 1; // Initialize to enter loop

        // Read all available data for this client in this cycle
        while (client_info->status != CLIENT_STATE_INACTIVE && bytesReceived > 0)
        {
            SDL_ClearError();
            bytesReceived = SDLNet_ReadFromStreamSocket(client_info->socket, buffer, sizeof(buffer));

            if (bytesReceived > 0)
            {
                internal_process_client_message(ns_state, i, buffer, bytesReceived, state);
                // Check status again in case processing caused disconnect
                if (client_info->status == CLIENT_STATE_INACTIVE)
                {
                    break;
                }
            }
            else if (bytesReceived == 0)
            {
                // No more data available for this client right now
                break;
            }
            else // bytesReceived < 0 indicates error or closed connection
            {
                const char *sdlError = SDL_GetError();
                if (sdlError && sdlError[0] != '\0' &&
                    strcmp(sdlError, "Socket is not connected") != 0 &&
                    strcmp(sdlError, "Connection reset by peer") != 0 &&
                    strcmp(sdlError, "Could not read from socket") != 0)
                {
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Server] Read error from client ID %u: %s. Marking for disconnect.", (unsigned int)client_info->client_id, sdlError);
                }
                else
                {
                    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Server] Connection closed for client ID %u (Read result: %d). Marking for disconnect.", (unsigned int)client_info->client_id, bytesReceived);
                }
                client_disconnected[i] = true;
                break; // Stop reading from this client
            }
        }
    }

    // Process disconnections after checking all clients
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (client_disconnected[i])
        {
            // Check status again before disconnecting
            if (ns_state->clients[i].status != CLIENT_STATE_INACTIVE)
            {
                disconnect_client(ns_state, i);
            }
        }
    }
}

// --- Static Callback Functions (for EntityManager) ---

/**
 * @brief Wrapper function conforming to EntityFunctions.update signature.
 * @param manager The EntityManager instance.
 * @param state Pointer to the main AppState.
 */
static void net_server_update_callback(EntityManager manager, AppState *state)
{
    (void)manager; // Manager instance is not used in this specific implementation
    NetServerState ns_state = state ? state->net_server_state : NULL;
    if (!ns_state)
        return;

    accept_new_client(ns_state, state);
    receive_from_all_clients(ns_state, state);
}

/**
 * @brief Wrapper function conforming to EntityFunctions.cleanup signature.
 * @param manager The EntityManager instance.
 * @param state Pointer to the main AppState.
 */
static void net_server_cleanup_callback(EntityManager manager, AppState *state)
{
    (void)manager; // Manager instance is not used in this specific implementation
    if (state)
    {
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "NetServer entity cleanup callback triggered.");
        // Actual resource cleanup happens in NetServer_Destroy
    }
}

// --- Public API Function Implementations ---

NetServerState NetServer_Init(AppState *state)
{
    if (!state || !state->entity_manager)
    {
        SDL_SetError("Invalid AppState or missing entity_manager for NetServer_Init");
        return NULL;
    }
    if (!state->is_server)
    {
        SDL_SetError("Attempted to initialize server module when not running as server.");
        return NULL;
    }

    NetServerState ns_state = (NetServerState)SDL_calloc(1, sizeof(struct NetServerState_s));
    if (!ns_state)
    {
        SDL_OutOfMemory();
        return NULL;
    }

    ns_state->listen_socket = NULL;
    ns_state->connected_clients_count = 0;
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        ns_state->clients[i].status = CLIENT_STATE_INACTIVE;
        ns_state->clients[i].socket = NULL;
    }

    ns_state->listen_socket = SDLNet_CreateServer(NULL, SERVER_PORT);
    if (!ns_state->listen_socket)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Server Init] SDLNet_CreateServer failed: %s", SDL_GetError());
        SDL_free(ns_state);
        return NULL;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Server] Listening on port %d...", SERVER_PORT);

    EntityFunctions net_server_funcs = {
        .name = "net_server",
        .update = net_server_update_callback,
        .cleanup = net_server_cleanup_callback,
        .render = NULL,
        .handle_events = NULL};

    if (!EntityManager_Add(state->entity_manager, &net_server_funcs))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Server Init] Failed to add entity to manager: %s", SDL_GetError());
        if (ns_state->listen_socket)
            SDLNet_DestroyServer(ns_state->listen_socket);
        SDL_free(ns_state);
        return NULL;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "NetServer module initialized and entity registered.");
    return ns_state;
}

void NetServer_Destroy(NetServerState ns_state)
{
    if (!ns_state)
        return;

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Destroying NetServerState...");

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (ns_state->clients[i].status != CLIENT_STATE_INACTIVE)
        {
            if (ns_state->clients[i].socket)
            {
                SDLNet_DestroyStreamSocket(ns_state->clients[i].socket);
                ns_state->clients[i].socket = NULL;
            }
            ns_state->clients[i].status = CLIENT_STATE_INACTIVE;
        }
    }

    if (ns_state->listen_socket)
    {
        SDLNet_DestroyServer(ns_state->listen_socket);
        ns_state->listen_socket = NULL;
    }

    SDL_free(ns_state);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "NetServerState container destroyed.");
}

void NetServer_BroadcastMessage(NetServerState ns_state, const void *buffer, int length, int exclude_client_index)
{
    internal_broadcast_message_impl(ns_state, buffer, length, exclude_client_index);
}
