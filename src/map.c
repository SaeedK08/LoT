#define CUTE_TILED_IMPLEMENTATION
#include "../include/map.h"

// --- Module Variables ---

static cute_tiled_map_t *map = NULL;
static Texture *texture = NULL;

// --- Static Helper Functions ---

/**
 * @brief Frees all resources associated with the map, including Tiled data and textures.
 * @return void
 */
static void cleanup()
{
  if (texture)
  {
    Texture *current_texture = texture;
    Texture *next_texture = NULL;

    while (current_texture)
    {
      next_texture = current_texture->next;
      if (current_texture->texture)
      {
        SDL_DestroyTexture(current_texture->texture);
      }
      SDL_free(current_texture);
      current_texture = next_texture;
    }
    texture = NULL;
  }

  if (map)
  {
    cute_tiled_free_map(map);
    map = NULL;
  }
  SDL_Log("Map has been cleaned up.");
}

/**
 * @brief Renders all visible tile layers of the map, adjusted by the camera position.
 * @param state The application state containing the renderer.
 * @return void
 */
static void render(AppState *state)
{
  // Don't render if map or textures aren't loaded
  if (!map || !texture)
    return;

  cute_tiled_layer_t *temp_layer = map->layers;

  while (temp_layer)
  {
    // Skip non-tile layers or invisible layers
    if (!temp_layer->data || !temp_layer->visible)
    {
      temp_layer = temp_layer->next;
      continue;
    }

    // --- Render Layer Tiles ---
    for (int i = 0; i < map->height; i++)
    {
      for (int j = 0; j < map->width; j++)
      {
        int tile_id = temp_layer->data[i * map->width + j];
        // Skip empty tiles (GID 0)
        if (tile_id == 0)
          continue;

        Texture *temp_texture_node = texture;
        Texture *texture_to_use = NULL;
        while (temp_texture_node)
        {
          // Use < for upper bound
          if (tile_id >= temp_texture_node->firstgid &&
              tile_id < temp_texture_node->firstgid + temp_texture_node->tilecount)
          {
            texture_to_use = temp_texture_node;
            break;
          }
          temp_texture_node = temp_texture_node->next;
        }

        // Skip if no texture found or texture failed to load
        if (!texture_to_use || !texture_to_use->texture)
        {
          SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[%s] No texture found or loaded for GID %d.", __func__, tile_id);
          continue;
        }

        int tileset_columns = texture_to_use->tileset_width / map->tilewidth;
        int local_tile_id = tile_id - texture_to_use->firstgid; // ID relative to tileset
        SDL_FRect src = {
            (float)((local_tile_id % tileset_columns) * map->tilewidth),
            (float)((local_tile_id / tileset_columns) * map->tileheight),
            (float)map->tilewidth,
            (float)map->tileheight};

        SDL_FRect dst = {
            (float)(j * map->tilewidth - camera.x),
            (float)(i * map->tileheight - camera.y),
            (float)map->tilewidth,
            (float)map->tileheight};

        SDL_RenderTexture(state->renderer, texture_to_use->texture, &src, &dst);
      }
    }
    temp_layer = temp_layer->next;
  }
}

// --- Public API Function Implementations ---

SDL_AppResult init_map(SDL_Renderer *renderer)
{
  // --- Load Tiled Map ---
  const char map_path[] = "./resources/Map/tiledMap.json";
  map = cute_tiled_load_map_from_file(map_path, NULL);
  if (!map)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Failed to load map '%s': %s", __func__, map_path, cute_tiled_error_reason);
    return SDL_APP_FAILURE;
  }

  // --- Load Tileset Textures ---
  cute_tiled_tileset_t *current_tileset = map->tilesets;
  Texture *current_texture_node = NULL;
  Texture *head_texture_node = NULL; // Keep track of the head for cleanup on failure

  while (current_tileset)
  {
    Texture *new_node = SDL_malloc(sizeof(Texture));
    if (!new_node)
    {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Failed to allocate memory for texture node.", __func__);
      texture = head_texture_node;
      cleanup();
      return SDL_APP_FAILURE;
    }

    memset(new_node, 0, sizeof(Texture));
    new_node->firstgid = current_tileset->firstgid;
    new_node->tilecount = current_tileset->tilecount;
    new_node->tileset_width = current_tileset->imagewidth;
    new_node->tileset_height = current_tileset->imageheight;

    const char *image_path = current_tileset->image.ptr;

    new_node->texture = IMG_LoadTexture(renderer, image_path);
    if (!new_node->texture)
    {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Failed to load texture '%s': %s", __func__, image_path, SDL_GetError());
      SDL_free(new_node);
      texture = head_texture_node;
      cleanup();
      return SDL_APP_FAILURE;
    }
    else
    {
      // Use nearest-neighbor scaling for pixel art
      SDL_SetTextureScaleMode(new_node->texture, SDL_SCALEMODE_NEAREST);
    }

    if (!head_texture_node)
    {
      head_texture_node = new_node;
      current_texture_node = head_texture_node;
    }
    else
    {
      current_texture_node->next = new_node;
      current_texture_node = new_node;
    }

    current_tileset = current_tileset->next;
  }

  texture = head_texture_node;

  // --- Create Map Entity ---
  Entity map_e = {
      .name = "map",
      .render = render,
      .cleanup = cleanup};

  if (create_entity(map_e) == SDL_APP_FAILURE)
  {
    // Cleanup map resources if entity creation failed
    cleanup();
    return SDL_APP_FAILURE;
  }

  return SDL_APP_SUCCESS;
}