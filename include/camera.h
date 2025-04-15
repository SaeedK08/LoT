#pragma once
#include "../include/entity.h"
#include "../include/player.h"
#include "../include/map.h"

// Represents the game camera's view area
typedef struct
{
  float x, y, w, h; // Position (x, y) and dimensions (width, height)
} Camera;

// Global camera instance
extern Camera camera;

// Function to initialize the camera system
void init_camera(SDL_Renderer *renderer);