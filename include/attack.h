#pragma once

#include "../include/common.h"
#include "../include/camera.h"
#include "../include/tower.h"
#include "../include/base.h"

// --- Constants ---

#define HIT_RANGE 3.0f           // Distance threshold for a fireball hit.
#define MAX_FIREBALLS 10         // Maximum number of concurrent fireballs.
#define FIREBALL_FRAME_WIDTH 40  // Width of a single frame in the fireball spritesheet.
#define FIREBALL_FRAME_HEIGHT 40 // Height of a single frame in the fireball spritesheet.
#define FIREBALL_WIDTH 33        // Rendered width of the fireball.
#define FIREBALL_HEIGHT 39       // Rendered height of the fireball.
#define FIREBALL_SPEED 100.0f    // Movement speed of the fireball in units per second.

// --- Structures ---

/**
 * @brief Represents a single fireball projectile in the game.
 */
typedef struct FireBall
{
    SDL_Texture *texture;        /**< Shared texture atlas for the fireball animation. */
    SDL_FPoint src;              /**< Source rectangle coordinates within the texture atlas (not currently used for animation). */
    SDL_FPoint dst;              /**< Destination position (top-left corner) in screen coordinates. */
    SDL_FPoint target;           /**< Target position in screen coordinates the fireball moves towards. */
    int hit;                     /**< Flag indicating if the fireball has hit something (1) or not (0). */
    SDL_FRect attackable_tower1; /**< The towers that a player can attack based on its team */
    SDL_FRect attackable_tower2; /**< The towers that a player can attack based on its team */
    SDL_FRect attackable_base;   /**< The bases that a player can attack based on its team */
    int active;                  /**< Flag indicating if the fireball slot is currently in use (1) or available (0). */
    float angle_deg;             /**< Angle of rotation in degrees for rendering. */
    float velocity_x;            /**< Horizontal velocity component. */
    float velocity_y;            /**< Vertical velocity component. */
    int rotation_diff_x;         /**< X offset adjustment due to rotation (not currently calculated). */
    int rotation_diff_y;         /**< Y offset adjustment due to rotation (not currently calculated). */
} FireBall;

// --- Global Variables ---

/**
 * @brief Global array storing all fireball instances.
 */
extern FireBall fireBalls[MAX_FIREBALLS];

// --- Public Function Declarations ---

/**
 * @brief Initializes the fireball system, loads resources, and registers the entity.
 * @param renderer The main SDL renderer.
 * @return SDL_APP_SUCCESS on success, SDL_APP_FAILURE on error.
 */
SDL_AppResult init_fireball(SDL_Renderer *renderer, bool team_arg);

/**
 * @brief Creates and launches a new fireball from the player's position towards the mouse cursor.
 * @param player_pos_x The player's world X coordinate.
 * @param player_pos_y The player's world Y coordinate.
 * @param cam_x The camera's world X coordinate.
 * @param cam_y The camera's world Y coordinate.
 * @param mouse_view_x The mouse's X coordinate relative to the viewport.
 * @param mouse_view_y The mouse's Y coordinate relative to the viewport.
 */
void activate_fireballs(float player_pos_x, float player_pos_y, float cam_x, float cam_y, float mouse_view_x, float mouse_view_y, bool team);