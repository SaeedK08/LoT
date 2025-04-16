#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "../include/entity.h"
#include "../include/cute_tiled.h"
#include "../include/camera.h"

#define MAP_WIDTH 3200
#define MAP_HEIGHT 1760

typedef struct Texture
{
  SDL_Texture *texture;
  int firstgid;
  int tilecount;
  int tileset_width;
  int tileset_height;
  struct Texture *next;
} Texture;

SDL_AppResult init_map(SDL_Renderer *renderer);