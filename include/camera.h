#pragma once
#include "../include/entity.h"
#include "../include/player.h"
#include "../include/map.h"

#define CAMER_VIEW_WIDTH 600
#define CAMER_VIEW_HEIGHT 300

// Represents the game camera's view area
typedef struct
{
  float x, y, w, h;
} Camera;

// Global camera instance
extern Camera camera;

SDL_AppResult init_camera(SDL_Renderer *renderer);