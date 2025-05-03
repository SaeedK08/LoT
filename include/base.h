#pragma once

#include "../include/common.h"
#include "../include/camera.h"

#define MAX_BASES 2

// --- Public Function Declarations ---

/**
 * @brief Initializes the base entities, loads their textures, and registers the rendering system.
 * @param renderer The main SDL renderer used for texture loading.
 * @return SDL_APP_SUCCESS on successful initialization, SDL_APP_FAILURE otherwise.
 */
SDL_AppResult init_base(SDL_Renderer *renderer);