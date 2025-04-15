#pragma once
#include <SDL3/SDL_rect.h>
#include "../include/entity.h"
#include "../include/player.h"
#include "../include/map.h"
#include "../include/common.h"

// Represents the game camera's view area and position
typedef struct
{
  float x, y, w, h; // Position (x, y) and dimensions (width, height)
} Camera;

// Global camera instance
extern Camera camera;

// Function to initialize the camera system
void init_camera(SDL_Renderer *renderer);