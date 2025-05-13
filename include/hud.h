#pragma once

// --- SDL/External Library Includes ---

// --- Project Includes ---
#include "../include/common.h"
#include "../include/net_server.h"

/**
 * @brief Enum defining different attack types.
 */
typedef enum HUDElement
{
    LOBBY_HOST_MSG = 0,
    LOBBY_CLIENT_MSG = 1,
    LOBBY_HOST_INPUT = 2,
} HUDElement;

#define HUD_DEFAULT_FONT_SIZE 28
#define HUD_MAX_ELEMENTS_AMOUNT 100

// --- Opaque Pointer Type ---

/**
 * @brief Opaque handle to the HUDManager state.
 * Manages all active HUD instances in the game.
 */
typedef struct HUDManager_s *HUDManager;

// --- Public API Function Declarations ---

/**
 * @brief Initializes the HUDManager module and registers its entity functions.
 * Loads resources required for HUD elements.
 * @param state Pointer to the main AppState.
 * @return A new HUDManager instance on success, NULL on failure.
 * @sa HUDManager_Destroy
 */
HUDManager HUDManager_Init(AppState *state);

/**
 * @brief Destroys the HUDManager instance and associated resources.
 * Cleanup is primarily handled via the EntityManager callback. This frees the manager state itself.
 * @param am The HUDManager instance to destroy.
 * @sa HUDManager_Init
 */
// void HUDManager_Destroy(HUDManager hm);

void create_hud_instace(AppState *state, int index, char name[], bool visible, char text_buffer[], SDL_Color color, bool changeable, SDL_FPoint dest_point);
