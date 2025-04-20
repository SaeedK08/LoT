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
    SDL_Log("Client network resources cleaned up.");
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

            // Send initial message type
            uint8_t msg_type = MSG_TYPE_C_HELLO;
            SDL_Log("Sending C_HELLO message type (%d)", msg_type);
            bool sentSuccess = SDLNet_WriteToStreamSocket(serverConnection, &msg_type, sizeof(msg_type));
            if (!sentSuccess)
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Initial send failed: %s. Disconnecting.", SDL_GetError());
                SDLNet_DestroyStreamSocket(serverConnection);
                serverConnection = NULL;
                isConnected = false;
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
        bytesReceived = SDLNet_ReadFromStreamSocket(serverConnection, buffer, sizeof(buffer));

        if (bytesReceived > 0)
        {
            if (bytesReceived >= sizeof(uint8_t)) // Need at least 1 byte for type
            {
                uint8_t msg_type_byte = buffer[0]; // Get the first byte

                switch (msg_type_byte)
                {
                case MSG_TYPE_S_WELCOME:
                    SDL_Log("Received S_WELCOME from server");
                    // Client is now fully welcomed by server
                    break;

                default:
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Received unknown message type (%d) from server", msg_type_byte);
                    break;
                }
            }
            else
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Received incomplete message (less than 1 byte?) from server");
            }
        }
        else if (bytesReceived < 0)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ReadStreamSocket failed: %s. Disconnected.", SDL_GetError());
            SDLNet_DestroyStreamSocket(serverConnection);
            serverConnection = NULL;
            isConnected = false;
        }
    }
}

SDL_AppResult init_client()
{
    isConnected = false;

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