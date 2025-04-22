#include "../include/net_client.h"

// Static Variables
static SDLNet_Address *serverAddress = NULL;
static SDLNet_StreamSocket *serverConnection = NULL;
static bool isConnected = false;
RemotePlayer remotePlayers[MAX_CLIENTS]; // Holds data for other players

// Forward Declarations for Static Helper Functions
static SDL_AppResult handle_connection_attempt(void);
static void process_server_message(char *buffer, int bytesReceived);
static SDL_AppResult receive_server_data(void);
static SDL_AppResult send_client_data(void);

static void cleanup()
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
    SDL_Log("Client network resources cleaned up.");
    isConnected = false;
    memset(remotePlayers, 0, sizeof(remotePlayers)); // Clear remote player data
}

static void update(float delta_time)
{
    (void)delta_time;

    // Only attempt connection process if not yet connected
    if (!isConnected)
    {
        // handle_connection_attempt logs errors internally and calls cleanup on failure
        if (handle_connection_attempt() == SDL_APP_FAILURE)
        {
            // No need to return failure from update
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[%s] Connection attempt failed, client inactive.", __func__);
            return; // Stop processing for this frame if connection failed
        }
        // If handle_connection_attempt succeeded and we are now connected, proceed
        if (!isConnected)
        {
            // Still connecting asynchronously, wait for next frame
            return;
        }
    }

    // If connected
    if (isConnected && serverConnection != NULL)
    {
        // Receive data from server. Handles disconnect on error internally.
        if (receive_server_data() == SDL_APP_FAILURE)
        {
            // Error logged in receive_server_data, cleanup already called.
            // isConnected will be false now.
            return; // Stop processing for this frame
        }

        // Check connection again in case receive_server_data disconnected us
        if (isConnected && serverConnection != NULL)
        {
            // Send data to server. Handles disconnect on error internally.
            if (send_client_data() == SDL_APP_FAILURE)
            {
                // Error logged in send_client_data, cleanup already called.
                // isConnected will be false now.
                return; // Stop processing for this frame
            }
        }
    }
}

SDL_AppResult init_client()
{
    isConnected = false;
    memset(remotePlayers, 0, sizeof(remotePlayers));

    // Resolve hostname asynchronously
    serverAddress = SDLNet_ResolveHostname(HOSTNAME);
    if (serverAddress == NULL)
    {
        // Immediate failure
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] SDLNet_ResolveHostname failed immediately: %s", __func__, SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Log("Attempting to resolve hostname: %s", HOSTNAME);

    Entity client_e = {
        .name = "net_client",
        .update = update,
        .cleanup = cleanup};

    // Check result of create_entity
    if (create_entity(client_e) == SDL_APP_FAILURE)
    {
        SDLNet_UnrefAddress(serverAddress); // Clean up address if entity creation failed
        serverAddress = NULL;
        return SDL_APP_FAILURE;
    }
    return SDL_APP_SUCCESS;
}

// Handles the logic for resolving the address and checking connection status
static SDL_AppResult handle_connection_attempt(void)
{
    // Check address resolution status first
    if (serverAddress != NULL && serverConnection == NULL)
    {
        int address_status = SDLNet_GetAddressStatus(serverAddress);
        if (address_status == 1) // Address resolved, attempt connection
        {
            SDL_Log("Hostname resolved. Attempting to connect...");
            serverConnection = SDLNet_CreateClient(serverAddress, SERVER_PORT);
            if (serverConnection == NULL) // Immediate connection failure
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] SDLNet_CreateClient failed: %s", __func__, SDL_GetError());
                cleanup();
                return SDL_APP_FAILURE;
            }
            // else: Connection attempt initiated, status will be checked below
        }
        else if (address_status == -1) // Resolution failed asynchronously
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] SDLNet_ResolveHostname failed asynchronously: %s", __func__, SDL_GetError());
            cleanup();
            return SDL_APP_FAILURE;
        }
        // else (address_status == 0): Still resolving, wait.
    }

    // Check connection status if we have initiated a connection attempt
    if (serverConnection != NULL && !isConnected)
    {
        int connection_status = SDLNet_GetConnectionStatus(serverConnection);
        if (connection_status == 1) // Connected successfully
        {
            isConnected = true;
            SDL_Log("Connected to server!");

            // Send initial hello message
            uint8_t msg_type = MSG_TYPE_C_HELLO;
            if (!SDLNet_WriteToStreamSocket(serverConnection, &msg_type, sizeof(msg_type)))
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Initial C_HELLO send failed: %s. Disconnecting.", __func__, SDL_GetError());
                cleanup();
                return SDL_APP_FAILURE;
            }
        }
        else if (connection_status == -1) // Connection failed asynchronously
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] SDLNet_GetConnectionStatus failed: %s", __func__, SDL_GetError());
            cleanup();
            return SDL_APP_FAILURE;
        }
        // else (connection_status == 0): Still connecting, wait.
    }
    return SDL_APP_SUCCESS;
}

