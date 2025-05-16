#pragma once

// --- Includes ---
#include "../include/common.h"
#include "../include/camera.h"
#include "../include/entity.h"

// --- Constants ---
#define MAX_BASES 2

#define BASE_RENDER_WIDTH 296.0f
#define BASE_RENDER_HEIGHT 205.0f
#define BASE_HEALTH_MAX 200

// Hardcoded positions
#define BASE_RED_POS_X 160.0f
#define BASE_BLUE_POS_X 3040.0f

#define RED_BASE_PATH "./resources/Sprites/Red_Team/Castle_Red.png"
#define BLUE_BASE_PATH "./resources/Sprites/Blue_Team/Castle_Blue.png"
#define DESTROYED_BASE_PATH "./resources/Sprites/Castle_Destroyed.png"

// --- Opaque Pointer Type ---
/**
 * @brief Opaque handle to the BaseManager state.
 * Manages data and actions related to team bases.
 */
typedef struct BaseManagerState_s *BaseManagerState;

/**
 * @brief State of a single base.
 */
typedef struct BaseInstance
{
    int index;
    SDL_FRect rect;       /**< Rect for this base. */
    SDL_FPoint position;  /**< World position (center). */
    SDL_Texture *texture; /**< Texture for this base. */
    int current_health;   /**< Current health points. */
    bool team;            /**< Which team the base belongs to. */
    bool immune;
} BaseInstance;

/**
 * @brief Internal state for the BaseManager module.
 */
struct BaseManagerState_s
{
    BaseInstance bases[MAX_BASES];  /**< Array for the two bases (index 0=Red, 1=Blue). */
    SDL_Texture *red_texture;       /**< Texture for the red base. */
    SDL_Texture *blue_texture;      /**< Texture for the blue base. */
    SDL_Texture *destroyed_texture; /**< Texture for the destroyed base. */
};

// --- Public API Function Declarations ---

/**
 * @brief Initializes the BaseManager module and registers its entity functions.
 * Loads required resources for the bases.
 * @param state Pointer to the main AppState (provides renderer, entity manager).
 * @return A new BaseManagerState instance on success, NULL on failure.
 * @sa BaseManager_Destroy
 */
BaseManagerState BaseManager_Init(AppState *state);

/**
 * @brief Destroys the BaseManagerState instance and associated resources.
 * Cleanup is primarily handled via the EntityManager callback. This frees the manager state itself.
 * @param bm_state The BaseManagerState instance to destroy.
 * @sa BaseManager_Init
 */
void BaseManager_Destroy(BaseManagerState bm_state);

/**
 * @brief Applies damage to a base, sends damageValue to server and checks if
 * base has been destroyed.
 * @param state Pointer to the main AppState.
 * @param baseIndex The index of the base to apply damage to.
 * @param damageValue The amount of damage to apply.
 * @param sendToServer true if the local client is the one that hit the base.
 */
void damageBase(AppState *state, int baseIndex, float damageValue, bool sendToServer);
