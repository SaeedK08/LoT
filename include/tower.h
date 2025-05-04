#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "../include/entity.h"
#include "../include/camera.h"
#include "../include/common.h"

typedef struct tower Tower;

/**
 * @brief Gets the bounds of a specific tower
 * @param towerIndex The index of the tower to get the position of
 * @return An SDL_FRect for the tower
 */
SDL_FRect getTowerPos(int towerIndex);

/**
 * @brief Initializes the tower system, loads tower textures, and registers the tower entity.
 * @param renderer The main SDL renderer used for loading textures.
 * @return SDL_APP_SUCCESS on successful initialization, SDL_APP_FAILURE otherwise.
 */
SDL_AppResult init_tower(SDL_Renderer *renderer);

void damageTower(float tower_posx);
void destroyTower(int towerIndex);