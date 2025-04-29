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
#define FRAME_WIDTH 48.0f
#define FRAME_HEIGHT 80.0f
#define IDLE_ROW_Y 0.0f
#define WALK_ROW_Y 80.0f
#define NUM_IDLE_FRAMES 6
#define NUM_WALK_FRAMES 6
#define TIME_PER_FRAME 0.1f

typedef struct Player Player;

// Global variable storing the player's current world position
SDL_FPoint funcGetPlayerPos();

Player *init_player(SDL_Renderer *renderer, int myIndex);

// void render_remote_players(SDL_Renderer *renderer);