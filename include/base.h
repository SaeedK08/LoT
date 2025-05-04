#pragma once

#include "../include/common.h"
#include "../include/camera.h"
#include "../include/net_client.h"

typedef struct base Base;

// --- Public Function Declarations ---

/**
 * @brief Gets the bounds of a specific base
 * @param baseIndex The index of the base to get the position of
 * @return An SDL_FRect for the base
 */
SDL_FRect getBasePos(int baseIndex);

/**
 * @brief Initializes the base entities, loads their textures, and registers the rendering system.
 * @param renderer The main SDL renderer used for texture loading.
 * @return SDL_APP_SUCCESS on successful initialization, SDL_APP_FAILURE otherwise.
 */
SDL_AppResult init_base(SDL_Renderer *renderer);

void damageBase(float base_posx);
void destroyBase(int baseIndex);
