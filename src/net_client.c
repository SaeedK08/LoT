#include "../include/net_client.h"

// --- Internal Structures ---

/**
 * @brief Represents the current network connection status of the client.
 */
typedef enum ClientNetworkStatus
{
    CLIENT_STATUS_DISCONNECTED, /**< Not connected and not attempting to connect. */
    CLIENT_STATUS_RESOLVING,    /**< Attempting to resolve the server hostname. */
    CLIENT_STATUS_CONNECTING,   /**< Attempting to connect to the resolved server address. */
    CLIENT_STATUS_CONNECTED     /**< Actively connected to the server. */
} ClientNetworkStatus;

/**
 * @brief Internal state for the NetClient module.
 */
struct NetClientState_s
{
    SDLNet_Address *server_address_resolved; /**< Resolved server address structure, or NULL. */
    SDLNet_StreamSocket *server_connection;  /**< Active socket connection to the server, or NULL. */
    ClientNetworkStatus network_status;      /**< Current connection status. */
    int my_client_id;                        /**< Client ID assigned by the server, or -1 if not assigned. */
    Uint64 last_state_send_time;             /**< Timestamp of the last player state message sent. */
    char hostname[MAX_NAME_LENGTH];          /**< Hostname to connect to, provided by the user or default. */
};

// --- Constants ---
const Uint32 STATE_UPDATE_INTERVAL_MS = 50; /**< Interval (ms) for sending player state updates. */

// --- Static Helper Functions ---

/**
 * @brief Sends a data buffer to the server.
 * Handles potential disconnect on send failure.
 * @param nc_state The NetClientState instance.
 * @param buffer Pointer to the data buffer to send.
 * @param length The number of bytes to send from the buffer.
 * @return True if the send was successful, false otherwise (indicates disconnect).
 */
static bool NetClient_SendBuffer(NetClientState nc_state, const void *buffer, int length)
{
    if (!nc_state || nc_state->network_status != CLIENT_STATUS_CONNECTED || !nc_state->server_connection)
    {
        return false;
    }
    if (!SDLNet_WriteToStreamSocket(nc_state->server_connection, buffer, length))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] Send failed: %s. Disconnecting.", SDL_GetError());
        NetClient_Destroy(nc_state); // Trigger full cleanup on send failure
        // If AppState is accessible here, nullify its pointer too
        return false;
    }

    return true;
}

/**
 * @brief Attempts to start resolving the server hostname asynchronously.
 * @param nc_state The NetClientState instance.
 */
static void internal_attempt_resolve(NetClientState nc_state)
{
    if (!nc_state || nc_state->network_status != CLIENT_STATUS_DISCONNECTED)
        return;

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Attempting to resolve hostname: %s", nc_state->hostname);
    if (nc_state->server_address_resolved)
    {
        SDLNet_UnrefAddress(nc_state->server_address_resolved);
        nc_state->server_address_resolved = NULL;
    }

    // Use the hostname stored in nc_state
    nc_state->server_address_resolved = SDLNet_ResolveHostname(nc_state->hostname);
    if (nc_state->server_address_resolved == NULL)
    {
        // This is a critical error during synchronous part of resolution
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] SDLNet_ResolveHostname failed immediately for '%s': %s", nc_state->hostname, SDL_GetError());
        // Consider further error handling, e.g. setting status to an error state or retrying
    }
    else
    {
        nc_state->network_status = CLIENT_STATUS_RESOLVING;
    }
}

/**
 * @brief Checks the status of an ongoing hostname resolution attempt.
 * Transitions state if resolved or handles cleanup on failure.
 * @param nc_state The NetClientState instance.
 */
static void internal_check_resolve_status(NetClientState nc_state)
{
    if (!nc_state || nc_state->network_status != CLIENT_STATUS_RESOLVING || !nc_state->server_address_resolved)
        return;

    int status = SDLNet_GetAddressStatus(nc_state->server_address_resolved);
    if (status == 1) // 1 indicates success
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Hostname resolved.");
        nc_state->network_status = CLIENT_STATUS_DISCONNECTED; // Ready to attempt connection
    }
    else if (status == -1) // -1 indicates failure
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] SDLNet_ResolveHostname failed async: %s", SDL_GetError());
        NetClient_Destroy(nc_state); // Full cleanup on resolve failure
    }
    // status == 0 means still resolving, do nothing
}

