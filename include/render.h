#pragma once

// --- Includes ---
#include "../include/common.h"
#include "../include/entity.h"

// --- Function Declarations ---

/**
 * @brief Renders the application state for the current frame.
 * Clears the screen and calls the render function for all managed entities.
 * @param appstate Void pointer to the main AppState struct.
 */
void app_render(void *appstate);
