#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "../include/entity.h"
#include "../include/camera.h"
#include "../include/common.h"

#define MAX_TOWERS 4

/**
 * @brief Initializes the tower system, loads tower textures, and registers the tower entity.
 * @param renderer The main SDL renderer used for loading textures.
 * @return SDL_APP_SUCCESS on successful initialization, SDL_APP_FAILURE otherwise.
 */
SDL_AppResult init_tower(SDL_Renderer *renderer);