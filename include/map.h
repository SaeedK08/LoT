#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_rect.h>
#include <math.h>
#include "../include/entity.h"
#include "../include/cute_tiled.h"
#include "../include/camera.h"
#include "../include/common.h"

// Define pixel dimensions of the map
#define MAP_WIDTH 3200
#define MAP_HEIGHT 1760

// Maximum number of tileset textures expected
#define MAX_TILESET_TEXTURES 10

// Structure to hold loaded texture and associated tileset info
typedef struct TextureNode
{
  SDL_Texture *texture;
  int firstgid;
  int tilecount;
  int tileset_width;  // Width of the tileset image in pixels
  int tileset_height; // Height of the tileset image in pixels
  int columns;        // Number of columns in the tileset grid
  struct TextureNode *next;
} TextureNode;

// Function to initialize the map, returns 1 on success, 0 on failure
int init_map(SDL_Renderer *renderer);