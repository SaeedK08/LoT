#include "../include/net.h"

#define SERVER_PORT 8080

static SDLNet_Server *serverSocket = NULL;
static SDLNet_StreamSocket *clientSocket = NULL;

static void cleanup()
{
    SDLNet_DestroyStreamSocket(clientSocket);
    clientSocket = NULL;

    SDLNet_DestroyServer(serverSocket);
    serverSocket = NULL;

    SDLNet_Quit();
    SDL_Log("SDLNet has been cleaned up.");
}

static void update(float delta_time)
{
    // Only try to accept if the server socket exists and we don't already have a client
    if (serverSocket != NULL && clientSocket == NULL)
    {
        SDLNet_StreamSocket *newClient = NULL; // Temporary variable to hold potential new client

        // Try to accept a client
        bool success = SDLNet_AcceptClient(serverSocket, &newClient);

        if (!success)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDLNet_AcceptClient failed: %s", SDL_GetError());
        }
        else if (newClient != NULL)
        {
            clientSocket = newClient; // Store the handle
            SDL_Log("Client connected!");
        }
    }

    if (clientSocket != NULL)
    {
        // --- Receive ---
        char buffer[512];
        int bytesReceived;
        bytesReceived = SDLNet_ReadFromStreamSocket(clientSocket, buffer, sizeof(buffer) - 1);

        if (bytesReceived > 0)
        {
            // Data was received!
            buffer[bytesReceived] = '\0';
            SDL_Log("Received %d bytes from client: %s", bytesReceived, buffer);

            // --- Send a Reply ---
            const char *reply = "Server received your message!";
            SDL_Log("Sending reply: %s", reply);
            bool sentSuccess = SDLNet_WriteToStreamSocket(clientSocket, reply, SDL_strlen(reply)); // Use SDL_strlen for C strings

            if (!sentSuccess)
            {
                // Send failed, likely client disconnected
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error sending reply to client: %s. Client disconnected.", SDL_GetError());
                SDLNet_DestroyStreamSocket(clientSocket);
                clientSocket = NULL;
            }
            else
            {
                SDL_Log("Reply sent successfully.");
            }
            // --- Reply Sent ---
        }
        else if (bytesReceived == 0)
        {
            // No data available right now.
        }
        else
        {
            // Error or disconnection on read
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error reading from client socket: %s. Client disconnected.", SDL_GetError());
            SDLNet_DestroyStreamSocket(clientSocket);
            clientSocket = NULL;
        }
    }
}

SDL_AppResult init_net()
{
    if (!SDLNet_Init())
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error initializing SDL_Net: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_Log("SDLNet has been initialized.");

    serverSocket = SDLNet_CreateServer(NULL, SERVER_PORT);

    if (serverSocket == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error creating server socket: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_Log("Server socket created. Listening on port %d.", SERVER_PORT);

    Entity net_e = {
        .name = "net",
        .update = update,
        .cleanup = cleanup};
    create_entity(net_e);

    return SDL_APP_SUCCESS;
}