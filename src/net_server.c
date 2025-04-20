#include "../include/net_server.h"

typedef struct
{
    SDLNet_StreamSocket *socket;
    SDL_FPoint position;
    bool active;
} ClientInfo;

static SDLNet_Server *serverSocket = NULL;
static ClientInfo clients[MAX_CLIENTS];

static void cleanup()
{
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i].active && clients[i].socket != NULL)
        {
            SDLNet_DestroyStreamSocket(clients[i].socket);
        }
    }
    memset(clients, 0, sizeof(clients));
    if (serverSocket != NULL)
    {
        SDLNet_DestroyServer(serverSocket);
        serverSocket = NULL;
    }
    SDL_Log("Server network resources cleaned up.");
}

static void disconnectClient(int clientIndex)
{
    if (clientIndex < 0 || clientIndex >= MAX_CLIENTS || !clients[clientIndex].active)
        return;
    SDL_Log("Disconnecting client %d.", clientIndex);
    if (clients[clientIndex].socket != NULL)
    {
        SDLNet_DestroyStreamSocket(clients[clientIndex].socket);
        clients[clientIndex].socket = NULL;
    }
    clients[clientIndex].active = false;
}

static void acceptConnections()
{
    if (serverSocket == NULL)
        return;
    SDLNet_StreamSocket *newClientSocket = NULL;
    bool success = SDLNet_AcceptClient(serverSocket, &newClientSocket);
    if (!success)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "AcceptClient failed: %s", SDL_GetError());
        return;
    }
    if (newClientSocket != NULL)
    {
        int clientSlot = -1;
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (!clients[i].active)
            {
                clientSlot = i;
                break;
            }
        }
        if (clientSlot != -1)
        {
            clients[clientSlot].socket = newClientSocket;
            clients[clientSlot].active = true;
            clients[clientSlot].position = (SDL_FPoint){0.0f, 0.0f};
            SDL_Log("Client connected (Slot %d)", clientSlot);
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Server full. Rejecting new client.");
            SDLNet_DestroyStreamSocket(newClientSocket);
        }
    }
}

static void handleClients()
{
    char buffer[BUFFER_SIZE];
    int bytesReceived;
    bool sentSuccess;

    for (int i = 0; i < MAX_CLIENTS; ++i) // Handle incoming data from client i
    {
        if (!clients[i].active || clients[i].socket == NULL)
            continue;

        bytesReceived = SDLNet_ReadFromStreamSocket(clients[i].socket, buffer, sizeof(buffer));

        if (bytesReceived > 0)
        {
            if (bytesReceived >= sizeof(uint8_t))
            {
                uint8_t msg_type_byte = buffer[0];
                switch (msg_type_byte)
                {
                case MSG_TYPE_C_HELLO:
                    SDL_Log("Received C_HELLO from client %d", i);
                    uint8_t reply_type = MSG_TYPE_S_WELCOME;
                    sentSuccess = SDLNet_WriteToStreamSocket(clients[i].socket, &reply_type, sizeof(reply_type));
                    if (!sentSuccess)
                    {
                        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Write S_WELCOME failed (Client %d): %s.", i, SDL_GetError());
                        disconnectClient(i);
                    }
                    break;

                case MSG_TYPE_PLAYER_POS:
                    // C->S message is Type + SDL_FPoint
                    if (bytesReceived >= (1 + sizeof(SDL_FPoint)))
                    {
                        SDL_FPoint received_pos;
                        memcpy(&received_pos, buffer + 1, sizeof(SDL_FPoint));
                        clients[i].position = received_pos; // Store position

                        // S->C message is Type + PlayerPosUpdateData (ID + SDL_FPoint)
                        uint8_t broadcast_buffer[1 + sizeof(PlayerPosUpdateData)];
                        broadcast_buffer[0] = MSG_TYPE_PLAYER_POS; // Use same type for S->C update

                        PlayerPosUpdateData update_data;
                        update_data.client_id = (uint8_t)i;         // ID of the player who moved
                        update_data.position = clients[i].position; // Their new position

                        memcpy(broadcast_buffer + 1, &update_data, sizeof(PlayerPosUpdateData));

                        // Loop through all possible recipients
                        for (int j = 0; j < MAX_CLIENTS; ++j)
                        {
                            // Send to everyone EXCEPT the original sender and inactive slots
                            if (i == j || !clients[j].active || clients[j].socket == NULL)
                            {
                                continue;
                            }

                            sentSuccess = SDLNet_WriteToStreamSocket(clients[j].socket, broadcast_buffer, sizeof(broadcast_buffer));
                            if (!sentSuccess)
                            {
                                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Broadcast POS failed (Sender %d, Recipient %d): %s.", i, j, SDL_GetError());
                                disconnectClient(j); // Disconnect recipient if send failed
                            }
                        }
                    }
                    else
                    {
                        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Received incomplete PLAYER_POS message from client %d", i);
                    }
                    break;

                default:
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Received unknown message type (%d) from client %d", msg_type_byte, i);
                    break;
                }
            }
            else
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Received incomplete message (0 bytes?) from client %d", i);
            }
        }
        else if (bytesReceived < 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Read failed (Client %d): %s.", i, SDL_GetError());
            disconnectClient(i);
        }
    }
}

static void update(float delta_time)
{
    acceptConnections();
    handleClients();
}

SDL_AppResult init_server()
{
    memset(clients, 0, sizeof(clients));
    serverSocket = SDLNet_CreateServer(NULL, SERVER_PORT);
    if (serverSocket == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDLNet_CreateServer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_Log("Server listening on port %d.", SERVER_PORT);
    Entity server_e = {.name = "net_server", .update = update, .cleanup = cleanup};
    create_entity(server_e);
    return SDL_APP_SUCCESS;
}
