#pragma once

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
// #include "../include/entity.h"

#define MAX_NAME_LENGTH 64
#define MAX_ENTITIES 100
extern int entities_count;

typedef enum
{
  MSG_TYPE_INVALID = 0,
  MSG_TYPE_C_HELLO = 1,
  MSG_TYPE_S_WELCOME = 2,
  MSG_TYPE_PLAYER_POS = 3
} MessageType;

typedef struct
{
  uint8_t client_id;
  SDL_FPoint position;
} PlayerPosUpdateData;

typedef struct AppState
{
  SDL_Window *window;
  SDL_Renderer *renderer;
  float last_tick;
  float current_tick;
  float delta_time;
} AppState;

typedef struct
{
  char name[MAX_NAME_LENGTH];
  void (*cleanup)(void);
  void (*handle_events)(void *, SDL_Event *);
  void (*update)(AppState *);
  void (*render)(AppState *);
} Entity;

extern Entity entities[MAX_ENTITIES];