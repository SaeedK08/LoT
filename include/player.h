#pragma once

#include <stdbool.h>

#include "../include/common.h"
#include "../include/camera.h"
#include "../include/net_client.h"
#include "../include/camera.h"
#include "../include/net_client.h"

struct Player;

/**
 * @brief Initializes the player resources and entity for a specific client index.
 * Creates the player entity if it doesn't exist. Assumes it's the local player.
 * @param renderer The main SDL renderer.
 * @param assigned_player_index The client index assigned to this player.
 * @return Pointer to the internally managed Player struct, or NULL on failure.
 */
struct Player *init_player(SDL_Renderer *renderer, int assigned_player_index);

/**
 * @brief Gets the current world coordinates of the local player.
 * @return SDL_FPoint containing the player's world x, y position. Returns {-1.0f, -1.0f} if the local player is not initialized.
 */
SDL_FPoint funcGetPlayerPos(void);

/**
 * @brief Populates a given struct with the local player's current state suitable for network transmission.
 * @param out_data Pointer to a PlayerStateData struct to be filled with the local player's current network-relevant state.
 * @return true if the local player is initialized and out_data was successfully populated, false otherwise (e.g., player not initialized, out_data is NULL).
 */
bool get_local_player_state_for_network(PlayerStateData *out_data);