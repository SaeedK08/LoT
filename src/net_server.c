#include "../include/net_server.h"

typedef struct
{
    SDLNet_StreamSocket *socket;
    SDL_FPoint position;
    bool active;
} ClientInfo;

// Static Variables
static SDLNet_Server *serverSocket = NULL;
static ClientInfo clients[MAX_CLIENTS];

int amountPlayers = -1;

// Forward Declarations
static void disconnectClient(int clientIndex);
static SDL_AppResult acceptConnections(void);
static void broadcast_position(int senderIndex, const PlayerPosUpdateData *update_data);
static void process_client_message(int clientIndex, char *buffer, int bytesReceived);
static void handle_single_client(int clientIndex);

static void cleanup()
{
    // Client Cleanup
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i].active && clients[i].socket != NULL)
        {
            disconnectClient(i);
        }
    }
    memset(clients, 0, sizeof(clients)); // Clear the array

    // Server Socket Cleanup
    if (serverSocket != NULL)
    {
        SDLNet_DestroyServer(serverSocket);
        serverSocket = NULL;
    }
}

// Disconnects a client, cleans up their resources
static void disconnectClient(int clientIndex)
{
    // Input Validation
    if (clientIndex < 0 || clientIndex >= MAX_CLIENTS || !clients[clientIndex].active)
        return;

    // Socket Cleanup
    if (clients[clientIndex].socket != NULL)
    {
        SDLNet_DestroyStreamSocket(clients[clientIndex].socket);
        clients[clientIndex].socket = NULL;
    }
    // Reset State
    clients[clientIndex].active = false;
    clients[clientIndex].position = (SDL_FPoint){0.0f, 0.0f};
}

// Accepts any new incoming connections
static SDL_AppResult acceptConnections(void)
{
    if (serverSocket == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Server socket is NULL.", __func__);
        return SDL_APP_FAILURE; // Cannot accept if server isn't initialized
    }

    SDLNet_StreamSocket *newClientSocket = NULL;

    while (SDLNet_AcceptClient(serverSocket, &newClientSocket) && newClientSocket != NULL)
    {
        int clientSlot = -1;
        // Find Available Slot
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (!clients[i].active)
            {
                clientSlot = i;
                break;
            }
        }

        // Handle Connection
        if (clientSlot != -1)
        {
            clients[clientSlot].socket = newClientSocket;
            clients[clientSlot].active = true;
            newClientSocket = NULL; // Reset for next potential accept
            SDL_Log("server amountplayers %d", clientSlot);
            amountPlayers++;
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[%s] Server full. Rejecting new client.", __func__);
            SDLNet_DestroyStreamSocket(newClientSocket);
            newClientSocket = NULL;
            break; // Stop trying to accept if server is full
        }
    }

    // Check for actual errors (when SDLNet_AcceptClient returns false or newClientSocket is NULL)
    const char *error = SDL_GetError();
    if (error && error[0] != '\0' && strcmp(error, "No incoming connections") != 0)
    {
        // SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] SDLNet_AcceptClient failed: %s", __func__, error);
        // return SDL_APP_FAILURE;
    }
    return SDL_APP_SUCCESS; // Return success even if non-fatal accept error occurred
}

// Broadcasts position data to all clients except the sender
static void broadcast_position(int senderIndex, const PlayerPosUpdateData *update_data)
{
    // // Prepare Buffer
    // uint8_t broadcast_buffer[1 + sizeof(PlayerPosUpdateData)];
    // broadcast_buffer[0] = MSG_TYPE_PLAYER_POS;
    // memcpy(broadcast_buffer + 1, update_data, sizeof(PlayerPosUpdateData));

    // // Send to Clients
    // for (int j = 0; j < MAX_CLIENTS; ++j)
    // {
    //     // Skip sender and inactive/invalid clients
    //     if (j == senderIndex || !clients[j].active || clients[j].socket == NULL)
    //     {
    //         continue;
    //     }

    //     // Write Data
    //     if (!SDLNet_WriteToStreamSocket(clients[j].socket, broadcast_buffer, sizeof(broadcast_buffer)))
    //     {
    //         SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Broadcast POS failed (Sender %d, Recipient %d): %s.", __func__, senderIndex, j, SDL_GetError());
    //         disconnectClient(j);
    //     }
    // }
}

