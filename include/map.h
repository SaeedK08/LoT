#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "../include/entity.h"
#include "../include/cute_tiled.h"
#include "../include/camera.h"

/**
 * @brief Represents a node in a linked list storing loaded tileset textures and metadata.
 * Used internally by the map rendering system.
 */
typedef struct Texture
{
  SDL_Texture *texture; /**< The loaded SDL texture for the tileset image. */
  int firstgid;         /**< The first global tile ID in this tileset. */
  int tilecount;        /**< The total number of tiles in this tileset. */
  int tileset_width;    /**< The width of the tileset image in pixels. */
  int tileset_height;   /**< The height of the tileset image in pixels. */
  struct Texture *next; /**< Pointer to the next Texture node in the list, or NULL if last. */
} Texture;

/**
 * @brief Initializes the map system, loads the Tiled map data and associated tileset textures.
 * @param renderer The main SDL renderer used for loading textures.
 * @return SDL_APP_SUCCESS on successful initialization, SDL_APP_FAILURE otherwise.
 */
SDL_AppResult init_map(SDL_Renderer *renderer);