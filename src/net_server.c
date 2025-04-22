#include "../include/net_server.h"

typedef struct
{
    SDLNet_StreamSocket *socket;
    SDL_FPoint position;
    bool active;
} ClientInfo;

// --- Static Variables ---
static SDLNet_Server *serverSocket = NULL;
static ClientInfo clients[MAX_CLIENTS];

// --- Forward Declarations ---
static void disconnectClient(int clientIndex);
static void acceptConnections(void);
static void broadcast_position(int senderIndex, const PlayerPosUpdateData *update_data);
static void process_client_message(int clientIndex, char *buffer, int bytesReceived);
static void handle_single_client(int clientIndex);

// --- Entity Functions ---

static void cleanup()
{
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i].active && clients[i].socket != NULL)
        {
            disconnectClient(i); // Use disconnect to ensure proper cleanup
        }
    }
    // Clear the array after disconnecting all
    memset(clients, 0, sizeof(clients));

    if (serverSocket != NULL)
    {
        SDLNet_DestroyServer(serverSocket);
        serverSocket = NULL;
    }
    SDL_Log("Server network resources cleaned up.");
}

// Disconnects a client, cleans up their resources
static void disconnectClient(int clientIndex)
{
    if (clientIndex < 0 || clientIndex >= MAX_CLIENTS || !clients[clientIndex].active)
        return;

    SDL_Log("Disconnecting client %d.", clientIndex);
    if (clients[clientIndex].socket != NULL)
    {
        SDLNet_DestroyStreamSocket(clients[clientIndex].socket);
        clients[clientIndex].socket = NULL; // Important to nullify after destroy
    }
    clients[clientIndex].active = false;
    clients[clientIndex].position.x = 0;
    clients[clientIndex].position.y = 0;
}

// Accepts any new incoming connections
static void acceptConnections()
{
    if (serverSocket == NULL)
        return;

    SDLNet_StreamSocket *newClientSocket = NULL;
    // Loop to accept all pending connections in this frame
    while (SDLNet_AcceptClient(serverSocket, &newClientSocket) && newClientSocket != NULL)
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
            clients[clientSlot].position = (SDL_FPoint){0.0f, 0.0f}; // Default position
            SDL_Log("Client connected (Slot %d)", clientSlot);
            newClientSocket = NULL; // Reset for next potential accept
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Server full. Rejecting new client.");
            SDLNet_DestroyStreamSocket(newClientSocket);
            newClientSocket = NULL;
            break; // Stop trying to accept if server is full
        }
    }
}

// Broadcasts position data to all clients except the sender
static void broadcast_position(int senderIndex, const PlayerPosUpdateData *update_data)
{
    uint8_t broadcast_buffer[1 + sizeof(PlayerPosUpdateData)];
    broadcast_buffer[0] = MSG_TYPE_PLAYER_POS;
    memcpy(broadcast_buffer + 1, update_data, sizeof(PlayerPosUpdateData));

    for (int j = 0; j < MAX_CLIENTS; ++j)
    {
        // Send to everyone EXCEPT the original sender and inactive slots
        if (j == senderIndex || !clients[j].active || clients[j].socket == NULL)
        {
            continue;
        }

        if (!SDLNet_WriteToStreamSocket(clients[j].socket, broadcast_buffer, sizeof(broadcast_buffer)))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Broadcast POS failed (Sender %d, Recipient %d): %s.", senderIndex, j, SDL_GetError());
            disconnectClient(j); // Disconnect recipient if send failed
        }
    }
}

// Processes a received message from a specific client
static void process_client_message(int clientIndex, char *buffer, int bytesReceived)
{
    if (bytesReceived < (int)sizeof(uint8_t))
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Received incomplete message (<1 byte) from client %d", clientIndex);
        return;
    }

    uint8_t msg_type_byte = buffer[0];

    switch (msg_type_byte)
    {
    case MSG_TYPE_C_HELLO:
        SDL_Log("Received C_HELLO from client %d", clientIndex);
        uint8_t reply_type = MSG_TYPE_S_WELCOME;
        if (!SDLNet_WriteToStreamSocket(clients[clientIndex].socket, &reply_type, sizeof(reply_type)))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Write S_WELCOME failed (Client %d): %s.", clientIndex, SDL_GetError());
            disconnectClient(clientIndex);
        }
        break;

    case MSG_TYPE_PLAYER_POS:
        if (bytesReceived >= (int)(1 + sizeof(SDL_FPoint)))
        {
            SDL_FPoint received_pos;
            memcpy(&received_pos, buffer + 1, sizeof(SDL_FPoint));
            clients[clientIndex].position = received_pos; // Store position

            PlayerPosUpdateData update_data;
            update_data.client_id = (uint8_t)clientIndex;
            update_data.position = clients[clientIndex].position;

            broadcast_position(clientIndex, &update_data);
        }
        else
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Received incomplete PLAYER_POS message from client %d (%d bytes)", clientIndex, bytesReceived);
        }
        break;

    default:
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Received unknown message type (%d) from client %d", msg_type_byte, clientIndex);
        break;
    }
}

// Reads data from a single client and processes it
static void handle_single_client(int clientIndex)
{
    // Skip inactive clients or those somehow marked active but without a socket
    if (!clients[clientIndex].active || clients[clientIndex].socket == NULL)
        return;

    char buffer[BUFFER_SIZE];
    int bytesReceived;

    // Non-blocking read loop for this client
    while (true)
    {
        bytesReceived = SDLNet_ReadFromStreamSocket(clients[clientIndex].socket, buffer, sizeof(buffer));

        if (bytesReceived > 0)
        {
            process_client_message(clientIndex, buffer, bytesReceived);
            // If we read less than the buffer size, assume no more data for now
            if (bytesReceived < (int)sizeof(buffer))
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
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Read failed (Client %d): %s.", clientIndex, SDL_GetError());
            disconnectClient(clientIndex);
            break; // Exit loop after disconnect
        }
    }
}

// Main update function for the server entity
static void update(float delta_time)
{
    (void)delta_time;

    // Accept any new connections
    acceptConnections();

    // Handle data from all currently active clients
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        handle_single_client(i);
    }
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

    Entity server_e = {
        .name = "net_server",
        .update = update,
        .cleanup = cleanup};
    create_entity(server_e);

    return SDL_APP_SUCCESS;
}