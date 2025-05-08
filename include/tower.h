#pragma once

// --- Includes ---
#include "../include/common.h"
#include "../include/camera.h"
#include "../include/player.h"
#include "../include/attack.h"
#include "../include/entity.h"

// --- Constants ---
#define MAX_TOWERS_PER_TEAM 2
#define MAX_TOTAL_TOWERS (MAX_TOWERS_PER_TEAM * 2)

// Define Tower properties and positions here
#define TOWER_RENDER_WIDTH 114.0f
#define TOWER_RENDER_HEIGHT 183.0f
#define TOWER_HEALTH_MAX 100
#define TOWER_ATTACK_RANGE 200.0f
#define TOWER_ATTACK_DAMAGE 50
#define TOWER_ATTACK_COOLDOWN 1.5f // Seconds between shots

#define TOWER_RED_1_X 700.0f    // Position of the first red tower
#define TOWER_BLUE_1_X 2500.0f  // Position of the first blue tower
#define TOWER_DISTANCE_X 500.0f // Distance between each teams towers

// --- Opaque Pointer Type ---
/**
 * @brief Opaque handle to the TowerManager state.
 * Manages data and actions related to team towers.
 */
typedef struct TowerManagerState_s *TowerManagerState;

/**
 * @brief State of a single tower.
 */
typedef struct TowerInstance
{
    bool team;                   /**< Which team the tower belongs to. */
    SDL_FRect rect;              /**< Rect for this tower. */
    SDL_FPoint position;         /**< World position (center or base). */
    SDL_Texture *texture;        /**< Texture for this tower. */
    int current_health;          /**< Current health points. */
    int max_health;              /**< Maximum health points. */
    float attack_cooldown_timer; /**< Time remaining until the next attack can occur. */
    bool is_active;              /**< Flag indicating if the tower is currently functional. */
} TowerInstance;

/**
 * @brief Internal state for the TowerManager module.
 */
struct TowerManagerState_s
{
    TowerInstance towers[MAX_TOTAL_TOWERS]; /**< Array for all towers. */
    SDL_Texture *red_texture;               /**< Texture for red towers. */
    SDL_Texture *blue_texture;              /**< Texture for blue towers. */
    SDL_Texture *destroyed_texture;         /**< Texture for destroyed towers. */
    int tower_count;                        /**< Number of initialized towers. */
};

// --- Public API Function Declarations ---

/**
 * @brief Initializes the TowerManager module and registers its entity functions.
 * Loads required resources for the towers.
 * @param state Pointer to the main AppState (provides renderer, entity manager).
 * @return A new TowerManagerState instance on success, NULL on failure.
 * @sa TowerManager_Destroy
 */
TowerManagerState TowerManager_Init(AppState *state);

/**
 * @brief Destroys the TowerManagerState instance and associated resources.
 * Cleanup is primarily handled via the EntityManager callback. This frees the manager state itself.
 * @param tm_state The TowerManagerState instance to destroy.
 * @sa TowerManager_Init
 */
void TowerManager_Destroy(TowerManagerState tm_state);

/**
 * @brief Applies damage to a tower, sends damageValue to server and checks if
 * tower has been destroyed.
 * @param state Pointer to the main AppState.
 * @param towerIndex The index of the tower to apply damage to.
 * @param damageValue The amount of damage to apply.
 * @param sendToServer true if the local client is the one that hit the tower.
 */
void damageTower(AppState state, int towerIndex, float damageValue, bool sendToServer);