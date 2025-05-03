#include "../include/net_client.h"

/**
 * @brief Represents the different network connection states of the client.
 */
typedef enum
{
    CLIENT_STATE_DISCONNECTED,
    CLIENT_STATE_RESOLVING,
    CLIENT_STATE_CONNECTING,
    CLIENT_STATE_CONNECTED
} ClientNetworkState;

// --- Module Variables ---
static SDLNet_Address *serverAddress = NULL;
static SDLNet_StreamSocket *serverConnection = NULL;
static ClientNetworkState networkState = CLIENT_STATE_DISCONNECTED;
static int myClientIndex = -1;
static Uint64 last_state_send_time = 0;
const Uint32 STATE_UPDATE_INTERVAL_MS = 50; // Send state ~20 times/sec
static bool client_team;
RemotePlayer remotePlayers[MAX_CLIENTS];

// --- Static Helper Functions ---

/**
 * @brief Cleans up all network resources (socket, address) and resets state.
 * @return void
 */
static void cleanup(void)
{
    if (serverConnection != NULL)
    {
        SDLNet_DestroyStreamSocket(serverConnection);
        serverConnection = NULL;
    }
    if (serverAddress != NULL)
    {
        SDLNet_UnrefAddress(serverAddress);
        serverAddress = NULL;
    }
    networkState = CLIENT_STATE_DISCONNECTED;
    myClientIndex = -1;
    memset(remotePlayers, 0, sizeof(remotePlayers));
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Network resources cleaned up.");
}

/**
 * @brief Sends a data buffer to the server via the established connection.
 * Cleans up and disconnects if the send fails.
 * @param buffer Pointer to the data buffer to send.
 * @param length The number of bytes to send from the buffer.
 * @return true if the send was successful, false otherwise (and triggers cleanup).
 */
static bool send_buffer(const void *buffer, int length)
{
    if (networkState != CLIENT_STATE_CONNECTED || serverConnection == NULL)
        return false;
    if (!SDLNet_WriteToStreamSocket(serverConnection, buffer, length))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] Send failed: %s. Disconnecting.", SDL_GetError());
        cleanup(); // Disconnect on send failure
        return false;
    }
    return true;
}

/**
 * @brief Attempts to read data from the server stream socket.
 * @param buffer The buffer to store received data.
 * @param bufferSize The maximum number of bytes to read into the buffer.
 * @return The number of bytes received (0 if non-blocking and no data), -1 on error or disconnect.
 */
static int read_from_server(char *buffer, int bufferSize)
{
    if (networkState != CLIENT_STATE_CONNECTED || serverConnection == NULL)
        return -1;
    SDL_ClearError();
    // Note: Assuming non-blocking based on typical game loop usage
    return SDLNet_ReadFromStreamSocket(serverConnection, buffer, bufferSize);
}

/**
 * @brief Initiates asynchronous hostname resolution if currently disconnected.
 * @return void
 */
static void attempt_resolve(void)
{
    if (networkState != CLIENT_STATE_DISCONNECTED)
        return;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Attempting to resolve hostname: %s", HOSTNAME);
    serverAddress = SDLNet_ResolveHostname(HOSTNAME);
    if (serverAddress == NULL)
    {
        // Immediate failure
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] SDLNet_ResolveHostname failed immediately: %s", SDL_GetError());
    }
    else
    {
        networkState = CLIENT_STATE_RESOLVING;
    }
}

/**
 * @brief Attempts to create a client socket connection to the resolved server address.
 * Only proceeds if hostname resolution succeeded and state is ready for connection.
 * @return void
 */
static void attempt_connection(void)
{
    if (serverAddress == NULL || networkState != CLIENT_STATE_DISCONNECTED)
        return;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Attempting connection to port %d...", SERVER_PORT);
    serverConnection = SDLNet_CreateClient(serverAddress, SERVER_PORT);
    if (serverConnection == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] SDLNet_CreateClient failed: %s", SDL_GetError());
        cleanup(); // Cleanup if connection fails immediately
    }
    else
    {
        networkState = CLIENT_STATE_CONNECTING;
    }
}

/**
 * @brief Checks the status of an ongoing asynchronous hostname resolution.
 * If resolved, proceeds to attempt connection. If failed, cleans up.
 * @return void
 */
