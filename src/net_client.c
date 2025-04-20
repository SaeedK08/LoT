#include "../include/net_client.h"

typedef struct
{
    SDL_FPoint position;
    bool active;
} RemotePlayer;

static SDLNet_Address *serverAddress = NULL;
static SDLNet_StreamSocket *serverConnection = NULL;
static bool isConnected = false;
static RemotePlayer remotePlayers[MAX_CLIENTS];

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
    // Check address resolution and attempt connection
    if (serverAddress != NULL && serverConnection == NULL)
    {
        int address_status = SDLNet_GetAddressStatus(serverAddress);
        if (address_status == 1)
        {
            serverConnection = SDLNet_CreateClient(serverAddress, SERVER_PORT);
            if (serverConnection == NULL)
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "CreateClient failed: %s", SDL_GetError());
                SDLNet_UnrefAddress(serverAddress);
                serverAddress = NULL;
            }
            else
            {
                SDL_Log("Connection attempt initiated.");
            }
        }
        else if (address_status == -1)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ResolveHostname failed: %s", SDL_GetError());
            SDLNet_UnrefAddress(serverAddress);
            serverAddress = NULL;
        }
    }

    // Check connection status if attempting
    if (serverConnection != NULL && !isConnected)
    {
        int connection_status = SDLNet_GetConnectionStatus(serverConnection);
        if (connection_status == 1)
        {
            isConnected = true;
            SDL_Log("Connected to server!");
            uint8_t msg_type = MSG_TYPE_C_HELLO;
            bool sentSuccess = SDLNet_WriteToStreamSocket(serverConnection, &msg_type, sizeof(msg_type));
        }
        else if (connection_status == -1)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "GetConnectionStatus failed: %s", SDL_GetError());
            SDLNet_DestroyStreamSocket(serverConnection);
            serverConnection = NULL;
            if (serverAddress != NULL)
            {
                SDLNet_UnrefAddress(serverAddress);
                serverAddress = NULL;
            }
        }
    }

    // Handle active connection (receiving and sending position)
    if (isConnected && serverConnection != NULL)
    {
        // --- Receive Data ---
        char buffer[BUFFER_SIZE];
        int bytesReceived;
        bytesReceived = SDLNet_ReadFromStreamSocket(serverConnection, buffer, sizeof(buffer));

        if (bytesReceived > 0)
        {
            if (bytesReceived >= sizeof(uint8_t))
            {
                uint8_t msg_type_byte = buffer[0];
                switch (msg_type_byte)
                {
                case MSG_TYPE_S_WELCOME:
                    SDL_Log("Received S_WELCOME from server");
                    break;

                case MSG_TYPE_PLAYER_POS:
                    // Expecting Type + PlayerPosUpdateData (ID + Pos)
                    if (bytesReceived >= (1 + sizeof(PlayerPosUpdateData)))
                    {
                        PlayerPosUpdateData update_data;
                        memcpy(&update_data, buffer + 1, sizeof(PlayerPosUpdateData));

                        uint8_t remote_client_id = update_data.client_id;
                        if (remote_client_id < MAX_CLIENTS)
                        { // Basic bounds check
                            // Store position and mark active
                            remotePlayers[remote_client_id].position = update_data.position;
                            remotePlayers[remote_client_id].active = true;
                            // SDL_Log("Stored position for player %d", remote_client_id);
                        }
                        else
                        {
                            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Rcvd POS update invalid client ID: %d", remote_client_id);
                        }
                    }
                    else
                    {
                        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Rcvd incomplete PLAYER_POS message from server");
                    }
                    break;

                default:
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Rcvd unknown message type (%d) from server", msg_type_byte);
                    break;
                }
            }
        }
        else if (bytesReceived < 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ReadStreamSocket failed: %s. Disconnected.", SDL_GetError());
            SDLNet_DestroyStreamSocket(serverConnection);
            serverConnection = NULL;
            isConnected = false;
            return; // Skip sending if read failed
        }

        // --- Send Own Position Data ---
        uint8_t send_buffer[1 + sizeof(SDL_FPoint)];
        send_buffer[0] = MSG_TYPE_PLAYER_POS;
        memcpy(send_buffer + 1, &player_position, sizeof(SDL_FPoint));
        bool sentSuccess = SDLNet_WriteToStreamSocket(serverConnection, send_buffer, sizeof(send_buffer));
        if (!sentSuccess)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Send position failed: %s. Disconnecting.", SDL_GetError());
            SDLNet_DestroyStreamSocket(serverConnection);
            serverConnection = NULL;
            isConnected = false;
        }
    }
}

SDL_AppResult init_client()
{
    isConnected = false;
    memset(remotePlayers, 0, sizeof(remotePlayers)); // Initialize remote player data

    serverAddress = SDLNet_ResolveHostname(HOSTNAME);
    if (serverAddress == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ResolveHostname failed immediately: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    Entity client_e = {.name = "net_client", .update = update, .cleanup = cleanup};
    create_entity(client_e);
    return SDL_APP_SUCCESS;
}
