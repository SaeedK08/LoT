#pragma once

#include "../include/common.h"
#include "../include/player.h"
#include "../include/base.h"
#include "../include/tower.h"
#include "../include/attack.h"

/**
 * @brief Represents the different network connection states of the client.
 */
typedef enum
{
    CLIENT_STATE_DISCONNECTED,
    CLIENT_STATE_RESOLVING,
    CLIENT_STATE_CONNECTING,
    CLIENT_STATE_CONNECTED
} ClientNetworkState;

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

/**
 * @brief Holds state information for remote fireballs received from the server.
 */
typedef struct
{
    bool active;         /**< Flag indicating if this fireball slot is active. */
    uint8_t client_id;   /**< Unique identifier for the client/player. */
    SDL_FPoint dst;      /**< Destination position (top-left corner) in screen coordinates. */
    SDL_FPoint target;   /**< Target position in screen coordinates the fireball moves towards. */
    float angle_deg;     /**< Angle of rotation in degrees for rendering. */
    float velocity_x;    /**< Horizontal velocity component. */
    float velocity_y;    /**< Vertical velocity component. */
    int rotation_diff_x; /**< X offset adjustment due to rotation (not currently calculated). */
    int rotation_diff_y; /**< Y offset adjustment due to rotation (not currently calculated). */
    int hit;
} RemoteFireBall;

// Array to store data about other players received from the server
extern RemotePlayer remotePlayers[MAX_CLIENTS];
extern RemoteFireBall RemoteFireballs[MAX_CLIENTS * MAX_FIREBALLS];

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

void send_match_result(MessageType game_result, int baseIndex);

void send_tower_destroyed(int towerIndex);

void send_local_fireball_state(FireballStateData local_fireball);