static void check_resolve_status(void)
{
    if (serverAddress == NULL || networkState != CLIENT_STATE_RESOLVING)
        return;
    int status = SDLNet_GetAddressStatus(serverAddress);
    if (status == 1) // Resolved successfully
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Hostname resolved.");
        networkState = CLIENT_STATE_DISCONNECTED; // State reset before attempting connection
        attempt_connection();                     // Proceed to connect
    }
    else if (status == -1) // Resolution failed
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] SDLNet_ResolveHostname failed async: %s", SDL_GetError());
        cleanup(); // Cleanup on failure
    }
    // status == 0 means still resolving, do nothing
}

/**
 * @brief Checks the status of an ongoing asynchronous connection attempt.
 * If connected, sends initial C_HELLO message. If failed, cleans up.
 * @return void
 */
static void check_connection_status(void)
{
    if (serverConnection == NULL || networkState != CLIENT_STATE_CONNECTING)
        return;
    int status = SDLNet_GetConnectionStatus(serverConnection);
    if (status == 1) // Connected successfully
    {
        networkState = CLIENT_STATE_CONNECTED;
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Connected to server!");
        // --- Send Hello ---
        uint8_t msg_type = MSG_TYPE_C_HELLO;
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Sending C_HELLO.");
        if (!send_buffer(&msg_type, sizeof(msg_type)))
            return; // send_buffer handles cleanup on failure
        last_state_send_time = SDL_GetTicks();
    }
    else if (status == -1) // Connection failed
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] Connection failed: %s", SDL_GetError());
        cleanup(); // Cleanup on failure
    }
    // status == 0 means still connecting, do nothing
}

/**
 * @brief Handles the S_WELCOME message from the server, storing the assigned client index
 * and initializing player and camera entities.
 * @param state The application state.
 * @param receivedIndex The client index assigned by the server.
 * @return void
 */
static void handle_server_welcome(AppState *state, int receivedIndex)
{
    if (myClientIndex != -1)
    {
        // Avoid processing duplicate welcome messages
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Received duplicate S_WELCOME (myIndex already %d). Ignoring.", myClientIndex);
        return;
    }
    myClientIndex = receivedIndex;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Received S_WELCOME, assigned myClientIndex = %d", myClientIndex);

    // --- Initialize Local Player and Camera ---
    if (!init_player(state->renderer, myClientIndex, client_team))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] Failed init_player after WELCOME.");
        cleanup(); // Network cleanup
        return;    // Avoid further processing
    }
    if (init_camera(state->renderer) == SDL_APP_FAILURE)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] Failed init_camera after WELCOME.");
        // Attempt player cleanup before network cleanup
        int p_idx = find_entity("player");
        if (p_idx != -1 && entities[p_idx].cleanup)
            entities[p_idx].cleanup();
        cleanup(); // Network cleanup
        return;    // Avoid further processing
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Player and Camera initialized.");
}

/**
 * @brief Updates the state of a remote player based on received data.
 * @param data Pointer to the PlayerStateData received from the server.
 * @return void
 */
static void handle_remote_player_state_update(const PlayerStateData *data)
{
    if (!data)
        return;
    uint8_t remote_id = data->client_id;

    // Ignore updates for self or invalid IDs
    if (remote_id == myClientIndex || remote_id >= MAX_CLIENTS)
        return;

    if (!remotePlayers[remote_id].active)
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Received first state update for remote player %u.", (unsigned int)remote_id);
        remotePlayers[remote_id].active = true;
    }
    remotePlayers[remote_id].position = data->position;
    remotePlayers[remote_id].sprite_portion = data->sprite_portion;
    remotePlayers[remote_id].flip_mode = data->flip_mode;
    remotePlayers[remote_id].team = data->team;
}

/**
 * @brief Processes a single message received from the server based on its type byte.
 * @param buffer Pointer to the received data buffer.
 * @param bytesReceived The number of bytes in the buffer.
 * @param state The application state.
 * @return void
 */
