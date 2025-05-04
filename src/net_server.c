#include "../include/net_server.h"

/**
 * @brief Holds connection and state information for a single connected client.
 */
typedef struct
{
    SDLNet_StreamSocket *socket;
    PlayerStateData last_received_state; /**< Last known state received from this client. */
    bool active;
} ClientInfo;

// --- Module Variables ---

static SDLNet_Server *serverSocket = NULL;
static ClientInfo clients[MAX_CLIENTS];
// --- Forward Declarations ---

static void disconnectClient(int clientIndex, const char *reason);
static bool send_buffer_to_client(int clientIndex, const void *buffer, int length);
void broadcast_buffer(const void *buffer, int length, int excludeClientIndex);
static void broadcast_building_destroyed(MessageType msg_type, int buildingIndex, int clientIndex);

// --- Static Helper Functions ---

/**
 * @brief Finds the first inactive client slot.
 * @return The index of the first available slot, or -1 if all slots are full.
 */
static int find_available_client_slot(void)
{
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (!clients[i].active)
            return i;
    }
    return -1; // No available slot
}

/**
 * @brief Initializes a client slot with a new socket connection and default state.
 * @param slotIndex The index of the client slot to initialize.
 * @param socket The newly accepted client socket.
 * @return void
 */
static void initialize_client_slot(int slotIndex, SDLNet_StreamSocket *socket)
{
    if (slotIndex < 0 || slotIndex >= MAX_CLIENTS || socket == NULL)
        return;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Server] Initializing client slot %d.", slotIndex);
    clients[slotIndex].socket = socket;
    clients[slotIndex].active = true;
    memset(&clients[slotIndex].last_received_state, 0, sizeof(PlayerStateData));
    clients[slotIndex].last_received_state.client_id = (uint8_t)slotIndex;
}

/**
 * @brief Accepts any pending incoming client connections and assigns them to available slots.
 * @return SDL_APP_SUCCESS if successful or no new connections, SDL_APP_FAILURE if server socket is invalid.
 */
static SDL_AppResult acceptConnections(void)
{
    if (serverSocket == NULL)
        return SDL_APP_FAILURE; // Cannot accept if server socket isn't valid

    SDLNet_StreamSocket *newClientSocket = NULL;
    // Poll for new connections until no more are pending
    while (SDLNet_AcceptClient(serverSocket, &newClientSocket) && newClientSocket != NULL)
    {
        int clientSlot = find_available_client_slot();
        if (clientSlot != -1)
        {
            initialize_client_slot(clientSlot, newClientSocket);
            newClientSocket = NULL; // Socket ownership transferred
        }
        else
        {
            // Server full, reject the connection
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Server full. Rejecting connection.");
            SDLNet_DestroyStreamSocket(newClientSocket);
            newClientSocket = NULL;
            break; // Stop checking if full
        }
    }

    // Check for errors during accept (ignore "No incoming connections")
    const char *error = SDL_GetError();
    if (error && error[0] != '\0' && strcmp(error, "No incoming connections") != 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Server] SDLNet_AcceptClient error: %s", error);
        // This might indicate a bigger issue, but continue running for now
    }
    return SDL_APP_SUCCESS;
}

/**
 * @brief Attempts to read data from a specific client's stream socket.
 * @param clientIndex The index of the client to read from.
 * @param buffer The buffer to store received data.
 * @param bufferSize The maximum number of bytes to read into the buffer.
 * @return The number of bytes received (0 if non-blocking and no data), -1 on error or disconnect.
 */
static int read_from_client(int clientIndex, char *buffer, int bufferSize)
{
    if (clientIndex < 0 || clientIndex >= MAX_CLIENTS || !clients[clientIndex].active || clients[clientIndex].socket == NULL)
        return -1; // Invalid client or not connected
    // Note: Assuming non-blocking based on typical game loop usage
    return SDLNet_ReadFromStreamSocket(clients[clientIndex].socket, buffer, bufferSize);
}

/**
 * @brief Handles the C_HELLO message from a client by sending back S_WELCOME.
 * @param clientIndex The index of the client who sent the message.
 * @return void
 */
static void handle_client_hello(int clientIndex)
{
    if (clientIndex < 0 || clientIndex >= MAX_CLIENTS || !clients[clientIndex].active)
        return;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Server] Rcvd C_HELLO from client %d.", clientIndex);

    // Prepare S_WELCOME message [Type][ClientID]
    int welcomeMsg[2];
    welcomeMsg[0] = (int)MSG_TYPE_S_WELCOME; // Message Type
    welcomeMsg[1] = clientIndex;             // Assigned Client ID

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Server] Sending S_WELCOME to client %d (ID: %d)", clientIndex, welcomeMsg[1]);
    send_buffer_to_client(clientIndex, welcomeMsg, sizeof(welcomeMsg)); // Forward declaration needed
}

