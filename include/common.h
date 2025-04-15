#pragma once
#include <SDL3/SDL.h>
#include "../include/entity.h"

// Used for SDL_SetRenderLogicalPresentation and Camera dimensions
#define LOGICAL_WIDTH 320
#define LOGICAL_HEIGHT 180

// Higher value = faster/less smooth camera movement
#define CAMERA_SMOOTH_SPEED 5.0f

// Structure to hold overall application state
typedef struct AppState
{
  SDL_Window *window;
  SDL_Renderer *renderer;
  float last_tick;
  float current_tick;
  float delta_time;
} AppState;