/**
 * @brief Attempts to initiate a non-blocking connection to the resolved server address.
 * @param nc_state The NetClientState instance.
 */
static void internal_attempt_connection(NetClientState nc_state)
{
    if (!nc_state || nc_state->network_status != CLIENT_STATUS_DISCONNECTED || !nc_state->server_address_resolved)
        return;

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Attempting connection to port %d...", SERVER_PORT);
    if (nc_state->server_connection)
    {
        SDLNet_DestroyStreamSocket(nc_state->server_connection);
        nc_state->server_connection = NULL;
    }

    nc_state->server_connection = SDLNet_CreateClient(nc_state->server_address_resolved, SERVER_PORT);
    if (nc_state->server_connection == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] SDLNet_CreateClient failed: %s", SDL_GetError());
    }
    else
    {
        nc_state->network_status = CLIENT_STATUS_CONNECTING;
    }
}

/**
 * @brief Checks the status of an ongoing connection attempt.
 * Transitions state to CONNECTED on success, sends C_HELLO, or handles cleanup on failure.
 * @param nc_state The NetClientState instance.
 */
static void internal_check_connection_status(NetClientState nc_state)
{
    if (!nc_state || nc_state->network_status != CLIENT_STATUS_CONNECTING || !nc_state->server_connection)
        return;

    int status = SDLNet_GetConnectionStatus(nc_state->server_connection);
    if (status == 1) // 1 indicates success
    {
        nc_state->network_status = CLIENT_STATUS_CONNECTED;
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Connected to server!");

        uint8_t msg_type = MSG_TYPE_C_HELLO;
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Sending C_HELLO.");
        if (!NetClient_SendBuffer(nc_state, &msg_type, sizeof(msg_type)))
        {
            return; // SendBuffer handles disconnect on failure
        }
        nc_state->last_state_send_time = SDL_GetTicks();
    }
    else if (status == -1) // -1 indicates failure
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] Connection failed: %s", SDL_GetError());
        NetClient_Destroy(nc_state); // Full cleanup on connection failure
    }
    // status == 0 means still connecting, do nothing
}

/**
 * @brief Sends the local player's current state to the server.
 * @param nc_state The NetClientState instance.
 * @param state The main AppState instance.
 */
static void internal_send_local_player_state(NetClientState nc_state, AppState *state)
{
    if (!nc_state || nc_state->network_status != CLIENT_STATUS_CONNECTED || nc_state->my_client_id < 0 || !state || !state->player_manager)
    {
        return;
    }

    Msg_PlayerStateData data;
    if (!PlayerManager_GetLocalPlayerState(state->player_manager, &data))
    {
        return;
    }

    NetClient_SendBuffer(nc_state, &data, sizeof(Msg_PlayerStateData));
}


/**
 * @brief Processes a single message received from the server based on its type.
 * @param nc_state The NetClientState instance.
 * @param buffer Pointer to the received data buffer.
 * @param bytesReceived Number of bytes in the buffer.
 * @param state The main AppState instance.
 */