// Processes a single message received from the server
static void process_server_message(char *buffer, int bytesReceived)
{
    if (buffer == NULL || bytesReceived <= 0)
        return;

    // Check if received data is enough for at least the message type
    if (bytesReceived < (int)sizeof(uint8_t))
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[%s] Rcvd incomplete message (< type byte) from server", __func__);
        return;
    }

    uint8_t msg_type_byte = buffer[0];
    switch (msg_type_byte)
    {
    case MSG_TYPE_S_WELCOME:
        SDL_Log("Received S_WELCOME from server");
        break;

    case MSG_TYPE_PLAYER_POS:
        // Check if received data is enough for type + update struct
        if (bytesReceived >= (int)(1 + sizeof(PlayerPosUpdateData)))
        {
            PlayerPosUpdateData update_data;
            memcpy(&update_data, buffer + 1, sizeof(PlayerPosUpdateData));
            uint8_t remote_client_id = update_data.client_id;

            // Validate client ID before accessing array
            if (remote_client_id < MAX_CLIENTS)
            {
                remotePlayers[remote_client_id].position = update_data.position;
                remotePlayers[remote_client_id].active = true; // Mark as active
            }
            else
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[%s] Rcvd POS update with invalid client ID: %u", __func__, (unsigned int)remote_client_id);
            }
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[%s] Rcvd incomplete PLAYER_POS message from server (%d bytes)", __func__, bytesReceived);
        }
        break;

    default:
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[%s] Rcvd unknown message type (%u) from server", __func__, (unsigned int)msg_type_byte);
        break;
    }
}

// Handles receiving all available data from the server in a non-blocking way
static SDL_AppResult receive_server_data(void)
{
    if (!isConnected || serverConnection == NULL)
        return SDL_APP_FAILURE;

    while (true)
    {
        char buffer[BUFFER_SIZE];
        int bytesReceived = SDLNet_ReadFromStreamSocket(serverConnection, buffer, sizeof(buffer));

        if (bytesReceived > 0)
        {
            process_server_message(buffer, bytesReceived);
            // If we read less than the buffer size, assume no more data for now
            if (bytesReceived < (int)sizeof(buffer))
            {
                break; // Exit loop for this frame
            }
            // Else, loop again immediately
        }
        else if (bytesReceived == 0) // No data available right now
        {
            break; // Exit loop for this frame
        }
        else // bytesReceived < 0 indicates error/disconnect
        {
            const char *sdlError = SDL_GetError();
            if (strcmp(sdlError, "Socket is not connected") != 0 && strcmp(sdlError, "Connection reset by peer") != 0)
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] SDLNet_ReadFromStreamSocket failed: %s. Disconnected.", __func__, sdlError);
            }
            else
            {
                SDL_Log("Disconnected from server.");
            }
            cleanup();              // Disconnect
            return SDL_APP_FAILURE; // Signal failure
        }
    }
    return SDL_APP_SUCCESS; // Success for this frame
}

// Handles sending client data
static SDL_AppResult send_client_data(void)
{
    if (!isConnected || serverConnection == NULL)
        return SDL_APP_FAILURE;

    // Prepare buffer: 1 byte for type + size of position data
    uint8_t send_buffer[1 + sizeof(SDL_FPoint)];
    send_buffer[0] = MSG_TYPE_PLAYER_POS;
    extern SDL_FPoint player_position; // Access the global player position
    memcpy(send_buffer + 1, &player_position, sizeof(SDL_FPoint));

    // Send position update
    if (!SDLNet_WriteToStreamSocket(serverConnection, send_buffer, sizeof(send_buffer)))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] SDLNet_WriteToStreamSocket (position) failed: %s. Disconnecting.", __func__, SDL_GetError());
        cleanup();              // Disconnect on failure
        return SDL_APP_FAILURE; // Signal failure
    }

    return SDL_APP_SUCCESS;
}