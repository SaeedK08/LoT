#pragma once

// --- Includes ---
#include "../include/common.h"
#include "../include/entity.h"

// --- Function Declarations ---

/**
 * @brief Updates the application state for the current frame.
 * Calculates delta time and calls the update function for all managed entities.
 * @param appstate Void pointer to the main AppState struct.
 */
void app_update(void *appstate);
