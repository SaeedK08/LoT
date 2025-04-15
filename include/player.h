#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_rect.h>
#include "../include/entity.h"
#include "../include/camera.h"
#include "../include/map.h"

// Player sprite dimensions
#define PLAYER_WIDTH 48
#define PLAYER_HEIGHT 80

// Global variable storing the player's current world position
extern SDL_FPoint player_position;

// Function to initialize the player entity
void init_player(SDL_Renderer *renderer);