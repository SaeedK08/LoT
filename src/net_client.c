#include "../include/net_client.h"

static SDLNet_Address *serverAddress = NULL;
static SDLNet_StreamSocket *serverConnection = NULL;
static bool isConnected = false;
RemotePlayer remotePlayers[MAX_CLIENTS];

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
            // Address resolved, attempt connection
            serverConnection = SDLNet_CreateClient(serverAddress, SERVER_PORT);
            if (serverConnection == NULL)
            {
                // Immediate failure
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDLNet_CreateClient failed: %s", SDL_GetError());
                SDLNet_UnrefAddress(serverAddress); // Connection won't proceed
                serverAddress = NULL;
            }
            else
            {
                SDL_Log("Connection attempt initiated to %s:%d.", SDLNet_GetAddressString(serverAddress), SERVER_PORT);
            }
        }
        else if (address_status == -1) // Resolution failed
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDLNet_ResolveHostname failed asynchronously: %s", SDL_GetError());
            SDLNet_UnrefAddress(serverAddress);
            serverAddress = NULL;
        }
    }

    // Check connection status if attempting
    if (serverConnection != NULL && !isConnected)
    {
        int connection_status = SDLNet_GetConnectionStatus(serverConnection);
        if (connection_status == 1) // Connected successfully
        {
            isConnected = true;
            SDL_Log("Connected to server!");
            uint8_t msg_type = MSG_TYPE_C_HELLO;
            bool sentSuccess = SDLNet_WriteToStreamSocket(serverConnection, &msg_type, sizeof(msg_type));
            if (!sentSuccess)
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Initial C_HELLO send failed: %s", SDL_GetError());
            }
        }
        else if (connection_status == -1) // Connection failed
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDLNet_GetConnectionStatus failed: %s", SDL_GetError());
            SDLNet_DestroyStreamSocket(serverConnection);
            serverConnection = NULL;
            if (serverAddress != NULL)
            {
                SDLNet_UnrefAddress(serverAddress);
                serverAddress = NULL;
            }
        }
    }

    // Handle active connection
    if (isConnected && serverConnection != NULL)
    {
        // --- Receive Data ---
        // Keep reading until no more data is immediately available in the buffer
        while (true)
        {
            char buffer[BUFFER_SIZE];
            int bytesReceived;
            bytesReceived = SDLNet_ReadFromStreamSocket(serverConnection, buffer, sizeof(buffer));

            if (bytesReceived > 0) // Got some data
            {
                if (bytesReceived >= sizeof(uint8_t)) // Need at least the message type byte
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
                            // Copy data after the type byte
                            memcpy(&update_data, buffer + 1, sizeof(PlayerPosUpdateData));

                            uint8_t remote_client_id = update_data.client_id;
                            // SDL_Log("Received position for player %d: (%.2f, %.2f)", remote_client_id, update_data.position.x, update_data.position.y);
                            if (remote_client_id < MAX_CLIENTS)
                            {
                                // Store position and mark active
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

                // If we read less than the buffer size, assume no more data for now
                if (bytesReceived < sizeof(buffer))
                {
                    break;
                }
            }
            else if (bytesReceived == 0) // No data available right now
            {
                break;
            }
            else // bytesReceived < 0 indicates an error/disconnect
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDLNet_ReadFromStreamSocket failed: %s. Disconnected.", SDL_GetError());
                SDLNet_DestroyStreamSocket(serverConnection);
                serverConnection = NULL;
                isConnected = false;
                memset(remotePlayers, 0, sizeof(remotePlayers)); // Clear them on disconnect
                return;                                          // Stop update processing for client on disconnect
            }
        }

        // Send own position data
        uint8_t send_buffer[1 + sizeof(SDL_FPoint)];
        send_buffer[0] = MSG_TYPE_PLAYER_POS;
        memcpy(send_buffer + 1, &player_position, sizeof(SDL_FPoint));
        bool sentSuccess = SDLNet_WriteToStreamSocket(serverConnection, send_buffer, sizeof(send_buffer));
        if (!sentSuccess)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDLNet_WriteToStreamSocket (position) failed: %s. Disconnecting.", SDL_GetError());
            SDLNet_DestroyStreamSocket(serverConnection);
            serverConnection = NULL;
            isConnected = false;
            memset(remotePlayers, 0, sizeof(remotePlayers)); // Clear remote players on disconnect
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