#pragma once

#include "../include/common.h"
#include "../include/player.h"

/**
 * @brief Holds state information for remote players received from the server.
 */
typedef struct
{
    SDL_FPoint position;
    SDL_FRect sprite_portion; /**< Current animation frame/sprite portion. */
    SDL_FlipMode flip_mode;   /**< Current sprite flip state. */
    bool active;              /**< Flag indicating if this player slot is active. */
    bool team;
} RemotePlayer;

// Array to store data about other players received from the server
extern RemotePlayer remotePlayers[MAX_CLIENTS];

/**
 * @brief Initializes the network client entity and prepares for connection attempts.
 * @return SDL_APP_SUCCESS on successful initialization, SDL_APP_FAILURE otherwise.
 */
SDL_AppResult init_client(bool team_arg);

/**
 * @brief Gets the client index assigned by the server upon successful connection.
 * @return The assigned client index (0 to MAX_CLIENTS-1), or -1 if not connected/assigned.
 */
int get_my_client_index(void);

/**
 * @brief Checks if the client is currently in the connected state.
 * @return true if the client state is CLIENT_STATE_CONNECTED, false otherwise.
 */
bool is_client_connected(void);

/**
 * @brief Sends the current state of the local player to the server.
 * Does nothing if the client is not connected or player state cannot be retrieved.
 * @return void
 */
void send_local_player_state(void);