/**
 * @brief Handles the PLAYER_STATE message, updating the server's state for the client
 * and broadcasting the update to other clients.
 * @param clientIndex The index of the client sending the state.
 * @param data Pointer to the received PlayerStateData.
 * @return void
 */
static void handle_player_state_update(int clientIndex, const PlayerStateData *data)
{
    if (clientIndex < 0 || clientIndex >= MAX_CLIENTS || !clients[clientIndex].active || !data)
        return;

    // Validate that the client ID in the data matches the sender's index
    if (data->client_id != clientIndex)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Client %d sent state update claiming to be client %u. Ignoring.", clientIndex, (unsigned int)data->client_id);
        return;
    }

    // Update server's stored state for this client
    clients[clientIndex].last_received_state = *data;

    // Prepare broadcast buffer: [MSG_TYPE] [PlayerStateData]
    uint8_t broadcast_msg[sizeof(uint8_t) + sizeof(PlayerStateData)];
    broadcast_msg[0] = MSG_TYPE_PLAYER_STATE;
    memcpy(broadcast_msg + sizeof(uint8_t), data, sizeof(PlayerStateData));

    // Broadcast to all *other* clients
    broadcast_buffer(broadcast_msg, sizeof(broadcast_msg), clientIndex);
}

/**
 * @brief Processes a single message received from a client based on its type byte.
 * @param clientIndex The index of the client sending the message.
 * @param buffer Pointer to the received data buffer.
 * @param bytesReceived The number of bytes in the buffer.
 * @return void
 */
static void process_client_message(int clientIndex, char *buffer, int bytesReceived)
{
    if (bytesReceived < (int)sizeof(uint8_t))
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Rcvd incomplete message (< type byte) from client %d", clientIndex);
        return;
    }
    uint8_t msg_type_byte;
    memcpy(&msg_type_byte, buffer, sizeof(uint8_t)); // Read the type byte

    switch ((MessageType)msg_type_byte)
    {
    case MSG_TYPE_C_HELLO:
        handle_client_hello(clientIndex);
        break;

    case MSG_TYPE_PLAYER_STATE:
        if (bytesReceived >= (int)(sizeof(uint8_t) + sizeof(PlayerStateData)))
        {
            PlayerStateData state_data;
            memcpy(&state_data, buffer + sizeof(uint8_t), sizeof(PlayerStateData)); // Read payload
            handle_player_state_update(clientIndex, &state_data);
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Rcvd incomplete PLAYER_STATE msg from %d (%d bytes, needed %u)", clientIndex, bytesReceived, (unsigned int)(sizeof(uint8_t) + sizeof(PlayerStateData)));
        }
        break;
    case MSG_TYPE_BLUE_WON:

        broadcast_building_destroyed(MSG_TYPE_BLUE_WON, buffer[4], clientIndex);

        break;
    case MSG_TYPE_RED_WON:
        broadcast_building_destroyed(MSG_TYPE_RED_WON, buffer[4], clientIndex);

        break;

    case MSG_TYPE_TOWER_DESTROYED:
        broadcast_building_destroyed(MSG_TYPE_TOWER_DESTROYED, buffer[4], clientIndex);

        break;

    default:
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Server] Rcvd unknown message type (%u) from client %d", (unsigned int)msg_type_byte, clientIndex);
        break;
    }
}

/**
 * @brief Reads and processes all available data from a single client's socket.
 * Handles disconnection if read fails.
 * @param clientIndex The index of the client to handle.
 * @return void
 */
static void handle_single_client(int clientIndex)
{
    if (clientIndex < 0 || clientIndex >= MAX_CLIENTS || !clients[clientIndex].active || clients[clientIndex].socket == NULL)
        return;

    char buffer[BUFFER_SIZE];
    int bytesReceived;

    // Read messages in a loop until no more data or buffer is full
    while ((bytesReceived = read_from_client(clientIndex, buffer, sizeof(buffer))) > 0)
    {
        process_client_message(clientIndex, buffer, bytesReceived);
        // If we didn't fill the buffer, we've read the last pending message for now
        if (bytesReceived < (int)sizeof(buffer))
            break;
    }

    if (bytesReceived < 0) // Read failed or disconnected
    {
        const char *sdlError = SDL_GetError();
        // Log error only if it's not a standard disconnect reason
        if (sdlError && sdlError[0] != '\0' && strcmp(sdlError, "Socket is not connected") != 0 && strcmp(sdlError, "Connection reset by peer") != 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Server] Read error (Client %d): %s.", clientIndex, sdlError);
        }
        else
        {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Server] Client %d disconnected.", clientIndex);
        }
        disconnectClient(clientIndex, "Read error or disconnect"); // Forward declaration needed
    }
}