static void internal_process_server_message(NetClientState nc_state, char *buffer, int bytesReceived, AppState *state)
{
    if (!nc_state || !state || bytesReceived < (int)sizeof(uint8_t))
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Rcvd incomplete message (< type byte)");
        return;
    }

    uint8_t msg_type_byte = (uint8_t)buffer[0];

    switch ((MessageType)msg_type_byte)
    {
    case MSG_TYPE_S_WELCOME:
        if (nc_state->my_client_id != -1)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Received duplicate S_WELCOME (myID already %d). Ignoring.", nc_state->my_client_id);
            return;
        }
        if (bytesReceived >= (int)sizeof(Msg_WelcomeData))
        {
            Msg_WelcomeData welcome_data;
            memcpy(&welcome_data, buffer, sizeof(Msg_WelcomeData));
            nc_state->my_client_id = welcome_data.assigned_client_id;
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Received S_WELCOME, assigned myClientID = %d", nc_state->my_client_id);
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Rcvd incomplete S_WELCOME (%d bytes, needed %lu)", bytesReceived, (unsigned long)sizeof(Msg_WelcomeData));
        }
        break;

    case MSG_TYPE_S_GAME_START:
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Received S_GAME_START, assigned myClientID = %d", nc_state->my_client_id);
        state->currentGameState = GAME_STATE_PLAYING;

        if (!state->player_manager || !state->camera_state)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] PlayerManager or CameraState is NULL when processing S_WELCOME.");
            NetClient_Destroy(nc_state);
            return;
        }
        if (!PlayerManager_SetLocalPlayerID(state, nc_state->my_client_id))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] Failed to set local player ID %d in PlayerManager. Error: %s", nc_state->my_client_id, SDL_GetError());
            NetClient_Destroy(nc_state);
            return;
        }
        create_hud_instace(state, get_hud_index_by_name(state, "lobby_client_msg"), "lobby_client_msg", false, "Client: Wating for host to start the game", (SDL_Color){255, 255, 255, 255}, false, (SDL_FPoint){0.0f, 0.0f}, 0);

        break;

    case MSG_TYPE_S_PLAYER_STATE:
        if (bytesReceived >= (int)sizeof(Msg_PlayerStateData))
        {
            Msg_PlayerStateData state_data;
            memcpy(&state_data, buffer, sizeof(Msg_PlayerStateData));
            if (state->player_manager)
            {
                PlayerManager_UpdateRemotePlayer(state, &state_data);
            }
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Rcvd incomplete S_PLAYER_STATE msg (%d bytes, needed %lu)", bytesReceived, (unsigned long)sizeof(Msg_PlayerStateData));
        }
        break;

    case MSG_TYPE_S_PLAYER_DISCONNECT:
        if (bytesReceived >= (int)sizeof(Msg_PlayerDisconnectData))
        {
            Msg_PlayerDisconnectData disconnect_data;
            memcpy(&disconnect_data, buffer, sizeof(Msg_PlayerDisconnectData));
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Received disconnect for client %u", (unsigned int)disconnect_data.client_id);
            if (state->player_manager)
            {
                PlayerManager_RemovePlayer(state->player_manager, disconnect_data.client_id);
            }
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Rcvd incomplete S_PLAYER_DISCONNECT msg (%d bytes, needed %lu)", bytesReceived, (unsigned long)sizeof(Msg_PlayerDisconnectData));
        }
        break;

    case MSG_TYPE_S_SPAWN_ATTACK:
        if (bytesReceived >= (int)sizeof(Msg_ServerSpawnAttackData))
        {
            Msg_ServerSpawnAttackData spawn_data;
            memcpy(&spawn_data, buffer, sizeof(Msg_ServerSpawnAttackData));
            if (state->attack_manager)
            {
                AttackManager_HandleServerSpawn(state->attack_manager, &spawn_data);
            }
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Rcvd incomplete S_SPAWN_ATTACK msg (%d bytes, needed %lu)", bytesReceived, (unsigned long)sizeof(Msg_ServerSpawnAttackData));
        }
        break;

    case MSG_TYPE_S_DESTROY_OBJECT:
        if (bytesReceived >= (int)sizeof(Msg_DestroyObjectData))
        {
            Msg_DestroyObjectData destroy_data;
            memcpy(&destroy_data, buffer, sizeof(Msg_DestroyObjectData));
            if (destroy_data.object_type == OBJECT_TYPE_ATTACK && state->attack_manager)
            {
                AttackManager_HandleDestroyObject(state->attack_manager, &destroy_data);
            }
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Rcvd incomplete S_DESTROY_OBJECT msg (%d bytes, needed %lu)", bytesReceived, (unsigned long)sizeof(Msg_DestroyObjectData));
        }
        break;

    case MSG_TYPE_S_DAMAGE_PLAYER:
        if (bytesReceived >= (int)sizeof(Msg_DamagePlayer))
        {
            Msg_DamagePlayer state_data;
            memcpy(&state_data, buffer, sizeof(Msg_DamagePlayer));
            if (state->player_manager)
            {
                damagePlayer(*state, state_data.playerIndex, state_data.damageValue, false);
            }
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Rcvd incomplete S_DAMAGE_PLAYER msg (%d bytes, needed %lu)", bytesReceived, (unsigned long)sizeof(Msg_DamagePlayer));
        }
        break;

    case MSG_TYPE_S_DAMAGE_MINION: 
        if (bytesReceived >= (int)sizeof(Msg_DamageMinion))
        {
            Msg_DamageMinion state_data;
            memcpy(&state_data, buffer, sizeof(Msg_DamageMinion));
            if (state->minion_manager)
            {
                damageMinion(*state, state_data.minionIndex, 0, false, state_data.current_health);
            }
        }
        else 
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Rcvd incomplete S_DAMAGE_MINION msg (%d bytes, needed %lu)", bytesReceived, (unsigned long)sizeof(Msg_DamageMinion));
        }
        break;

    case MSG_TYPE_S_DAMAGE_TOWER:
        if (bytesReceived >= (int)sizeof(Msg_DamageTower))
        {
            Msg_DamageTower state_data;
            memcpy(&state_data, buffer, sizeof(Msg_DamageTower));
            if (state->tower_manager)
            {
                damageTower(*state, state_data.towerIndex, state_data.damageValue, false, state_data.current_health);
            }
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Rcvd incomplete S_DAMAGE_TOWER msg (%d bytes, needed %lu)", bytesReceived, (unsigned long)sizeof(Msg_DamageTower));
        }
        break;

    case MSG_TYPE_S_DAMAGE_BASE:
        if (bytesReceived >= (int)sizeof(Msg_DamageBase))
        {
            Msg_DamageBase state_data;
            memcpy(&state_data, buffer, sizeof(Msg_DamageBase));
            if (state->base_manager)
            {
                damageBase(*state, state_data.baseIndex, state_data.damageValue, false);
            }
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Rcvd incomplete S_DAMAGE_BASE msg (%d bytes, needed %lu)", bytesReceived, (unsigned long)sizeof(Msg_DamageBase));
        }
        break;

    case MSG_TYPE_S_GAME_RESULT:
        if (bytesReceived >= (int)sizeof(Msg_MatchResult))
        {
            Msg_MatchResult state_data;
            memcpy(&state_data, buffer, sizeof(Msg_MatchResult));
            SDL_Log("\n---\nMatch Won by team %s\n---\n", state_data.winningTeam ? "RED" : "BLUE");
            state->currentGameState = GAME_STATE_FINISHED;
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Rcvd incomplete S_DAMAGE_BASE msg (%d bytes, needed %lu)", bytesReceived, (unsigned long)sizeof(Msg_MatchResult));
        }
        break;

    default:
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Rcvd unknown message type (%u) from server", (unsigned int)msg_type_byte);
        break;
    }
}