static void process_server_message(char *buffer, int bytesReceived, AppState *state)
{
    if (bytesReceived < (int)sizeof(uint8_t))
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Rcvd incomplete message (< type byte)");
        return;
    }
    uint8_t msg_type_byte;
    memcpy(&msg_type_byte, buffer, sizeof(uint8_t)); // Read the type byte

    switch ((MessageType)msg_type_byte)
    {
    case MSG_TYPE_S_WELCOME:
        // --- Handle Welcome ---
        if (bytesReceived >= (int)(sizeof(int) * 2)) // Check size (using sizeof(int) placeholder as per original)
        {
            int idx;
            memcpy(&idx, buffer + sizeof(int), sizeof(int)); // Read index
            handle_server_welcome(state, idx);
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Rcvd incomplete S_WELCOME (%d bytes, needed %u)", bytesReceived, (unsigned int)(sizeof(int) * 2));
        }
        break;

    case MSG_TYPE_PLAYER_STATE:
        // --- Handle Player State Update ---
        if (bytesReceived >= (int)(sizeof(uint8_t) + sizeof(PlayerStateData)))
        {
            PlayerStateData state_data;
            memcpy(&state_data, buffer + sizeof(uint8_t), sizeof(PlayerStateData)); // Read state data
            handle_remote_player_state_update(&state_data);
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Rcvd incomplete PLAYER_STATE msg (%d bytes, needed %u)", bytesReceived, (unsigned int)(sizeof(uint8_t) + sizeof(PlayerStateData)));
        }
        break;

    default:
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Client] Rcvd unknown message type (%u) from server", (unsigned int)msg_type_byte);
        break;
    }
}

/**
 * @brief Reads all available data from the server socket and processes messages.
 * Handles disconnection if read fails.
 * @param state The application state.
 * @return SDL_APP_SUCCESS if processing completed without disconnect, SDL_APP_FAILURE on disconnect/error.
 */
static SDL_AppResult receive_server_data(AppState *state)
{
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    // Read messages in a loop until no more data or buffer is full
    while ((bytesReceived = read_from_server(buffer, sizeof(buffer))) > 0)
    {
        process_server_message(buffer, bytesReceived, state);
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
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] Read error: %s. Disconnecting.", sdlError);
        }
        else
        {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Disconnected from server.");
        }
        cleanup(); // Ensure cleanup happens on disconnect
        return SDL_APP_FAILURE;
    }

    return SDL_APP_SUCCESS; // Still connected
}

/**
 * @brief Handles receiving data and sending periodic state updates when connected.
 * @param state The application state.
 * @return void
 */
static void handle_server_communication(AppState *state)
{
    if (networkState != CLIENT_STATE_CONNECTED || serverConnection == NULL)
        return;

    // --- Receive Data ---
    if (receive_server_data(state) == SDL_APP_FAILURE)
        return; // Disconnected during receive

    // --- Send State Update (Throttled) ---
    Uint64 current_time = SDL_GetTicks();
    if (current_time > last_state_send_time + STATE_UPDATE_INTERVAL_MS)
    {
        send_local_player_state(); // Function defined below in Public API section
        last_state_send_time = current_time;
    }
}

/**
 * @brief Main update function for the network client entity, run each frame.
 * Manages the state machine for connection and communication.
 * @param state The application state.
 * @return void
 */
static void update(AppState *state)
{
    switch (networkState)
    {
    case CLIENT_STATE_DISCONNECTED:
        attempt_resolve();
        break;
    case CLIENT_STATE_RESOLVING:
        check_resolve_status();
        break;
    case CLIENT_STATE_CONNECTING:
        check_connection_status();
        break;
    case CLIENT_STATE_CONNECTED:
        handle_server_communication(state);
        break;
    }
}

// --- Public API Function Implementations ---

SDL_AppResult init_client(bool team_arg)
{
    cleanup(); // Ensure clean state before init
    networkState = CLIENT_STATE_DISCONNECTED;
    memset(remotePlayers, 0, sizeof(remotePlayers));

    client_team = team_arg;

    Entity client_e = {
        .name = "net_client",
        .update = update,
        .cleanup = cleanup};

    if (create_entity(client_e) == SDL_APP_FAILURE)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Client] Failed create net_client entity.");
        return SDL_APP_FAILURE;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Client] Network client entity initialized.");
    return SDL_APP_SUCCESS;
}

int get_my_client_index(void)
{
    return myClientIndex;
}

bool is_client_connected(void)
{
    return networkState == CLIENT_STATE_CONNECTED;
}

void send_local_player_state(void)
{
    // Check connection and assigned index
    if (networkState != CLIENT_STATE_CONNECTED || myClientIndex < 0)
        return;

    PlayerStateData data;
    // Retrieve local player state
    if (!get_local_player_state_for_network(&data))
    {
        return;
    }

    // Prepare message buffer
    uint8_t buffer[sizeof(uint8_t) + sizeof(PlayerStateData)];
    buffer[0] = (uint8_t)MSG_TYPE_PLAYER_STATE;                       // Set message type
    memcpy(buffer + sizeof(uint8_t), &data, sizeof(PlayerStateData)); // Copy payload

    // Send the buffer
    send_buffer(buffer, sizeof(buffer));
}