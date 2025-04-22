#include "../include/net_client.h"

// --- Static Variables ---
static SDLNet_Address *serverAddress = NULL;
static SDLNet_StreamSocket *serverConnection = NULL;
static bool isConnected = false;
RemotePlayer remotePlayers[MAX_CLIENTS];

// --- Forward Declarations for Static Helper Functions ---
static SDL_AppResult handle_connection_attempt(void);
static void process_server_message(char *buffer, int bytesReceived);
static SDL_AppResult receive_server_data(void);
static SDL_AppResult send_client_data(void);

// --- Entity Functions ---

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
    memset(remotePlayers, 0, sizeof(remotePlayers));
}

static void update(float delta_time)
{
    (void)delta_time;

    // Only attempt connection process if not yet connected
    if (!isConnected)
    {
        if (handle_connection_attempt() == SDL_APP_FAILURE)
        {
            return;
        }
    }

    // Handle active connection I/O only if we are connected
    if (isConnected && serverConnection != NULL)
    {
        if (receive_server_data() == SDL_APP_FAILURE)
        {
            return;
        }

        // Check connection again in case receive_server_data disconnected us
        if (isConnected && serverConnection != NULL)
        {
            if (send_client_data() == SDL_APP_FAILURE)
            {
                return;
            }
        }
    }
}

SDL_AppResult init_client()
{
    isConnected = false;
    memset(remotePlayers, 0, sizeof(remotePlayers));

    serverAddress = SDLNet_ResolveHostname(HOSTNAME);
    if (serverAddress == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDLNet_ResolveHostname failed immediately: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Log("Attempting to resolve hostname: %s", HOSTNAME);

    Entity client_e = {
        .name = "net_client",
        .update = update,
        .cleanup = cleanup};
    create_entity(client_e);
    return SDL_APP_SUCCESS;
}

// --- Static Helper Functions ---

// Handles the logic for resolving the address and checking connection status
static SDL_AppResult handle_connection_attempt(void)
{
    // Check address resolution and attempt connection if address exists but socket doesn't
    if (serverAddress != NULL && serverConnection == NULL)
    {
        int address_status = SDLNet_GetAddressStatus(serverAddress);
        if (address_status == 1) // Address resolved, attempt connection
        {
            serverConnection = SDLNet_CreateClient(serverAddress, SERVER_PORT);
            if (serverConnection == NULL) // Immediate failure
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDLNet_CreateClient failed: %s", SDL_GetError());
                SDLNet_UnrefAddress(serverAddress);
                serverAddress = NULL;
                return SDL_APP_FAILURE;
            }
            else
            {
                SDL_Log("Connection attempt initiated to %s:%d.", SDLNet_GetAddressString(serverAddress), SERVER_PORT);
            }
        }
        else if (address_status == -1) // Resolution failed asynchronously
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDLNet_ResolveHostname failed asynchronously: %s", SDL_GetError());
            SDLNet_UnrefAddress(serverAddress);
            serverAddress = NULL;
            return SDL_APP_FAILURE;
        }
    }

    // Check connection status if we have a socket but aren't marked as connected
    if (serverConnection != NULL && !isConnected)
    {
        int connection_status = SDLNet_GetConnectionStatus(serverConnection);
        if (connection_status == 1) // Connected successfully
        {
            isConnected = true;
            SDL_Log("Connected to server!");
            uint8_t msg_type = MSG_TYPE_C_HELLO;
            if (!SDLNet_WriteToStreamSocket(serverConnection, &msg_type, sizeof(msg_type)))
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Initial C_HELLO send failed: %s. Disconnecting.", SDL_GetError());
                cleanup();
                return SDL_APP_FAILURE;
            }
        }
        else if (connection_status == -1) // Connection failed
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDLNet_GetConnectionStatus failed: %s", SDL_GetError());
            cleanup();
            return SDL_APP_FAILURE;
        }
    }

    return SDL_APP_SUCCESS;
}

// Processes a single message received from the server
static void process_server_message(char *buffer, int bytesReceived)
{
    if (bytesReceived < (int)sizeof(uint8_t))
        return;

    uint8_t msg_type_byte = buffer[0];
    switch (msg_type_byte)
    {
    case MSG_TYPE_S_WELCOME:
        SDL_Log("Received S_WELCOME from server");
        break;

    case MSG_TYPE_PLAYER_POS:
        if (bytesReceived >= (int)(1 + sizeof(PlayerPosUpdateData)))
        {
            PlayerPosUpdateData update_data;
            memcpy(&update_data, buffer + 1, sizeof(PlayerPosUpdateData));
            uint8_t remote_client_id = update_data.client_id;

            if (remote_client_id < MAX_CLIENTS)
            {
                remotePlayers[remote_client_id].position = update_data.position;
                remotePlayers[remote_client_id].active = true;
            }
            else
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Rcvd POS update invalid client ID: %d", remote_client_id);
            }
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Rcvd incomplete PLAYER_POS message from server (%d bytes)", bytesReceived);
        }
        break;

    default:
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Rcvd unknown message type (%d) from server", msg_type_byte);
        break;
    }
}

// Handles receiving all available data from the server in a non-blocking way
static SDL_AppResult receive_server_data(void)
{
    while (true)
    {
        char buffer[BUFFER_SIZE];
        int bytesReceived = SDLNet_ReadFromStreamSocket(serverConnection, buffer, sizeof(buffer));

        if (bytesReceived > 0)
        {
            process_server_message(buffer, bytesReceived);
            if (bytesReceived < (int)sizeof(buffer))
            {
                break;
            }
        }
        else if (bytesReceived == 0)
        {
            break;
        }
        else // bytesReceived < 0
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDLNet_ReadFromStreamSocket failed: %s. Disconnected.", SDL_GetError());
            cleanup();
            return SDL_APP_FAILURE;
        }
    }
    return SDL_APP_SUCCESS;
}

// Handles sending client data
static SDL_AppResult send_client_data(void)
{
    uint8_t send_buffer[1 + sizeof(SDL_FPoint)];
    send_buffer[0] = MSG_TYPE_PLAYER_POS;
    extern SDL_FPoint player_position; // Make sure player_position is accessible
    memcpy(send_buffer + 1, &player_position, sizeof(SDL_FPoint));

    if (!SDLNet_WriteToStreamSocket(serverConnection, send_buffer, sizeof(send_buffer)))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDLNet_WriteToStreamSocket (position) failed: %s. Disconnecting.", SDL_GetError());
        cleanup();
        return SDL_APP_FAILURE;
    }
    return SDL_APP_SUCCESS;
}