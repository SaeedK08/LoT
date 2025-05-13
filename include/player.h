#pragma once

// --- Standard Library Includes ---
#include <stdbool.h>
#include <stdint.h>

// --- SDL/External Library Includes ---
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_events.h>

// --- Project Includes ---
#include "../include/common.h"
#include "../include/camera.h"
#include "../include/attack.h"
#include "../include/net_client.h"
#include "../include/map.h"
#include "../include/base.h"
#include "../include/tower.h"
#include "../include/entity.h"

// --- Constants ---
#define PLAYER_WIDTH 32.0f
#define PLAYER_HEIGHT 64.0f
#define PLAYER_SPEED 1000.0f       /**< Player movement speed in pixels per second. */
#define PLAYER_ATTACK_RANGE 100.0f /**< Maximum distance the player can initiate an attack from. */
#define PLAYER_HEALTH_MAX 200
#define PLAYER_DEATH_TIMER 2000 /**< Duration for death in ms. */

#define PLAYER_SPRITE_FRAME_WIDTH 64.0f
#define PLAYER_SPRITE_FRAME_HEIGHT 80.0f
#define PLAYER_SPRITE_IDLE_ROW_Y 0.0f
#define PLAYER_SPRITE_WALK_ROW_Y 80.0f
#define PLAYER_SPRITE_HURT_ROW_Y 320.0f
#define PLAYER_SPRITE_DEAD_ROW_Y 400.0f
#define PLAYER_SPRITE_ATTACK_ROW_Y 480.0f
#define PLAYER_SPRITE_NUM_IDLE_FRAMES 6
#define PLAYER_SPRITE_NUM_WALK_FRAMES 6
#define PLAYER_SPRITE_NUM_HURT_FRAMES 3
#define PLAYER_SPRITE_NUM_DEAD_FRAMES 4
#define PLAYER_SPRITE_NUM_ATTACK_FRAMES 6
#define PLAYER_SPRITE_TIME_PER_FRAME 0.1f /**< Duration each animation frame is displayed. */

#define RED_WIZARD_PATH "./resources/Sprites/Red_Team/Fire_Wizard/Fire_Wizard_Spiresheet.png"
#define BLUE_WIZARD_PATH "./resources/Sprites/Blue_Team/Lightning_Wizard/Lightning_Wizard_Spritesheet.png"

// --- Opaque Pointer Type ---

/**
 * @brief Opaque handle to the PlayerManager state.
 * Manages data and actions for all player characters (local and remote).
 */
typedef struct PlayerManager_s *PlayerManager;

/**
 * @brief Holds all state data for a single player instance (local or remote).
 */
typedef struct PlayerInstance
{
    SDL_FRect rect;
    SDL_FPoint position;      /**< Current world position (center). */
    SDL_FRect sprite_portion; /**< The source rect defining the current animation frame. */
    float anim_timer;         /**< Timer used to advance animation frames. */
    int current_frame;        /**< Index of the current frame within the current animation sequence. */
    SDL_FlipMode flip_mode;   /**< Rendering flip state (horizontal). */
    bool active;              /**< Whether this player slot is currently in use. */
    bool is_local;            /**< True if this is the player controlled by this game instance. */
    bool is_moving;           /**< Tracks if the player is currently considered moving (for animation). */
    bool team;
    bool dead;
    int deathTime;
    bool playDeathAnim;
    bool playHurtAnim;
    bool playAttackAnim;
    SDL_Texture *texture;
    int current_health; /**< Current health points. */
    int max_health;     /**< Maximum health points. */
    int index;
} PlayerInstance;

/**
 * @brief Internal state for the PlayerManager module ADT.
 */
struct PlayerManager_s
{
    PlayerInstance players[MAX_CLIENTS]; /**< Array holding data for all potential players. */
    int local_player_client_id;          /**< Client ID of the local player, or -1 if none/disconnected. */
    SDL_Texture *player_texture;         /**< Shared texture atlas for player sprites. */
    SDL_Texture *red_texture;
    SDL_Texture *blue_texture;
};

// --- Public API Function Declarations ---

/**
 * @brief Initializes the PlayerManager module and registers its entity functions.
 * Creates the manager state and loads common player resources.
 * @param state Pointer to the main AppState.
 * @return A new PlayerManager instance on success, NULL on failure.
 * @sa PlayerManager_Destroy
 */
PlayerManager PlayerManager_Init(AppState *state);

/**
 * @brief Destroys the PlayerManager instance and associated resources.
 * Cleanup is primarily handled via the EntityManager callback. This frees the manager state itself.
 * @param pm The PlayerManager instance to destroy.
 * @sa PlayerManager_Init
 */
void PlayerManager_Destroy(PlayerManager pm);

/**
 * @brief Sets the client ID for the local player instance managed by this manager.
 * Marks the corresponding player slot as the 'local' one for input handling.
 * Should be called after the client receives the S_WELCOME message.
 * @param pm The PlayerManager instance.
 * @param client_id The client ID assigned by the server.
 * @return True on success, false if the ID is invalid or already set.
 */
bool PlayerManager_SetLocalPlayerID(AppState *state, uint8_t client_id);

/**
 * @brief Updates the state of a remote player based on received network data.
 * Creates or updates the player data in the manager's internal array.
 * @param pm The PlayerManager instance.
 * @param data Pointer to the received Msg_PlayerStateData.
 */
void PlayerManager_UpdateRemotePlayer(AppState *state, const Msg_PlayerStateData *data);

/**
 * @brief Marks a remote player as inactive.
 * Typically called upon receiving a disconnect message from the server.
 * @param pm The PlayerManager instance.
 * @param client_id The ID of the client/player to deactivate.
 */
void PlayerManager_RemovePlayer(PlayerManager pm, uint8_t client_id);

/**
 * @brief Gets the world position of the local player.
 * Used by other systems like the Camera to track the player.
 * @param pm The PlayerManager instance.
 * @param out_pos Pointer to an SDL_FPoint to store the position.
 * @return True if the local player exists and position was retrieved, false otherwise.
 */
bool PlayerManager_GetLocalPlayerPosition(PlayerManager pm, SDL_FPoint *out_pos);

/**
 * @brief Gets the world position of any active player by their client ID.
 * Used by other systems like AttackManager or TowerManager.
 * @param pm The PlayerManager instance.
 * @param client_id The ID of the player to query.
 * @param out_pos Pointer to an SDL_FPoint to store the position.
 * @return True if the player exists, is active, and position was retrieved, false otherwise.
 */
bool PlayerManager_GetPlayerPosition(PlayerManager pm, uint8_t client_id, SDL_FPoint *out_pos);

/**
 * @brief Gets the current state of the local player for network transmission.
 * Fills the provided struct with the necessary data to send to the server.
 * @param pm The PlayerManager instance.
 * @param out_data Pointer to a Msg_PlayerStateData struct to fill.
 * @return True if the local player exists and state was retrieved, false otherwise.
 */
bool PlayerManager_GetLocalPlayerState(PlayerManager pm, Msg_PlayerStateData *out_data);

void damagePlayer(AppState state, int playerIndex, float damageValue, bool sendToServer);