/**
 * @brief Reads available data from the server socket and processes messages.
 * Handles disconnects if reads fail.
 * @param nc_state The NetClientState instance.
 * @param state The main AppState instance.
 * @return True if the connection is still active after reading, false if disconnected.
 */
static bool internal_receive_server_data(NetClientState nc_state, AppState *state)
{
    if (!nc_state || nc_state->network_status != CLIENT_STATUS_CONNECTED || !nc_state->server_connection)
    {
        return false;
    }

    char buffer[BUFFER_SIZE];
    int bytesReceived;
    SDL_ClearError();

    // Read all available data in the socket buffer for this frame
    while ((bytesReceived = SDLNet_ReadFromStreamSocket(nc_state->server_connection, buffer, sizeof(buffer))) > 0)
    {
        internal_process_server_message(nc_state, buffer, bytesReceived, state);
        // Check if processing caused a disconnect (e.g., critical error)
        if (nc_state->network_status != CLIENT_STATUS_CONNECTED)
        {
            return false;
        }
        // Stop reading if the buffer wasn't filled, indicating no more immediate data
        if (bytesReceived < (int)sizeof(buffer))
        {
            break;
        }
    }

    // Handle read errors or closed connection
    if (bytesReceived < 0)
    {
        const char *sdlError = SDL_GetError();
        if (sdlError && sdlError[0] != '\0' &&
            strcmp(sdlError, "Socket is not connected") != 0 &&
            strcmp(sdlError, "Connection reset by peer") != 0 &&
            strcmp(sdlError, "Could not read from socket") != 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] Read error: %s. Disconnecting.", sdlError);
        }
        else
        {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Connection closed (Read result: %d). Disconnecting.", bytesReceived);
        }
        NetClient_Destroy(nc_state);
        if (state && state->net_client_state == nc_state)
        {
            state->net_client_state = NULL; // Nullify pointer in AppState
        }
        return false;
    }
    // bytesReceived == 0 means no data available, connection is still fine

    return true;
}

