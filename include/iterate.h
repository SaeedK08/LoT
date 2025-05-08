#pragma once

// --- Includes ---
#include "../include/common.h"
#include "../include/update.h"
#include "../include/render.h"

// --- Constants ---
#define TARGET_FPS 144
#define TARGET_FRAME_TIME_MS (1000 / TARGET_FPS)

// --- Function Declarations ---

/**
 * @brief Waits for the appropriate time to maintain the target frame rate.
 * Called by SDL_AppIterate in init.c.
 * @param appstate Void pointer to the main AppState struct.
 */
void app_wait_for_next_frame(void *appstate);
