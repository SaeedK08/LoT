#pragma once
#include "../include/entity.h"
#include "../include/player.h"
#include "../include/map.h"

#define CAMERA_VIEW_WIDTH 600.0f
#define CAMERA_VIEW_HEIGHT 300.0f

// Represents the game camera's view area
typedef struct
{
  float x, y, w, h;
} Camera;

// Global camera instance
extern Camera camera;

SDL_AppResult init_camera(SDL_Renderer *renderer);