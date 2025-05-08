#pragma once

// --- Includes ---
#include "../include/common.h"
#include <SDL3/SDL.h>
#include "../include/entity.h"
#include "../include/map.h"
#include "../include/base.h"
#include "../include/tower.h"
#include "../include/attack.h"
#include "../include/player.h"
#include "../include/camera.h"
#include "../include/net_server.h"
#include "../include/net_client.h"

// --- Function Declarations ---

/**
 * @brief Main application cleanup function.
 * Called by SDL_AppQuit in init.c. Responsible for destroying all modules
 * and SDL resources in the correct order.
 * @param appstate Pointer to the main AppState.
 * @param result The result code from the application loop.
 */
void app_cleanup(void *appstate, SDL_AppResult result);