/**
 * @brief Handles all communication logic when in the CONNECTED state.
 * Reads incoming data and sends outgoing player state updates periodically.
 * @param nc_state The NetClientState instance.
 * @param state The main AppState instance.
 */
static void internal_handle_server_communication(NetClientState nc_state, AppState *state)
{
    if (!nc_state || nc_state->network_status != CLIENT_STATUS_CONNECTED)
        return;

    if (!internal_receive_server_data(nc_state, state))
    {
        return; // Disconnected during receive
    }

    // Check status again after receiving data
    if (nc_state->network_status != CLIENT_STATUS_CONNECTED)
        return;

    // Send state updates periodically
    Uint64 current_time = SDL_GetTicks();
    if (nc_state->my_client_id >= 0 && current_time > nc_state->last_state_send_time + STATE_UPDATE_INTERVAL_MS)
    {
        internal_send_local_player_state(nc_state, state);
        // internal_send_local_minion_state(nc_state, state);
        // Check status again after send, as it might trigger disconnect
        if (nc_state->network_status == CLIENT_STATUS_CONNECTED)
        {
            nc_state->last_state_send_time = current_time;
        }
    }
}

// --- Static Callback Functions (for EntityManager) ---

/**
 * @brief Wrapper function conforming to EntityFunctions.update signature.
 * Manages the client's network state machine (resolving, connecting, communicating).
 * @param manager The EntityManager instance.
 * @param state Pointer to the main AppState.
 */
static void net_client_update_callback(EntityManager manager, AppState *state)
{
    (void)manager; // Manager instance is not used in this specific implementation
    NetClientState nc_state = state ? state->net_client_state : NULL;
    if (!nc_state)
        return;

    switch (nc_state->network_status)
    {
    case CLIENT_STATUS_DISCONNECTED:
        if (!nc_state->server_address_resolved)
        {
            internal_attempt_resolve(nc_state);
        }
        else
        {
            internal_attempt_connection(nc_state);
        }
        break;
    case CLIENT_STATUS_RESOLVING:
        internal_check_resolve_status(nc_state);
        break;
    case CLIENT_STATUS_CONNECTING:
        internal_check_connection_status(nc_state);
        break;
    case CLIENT_STATUS_CONNECTED:
        internal_handle_server_communication(nc_state, state);
        break;
    }
}

/**
 * @brief Wrapper function conforming to EntityFunctions.cleanup signature.
 * @param manager The EntityManager instance.
 * @param state Pointer to the main AppState.
 */
static void net_client_cleanup_callback(EntityManager manager, AppState *state)
{
    (void)manager; // Manager instance is not used in this specific implementation
    if (state && state->net_client_state)
    {
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "NetClient entity cleanup callback triggered.");
        // Actual resource cleanup happens in NetClient_Destroy
    }
}

// --- Public API Function Implementations ---

NetClientState NetClient_Init(AppState *state, const char *hostname)
{
    if (!state || !state->entity_manager)
    {
        SDL_SetError("Invalid AppState or missing entity_manager for NetClient_Init");
        return NULL;
    }

    NetClientState nc_state = (NetClientState)SDL_calloc(1, sizeof(struct NetClientState_s));
    if (!nc_state)
    {
        SDL_OutOfMemory();
        return NULL;
    }

    // Copy the provided hostname to the struct
    strncpy(nc_state->hostname, hostname, MAX_NAME_LENGTH - 1);
    nc_state->hostname[MAX_NAME_LENGTH - 1] = '\0'; // Ensure null-termination

    nc_state->network_status = CLIENT_STATUS_DISCONNECTED;
    nc_state->server_address_resolved = NULL;
    nc_state->server_connection = NULL;
    nc_state->my_client_id = -1;
    nc_state->last_state_send_time = 0;

    EntityFunctions net_client_funcs = {
        .name = "net_client",
        .update = net_client_update_callback,
        .cleanup = net_client_cleanup_callback,
        .render = NULL,
        .handle_events = NULL};

    if (!EntityManager_Add(state->entity_manager, &net_client_funcs))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[NetClient Init] Failed to add entity to manager: %s", SDL_GetError());
        SDL_free(nc_state);
        return NULL;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "NetClient module initialized and entity registered.");
    return nc_state;
}

