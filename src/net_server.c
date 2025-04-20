#include "../include/net_server.h"

static SDLNet_Server *serverSocket = NULL;
static SDLNet_StreamSocket *clientSocket = NULL;

static void cleanup()
{
    if (clientSocket != NULL)
    {
        SDLNet_DestroyStreamSocket(clientSocket);
        clientSocket = NULL;
    }
    if (serverSocket != NULL)
    {
        SDLNet_DestroyServer(serverSocket);
        serverSocket = NULL;
    }
    SDLNet_Quit();
    SDL_Log("Server SDLNet cleaned up.");
}

static void update(float delta_time)
{
    // Accept connections
    if (serverSocket != NULL && clientSocket == NULL)
    {
        SDLNet_StreamSocket *newClient = NULL;
        bool success = SDLNet_AcceptClient(serverSocket, &newClient);
        if (!success)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "AcceptClient failed: %s", SDL_GetError());
        }
        else if (newClient != NULL)
        {
            clientSocket = newClient;
            SDL_Log("Client connected!");
        }
    }

    // Handle connected client
    if (clientSocket != NULL)
    {
        char buffer[512];
        int bytesReceived;

        // Receive data
        bytesReceived = SDLNet_ReadFromStreamSocket(clientSocket, buffer, sizeof(buffer) - 1);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            SDL_Log("Received %d bytes: %s", bytesReceived, buffer);

            // Send reply
            const char *reply = "Server received your message!";
            bool sentSuccess = SDLNet_WriteToStreamSocket(clientSocket, reply, SDL_strlen(reply));
            if (!sentSuccess)
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "WriteToStreamSocket failed: %s. Disconnecting client.", SDL_GetError());
                SDLNet_DestroyStreamSocket(clientSocket); // Clean up only the failed socket
                clientSocket = NULL;
            }
        }
        else if (bytesReceived < 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ReadFromStreamSocket failed: %s. Disconnecting client.", SDL_GetError());
            SDLNet_DestroyStreamSocket(clientSocket); // Clean up only the failed socket
            clientSocket = NULL;
        }
    }
}

SDL_AppResult init_server()
{
    if (!SDLNet_Init())
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Server SDLNet_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    serverSocket = SDLNet_CreateServer(NULL, SERVER_PORT);
    if (serverSocket == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDLNet_CreateServer failed: %s", SDL_GetError());
        SDLNet_Quit();
        return SDL_APP_FAILURE;
    }
    SDL_Log("Server listening on port %d.", SERVER_PORT);

    Entity server_e = {
        .name = "net_server",
        .update = update,
        .cleanup = cleanup};
    create_entity(server_e);
    return SDL_APP_SUCCESS;
}