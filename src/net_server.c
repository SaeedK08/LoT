#include "../include/net_server.h"

#define MAX_CLIENTS 10
#define BUFFER_SIZE 512

static SDLNet_Server *serverSocket = NULL;
static SDLNet_StreamSocket *clientSockets[MAX_CLIENTS] = {NULL};

static void cleanup()
{
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clientSockets[i] != NULL)
        {
            SDLNet_DestroyStreamSocket(clientSockets[i]);
            clientSockets[i] = NULL;
        }
    }
    if (serverSocket != NULL)
    {
        SDLNet_DestroyServer(serverSocket);
        serverSocket = NULL;
    }
    SDL_Log("Server network resources cleaned up.");
}

static void acceptConnections()
{
    if (serverSocket == NULL)
        return;

    SDLNet_StreamSocket *newClient = NULL;
    bool success = SDLNet_AcceptClient(serverSocket, &newClient);

    if (!success)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "AcceptClient failed: %s", SDL_GetError());
        return;
    }

    if (newClient != NULL)
    {
        int clientSlot = -1;
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (clientSockets[i] == NULL)
            {
                clientSlot = i;
                break;
            }
        }
        if (clientSlot != -1)
        {
            clientSockets[clientSlot] = newClient;
            SDL_Log("Client connected (Slot %d)", clientSlot);
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Server full. Rejecting new client.");
            SDLNet_DestroyStreamSocket(newClient);
        }
    }
}

static void handleClients()
{
    char buffer[BUFFER_SIZE];
    int bytesReceived;
    bool sentSuccess;

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clientSockets[i] == NULL)
            continue;

        bytesReceived = SDLNet_ReadFromStreamSocket(clientSockets[i], buffer, sizeof(buffer));

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
                    SDL_Log("Sending S_WELCOME to client %d", i);
                    sentSuccess = SDLNet_WriteToStreamSocket(clientSockets[i], &reply_type, sizeof(reply_type));
                    if (!sentSuccess)
                    {
                        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Write failed (Client %d): %s. Disconnecting.", i, SDL_GetError());
                        SDLNet_DestroyStreamSocket(clientSockets[i]);
                        clientSockets[i] = NULL; // Free up the slot
                    }
                    break;

                default:
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Received unknown message type (%d) from client %d", msg_type_byte, i);
                    break;
                }
            }
            else
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Received incomplete message from client %d", i);
            }
        }
        else if (bytesReceived < 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Read failed (Client %d): %s. Disconnecting.", i, SDL_GetError());
            SDLNet_DestroyStreamSocket(clientSockets[i]);
            clientSockets[i] = NULL;
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
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        clientSockets[i] = NULL;
    }

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