/**
 * @brief Main update function for the network server entity, run each frame.
 * Accepts new connections and handles communication for all active clients.
 * @param state The application state (unused in this function).
 * @return void
 */
static void update(AppState *state)
{
    (void)state; // Explicitly mark state as unused

    // --- Accept New Connections ---
    SDL_ClearError(); // Clear old errors before accepting
    if (acceptConnections() == SDL_APP_FAILURE)
    {
        // Error logged within acceptConnections if server socket was invalid
    }

    // --- Handle Active Clients ---
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i].active)
        {
            handle_single_client(i);
        }
    }
}

/**
 * @brief Cleans up server resources, disconnecting clients and destroying the server socket.
 * @return void
 */
static void cleanup(void)
{
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Server] Shutting down...");
    // Disconnect all active clients
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i].active)
        {
            disconnectClient(i, "Server shutdown"); // Forward declaration needed
        }
    }
    // Clear client array structure
    memset(clients, 0, sizeof(clients));

    // Destroy the server socket if it exists
    if (serverSocket != NULL)
    {
        SDLNet_DestroyServer(serverSocket);
        serverSocket = NULL;
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Server] Server socket destroyed.");
    }
}

// --- Functions with Circular Dependencies ---

/**
 * @brief Disconnects a specific client, closes their socket, and marks the slot inactive.
 * @param clientIndex The index of the client to disconnect.
 * @param reason A string indicating the reason for disconnection (for logging).
 * @return void
 */
static void disconnectClient(int clientIndex, const char *reason)
{
    if (clientIndex < 0 || clientIndex >= MAX_CLIENTS || !clients[clientIndex].active)
        return; // Already inactive or invalid index

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Server] Disconnecting client %d. Reason: %s", clientIndex, reason ? reason : "Unknown");

    // Close the socket if it exists
    if (clients[clientIndex].socket != NULL)
    {
        SDLNet_DestroyStreamSocket(clients[clientIndex].socket);
        clients[clientIndex].socket = NULL;
    }

    // Mark slot as inactive and clear state
    clients[clientIndex].active = false;
    memset(&clients[clientIndex].last_received_state, 0, sizeof(PlayerStateData));
}

/**
 * @brief Sends a data buffer to a specific client via their established connection.
 * Disconnects the client if the send fails.
 * @param clientIndex The index of the target client.
 * @param buffer Pointer to the data buffer to send.
 * @param length The number of bytes to send from the buffer.
 * @return true if the send was successful, false otherwise (and triggers disconnect).
 */
static bool send_buffer_to_client(int clientIndex, const void *buffer, int length)
{
    if (clientIndex < 0 || clientIndex >= MAX_CLIENTS || !clients[clientIndex].active || clients[clientIndex].socket == NULL)
        return false; // Invalid client or not connected

    if (!SDLNet_WriteToStreamSocket(clients[clientIndex].socket, buffer, length))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Server] Send failed (Client %d): %s.", clientIndex, SDL_GetError());
        disconnectClient(clientIndex, "Send error"); // Forward declaration needed
        return false;
    }
    return true;
}

// --- Public API Function Implementations ---

// Doxygen should be in net_server.h
SDL_AppResult init_server()
{
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Server] Initializing...");
    memset(clients, 0, sizeof(clients)); // Initialize client slots

    // --- Create Server Socket ---
    serverSocket = SDLNet_CreateServer(NULL, SERVER_PORT);
    if (serverSocket == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Server] SDLNet_CreateServer failed on port %d: %s", SERVER_PORT, SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Server] Listening on port %d", SERVER_PORT);

    // --- Create Server Entity ---
    Entity server_e = {
        .name = "net_server",
        .update = update,
        .cleanup = cleanup,
        .handle_events = NULL,
        .render = NULL};

    if (create_entity(server_e) == SDL_APP_FAILURE)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Server] Failed to create server entity.");
        // Clean up socket if entity creation fails
        SDLNet_DestroyServer(serverSocket);
        serverSocket = NULL;
        return SDL_APP_FAILURE;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Server] Network server entity initialized.");
    return SDL_APP_SUCCESS;
}

int get_active_client_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i].active)
        {
            count++;
        }
    }
    return count;
}

void broadcast_buffer(const void *buffer, int length, int excludeClientIndex)
{
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        // Send if client is active and not the excluded one
        if (clients[i].active && i != excludeClientIndex)
        {
            send_buffer_to_client(i, buffer, length);
        }
    }
}

static void broadcast_building_destroyed(MessageType msg_type, int buildingIndex, int clientIndex)
{
    int broadcast_msg[2];
    broadcast_msg[0] = (int)msg_type;
    broadcast_msg[1] = buildingIndex;

    broadcast_buffer(broadcast_msg, sizeof(broadcast_msg), clientIndex);
}