#pragma once

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include "../include/entity.h"

typedef enum
{
  MSG_TYPE_INVALID = 0,   // Should not be sent
  MSG_TYPE_C_HELLO = 1,   // Client -> Server: Initial connection message
  MSG_TYPE_S_WELCOME = 2, // Server -> Client: Acknowledgment of connection
} MessageType;

typedef struct AppState
{
  SDL_Window *window;
  SDL_Renderer *renderer;
  float last_tick;
  float current_tick;
  float delta_time;
} AppState;