void NetClient_Destroy(NetClientState nc_state)
{
    if (!nc_state)
        return;

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Destroying NetClientState...");
    if (nc_state->server_connection != NULL)
    {
        SDLNet_DestroyStreamSocket(nc_state->server_connection);
        nc_state->server_connection = NULL;
    }
    if (nc_state->server_address_resolved != NULL)
    {
        SDLNet_UnrefAddress(nc_state->server_address_resolved);
        nc_state->server_address_resolved = NULL;
    }
    nc_state->network_status = CLIENT_STATUS_DISCONNECTED;
    nc_state->my_client_id = -1;

    SDL_free(nc_state);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "NetClientState container destroyed.");
}

int NetClient_GetClientID(NetClientState nc_state)
{
    return nc_state ? nc_state->my_client_id : -1;
}

bool NetClient_IsConnected(NetClientState nc_state)
{
    return nc_state && nc_state->network_status == CLIENT_STATUS_CONNECTED;
}

bool NetClient_SendSpawnAttackRequest(NetClientState nc_state, AttackType type, float target_world_x, float target_world_y, bool team)
{
    if (!NetClient_IsConnected(nc_state))
    {
        return false;
    }

    Msg_ClientSpawnAttackData msg;
    msg.message_type = MSG_TYPE_C_SPAWN_ATTACK;
    msg.attack_type = (uint8_t)type;
    msg.target_pos.x = target_world_x;
    msg.target_pos.y = target_world_y;
    msg.team = team;

    return NetClient_SendBuffer(nc_state, &msg, sizeof(Msg_ClientSpawnAttackData));
}

bool NetClient_SendDamagePlayerRequest(NetClientState nc_state, int playerIndex, float damageValue)
{
    if (!NetClient_IsConnected(nc_state))
    {
        return false;
    }

    Msg_DamagePlayer msg;
    msg.message_type = MSG_TYPE_C_DAMAGE_PLAYER;
    msg.playerIndex = playerIndex;
    msg.damageValue = damageValue;

    return NetClient_SendBuffer(nc_state, &msg, sizeof(Msg_DamagePlayer));
}

bool NetClient_SendDamageMinionRequest(NetClientState nc_state, int minionIndex, float sentCurrentHealth)
{
    if (!NetClient_IsConnected(nc_state))
    {
        return false;
    }
    Msg_DamageMinion msg;
    msg.message_type = MSG_TYPE_C_DAMAGE_MINION;
    msg.minionIndex = minionIndex;
    msg.current_health = sentCurrentHealth;
    return NetClient_SendBuffer(nc_state, &msg, sizeof(Msg_DamageMinion));
}


bool NetClient_SendDamageTowerRequest(NetClientState nc_state, int towerIndex, float damageValue, float current_health)
{
    if (!NetClient_IsConnected(nc_state))
    {
        return false;
    }

    Msg_DamageTower msg;
    msg.message_type = MSG_TYPE_C_DAMAGE_TOWER;
    msg.towerIndex = towerIndex;
    msg.damageValue = damageValue;
    msg.current_health = current_health;

    return NetClient_SendBuffer(nc_state, &msg, sizeof(Msg_DamageTower));
}

bool NetClient_SendDamageBaseRequest(NetClientState nc_state, int baseIndex, float damageValue)
{
    if (!NetClient_IsConnected(nc_state))
    {
        return false;
    }

    Msg_DamageBase msg;
    msg.message_type = MSG_TYPE_C_DAMAGE_BASE;
    msg.baseIndex = baseIndex;
    msg.damageValue = damageValue;

    return NetClient_SendBuffer(nc_state, &msg, sizeof(Msg_DamageBase));
}

bool NetClient_SendMatchResult(NetClientState nc_state, bool winningTeam)
{
    if (!NetClient_IsConnected(nc_state))
    {
        return false;
    }

    Msg_MatchResult msg;
    msg.message_type = MSG_TYPE_C_MATCH_RESULT;
    msg.winningTeam = winningTeam;

    SDL_Log("NetClient_SendMatchResult %d", msg.winningTeam);

    return NetClient_SendBuffer(nc_state, &msg, sizeof(Msg_MatchResult));
}