// Processes a received message from a specific client
static void process_client_message(int clientIndex, char *buffer, int bytesReceived)
{
    // Basic Validation
    if (bytesReceived < (int)sizeof(uint8_t))
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[%s] Received incomplete message (<1 byte) from client %d", __func__, clientIndex);
        return;
    }

    uint8_t msg_type_byte = buffer[0];

    // Message Handling
    switch (msg_type_byte)
    {
    case MSG_TYPE_C_HELLO:
        int msgArray[2];
        msgArray[0] = MSG_TYPE_S_WELCOME;
        msgArray[1] = clientIndex;                                                         // Ensure this uses clientIndex
        SDL_Log("Sending S_WELCOME to client %d with index %d", clientIndex, msgArray[1]); // Added log for clarity
        if (!SDLNet_WriteToStreamSocket(clients[clientIndex].socket, msgArray, sizeof(msgArray)))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Write S_WELCOME failed (Client %d): %s.", __func__, clientIndex, SDL_GetError());
            disconnectClient(clientIndex);
        }
        break;

        // case MSG_TYPE_PLAYER_POS:
        //     if (bytesReceived >= (int)(1 + sizeof(SDL_FPoint)))
        //     {
        //         SDL_FPoint received_pos;
        //         memcpy(&received_pos, buffer + 1, sizeof(SDL_FPoint));
        //         clients[clientIndex].position = received_pos; // Store position

        //         PlayerPosUpdateData update_data;
        //         update_data.client_id = (uint8_t)clientIndex;
        //         update_data.position = clients[clientIndex].position;

        //         broadcast_position(clientIndex, &update_data); // Relay position
        //     }
        //     else
        //     {
        //         SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[%s] Received incomplete PLAYER_POS message from client %d (%d bytes)", __func__, clientIndex, bytesReceived);
        //     }
        //     break;

    default:
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[%s] Received unknown message type (%u) from client %d", __func__, (unsigned int)msg_type_byte, clientIndex);
        break;
    }
}

// Reads data from a single client and processes it
static void handle_single_client(int clientIndex)
{
    // Skip Inactive
    if (!clients[clientIndex].active || clients[clientIndex].socket == NULL)
        return;

    char buffer[BUFFER_SIZE];
    int bytesReceived;

    // Non-Blocking Read Loop
    while (true)
    {
        bytesReceived = SDLNet_ReadFromStreamSocket(clients[clientIndex].socket, buffer, sizeof(buffer));

        if (bytesReceived > 0)
        {
            process_client_message(clientIndex, buffer, bytesReceived);
            // Assume no more data if read is less than buffer size
            if (bytesReceived < (int)sizeof(buffer))
            {
                break;
            }
        }
        else if (bytesReceived == 0)
        {
            // No data currently available
            break;
        }
        else // bytesReceived < 0 indicates error/disconnect
        {
            const char *sdlError = SDL_GetError();
            if (strcmp(sdlError, "Socket is not connected") != 0 && strcmp(sdlError, "Connection reset by peer") != 0)
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Read failed (Client %d): %s.", __func__, clientIndex, sdlError);
            }
            disconnectClient(clientIndex);
            break; // Exit loop after disconnect/error
        }
    }
}

static void update(AppState *state)
{
    (void)state;

    acceptConnections();

    // Handle data from all currently active clients
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        handle_single_client(i);
    }
}

SDL_AppResult init_server()
{
    memset(clients, 0, sizeof(clients)); // Initialize client array

    // Create Server Socket
    serverSocket = SDLNet_CreateServer(NULL, SERVER_PORT);
    if (serverSocket == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] SDLNet_CreateServer failed: %s", __func__, SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create Server Entity
    Entity server_e = {
        .name = "net_server",
        .update = update,
        .cleanup = cleanup};

    // Check if entity creation succeeded
    if (create_entity(server_e) == SDL_APP_FAILURE)
    {
        SDLNet_DestroyServer(serverSocket);
        serverSocket = NULL;
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Failed to create server entity.", __func__);
        return SDL_APP_FAILURE;
    }

    return SDL_APP_SUCCESS;
}

int funcGetAmountPlayers()
{
    SDL_Log("in the func amountplayers %d", amountPlayers);
    return amountPlayers;
}