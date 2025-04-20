#include "../include/net_client.h"

static SDLNet_Address *serverAddress = NULL;
static SDLNet_StreamSocket *serverConnection = NULL;
static bool isConnected = false;

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
    SDLNet_Quit();
    SDL_Log("Client SDLNet cleaned up.");
    isConnected = false;
}

static void update(float delta_time)
{
    // Check address resolution and attempt connection
    if (serverAddress != NULL && serverConnection == NULL)
    {
        int address_status = SDLNet_GetAddressStatus(serverAddress);
        if (address_status == 1)
        {
            SDL_Log("Hostname resolved.");
            serverConnection = SDLNet_CreateClient(serverAddress, SERVER_PORT);
            if (serverConnection == NULL)
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "CreateClient failed: %s", SDL_GetError());
                SDLNet_UnrefAddress(serverAddress); // Clean up address if connection couldn't start
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

            // Send initial message
            const char *message = "Hello Server!";
            SDL_Log("Sending: %s", message);
            bool sentSuccess = SDLNet_WriteToStreamSocket(serverConnection, message, SDL_strlen(message));
            if (!sentSuccess)
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Initial send failed: %s. Disconnecting.", SDL_GetError());
                SDLNet_DestroyStreamSocket(serverConnection);
                serverConnection = NULL;
                isConnected = false;
                // Also clean up address if initial send failed right after connect
                if (serverAddress != NULL)
                {
                    SDLNet_UnrefAddress(serverAddress);
                    serverAddress = NULL;
                }
            }
        }
        else if (connection_status == -1)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "GetConnectionStatus failed: %s", SDL_GetError());
            SDLNet_DestroyStreamSocket(serverConnection);
            serverConnection = NULL;
            // Also clean up address if connection failed
            if (serverAddress != NULL)
            {
                SDLNet_UnrefAddress(serverAddress);
                serverAddress = NULL;
            }
        }
    }

    // Handle active connection (receiving)
    if (isConnected && serverConnection != NULL)
    {
        char buffer[512];
        int bytesReceived;
        bytesReceived = SDLNet_ReadFromStreamSocket(serverConnection, buffer, sizeof(buffer) - 1);

        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            SDL_Log("Received: %s", buffer);
        }
        else if (bytesReceived < 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ReadStreamSocket failed: %s. Disconnected.", SDL_GetError());
            SDLNet_DestroyStreamSocket(serverConnection);
            serverConnection = NULL;
            isConnected = false; // Reset connection flag
        }
    }
}

SDL_AppResult init_client()
{
    isConnected = false;
    if (!SDLNet_Init())
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Client SDLNet_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    serverAddress = SDLNet_ResolveHostname(HOSTNAME);
    if (serverAddress == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ResolveHostname failed immediately: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    Entity client_e = {
        .name = "net_client",
        .update = update,
        .cleanup = cleanup};
    create_entity(client_e);
    return SDL_APP_SUCCESS;
}