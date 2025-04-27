#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "../include/entity.h"
#include "../include/camera.h"
#include "../include/map.h"
#include "../include/net_client.h"
#include "../include/common.h"
#include "../include/attack.h"

// Player sprite dimensions
#define PLAYER_WIDTH 35
#define PLAYER_HEIGHT 67
#define ATTACK_RANGE 100.0f

// Global variable storing the player's current world position
extern SDL_FPoint player_position;

SDL_AppResult init_player(SDL_Renderer *renderer);

void render_remote_players(SDL_Renderer *renderer);