#define CUTE_TILED_IMPLEMENTATION
#include "../include/map.h"

static cute_tiled_map_t *map = NULL; // Loaded Tiled map data
static Texture *texture = NULL;      // Linked list of tileset textures & info

static void cleanup()
{
  // Free the linked list of Texture structs and associated SDL_Textures
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
      SDL_free(current_texture); // Free the Texture struct itself
      current_texture = next_texture;
    }
    texture = NULL;
  }

  // Free the Tiled map data
  if (map)
  {
    cute_tiled_free_map(map);
    map = NULL;
  }
  SDL_Log("Map has been cleaned up.");
}

static void render(SDL_Renderer *renderer)
{
  if (!map || !texture)
    return; // Don't render if map or textures aren't loaded

  cute_tiled_layer_t *temp_layer = map->layers; // Start from the first layer

  // Loop through all layers in the map
  while (temp_layer)
  {
    // Skip non-tile layers or invisible layers
    if (!temp_layer->data || !temp_layer->visible)
    {
      temp_layer = temp_layer->next;
      continue;
    }

    // Iterate through map grid
    for (int i = 0; i < map->height; i++)
    {
      for (int j = 0; j < map->width; j++)
      {
        // Get the tile GID from the layer data
        int tile_id = temp_layer->data[i * map->width + j];
        if (tile_id == 0) // Skip empty tiles (GID 0)
          continue;

        // Find the correct texture (tileset) for this tile GID
        Texture *temp_texture_node = texture;
        Texture *texture_to_use = NULL;
        while (temp_texture_node)
        {
          // Check if GID is within this tileset's range
          if (tile_id >= temp_texture_node->firstgid &&
              tile_id < temp_texture_node->firstgid + temp_texture_node->tilecount) // Use < for upper bound
          {
            texture_to_use = temp_texture_node;
            break;
          }
          temp_texture_node = temp_texture_node->next;
        }

        if (!texture_to_use || !texture_to_use->texture)
        {
          // Log warning if a tile GID has no corresponding loaded texture
          SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[%s] No texture found or loaded for GID %d.", __func__, tile_id);
          continue; // Skip if no texture found or texture failed to load
        }

        // Calculate source rect coordinates within the tileset image
        int tileset_columns = texture_to_use->tileset_width / map->tilewidth;
        int local_tile_id = tile_id - texture_to_use->firstgid; // ID relative to tileset
        SDL_FRect src = {
            (float)((local_tile_id % tileset_columns) * map->tilewidth),
            (float)((local_tile_id / tileset_columns) * map->tileheight),
            (float)map->tilewidth,
            (float)map->tileheight};

        // Calculate destination rect on screen, adjusted by camera
        SDL_FRect dst = {
            (float)(j * map->tilewidth - camera.x),
            (float)(i * map->tileheight - camera.y),
            (float)map->tilewidth,
            (float)map->tileheight};

        // Render the tile
        SDL_RenderTexture(renderer, texture_to_use->texture, &src, &dst);
      }
    }
    temp_layer = temp_layer->next; // Move to next layer
  }
}

SDL_AppResult init_map(SDL_Renderer *renderer)
{
  const char map_path[] = "./resources/Map/tiledMap.json";
  // Load the Tiled map file
  map = cute_tiled_load_map_from_file(map_path, NULL);
  if (!map)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Failed to load map '%s': %s", __func__, map_path, cute_tiled_error_reason);
    return SDL_APP_FAILURE;
  }

  // Prepare to load textures for each tileset into a linked list
  cute_tiled_tileset_t *current_tileset = map->tilesets;
  Texture *current_texture_node = NULL;
  Texture *head_texture_node = NULL; // Keep track of the head for cleanup on failure

  // Iterate through tilesets defined in the map file
  while (current_tileset)
  {
    // Create a new node for the texture linked list
    Texture *new_node = SDL_malloc(sizeof(Texture));
    if (!new_node)
    {
      // Standardized logging
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Failed to allocate memory for texture node.", __func__);
      texture = head_texture_node;
      cleanup();
      return SDL_APP_FAILURE;
    }

    // Initialize members
    memset(new_node, 0, sizeof(Texture)); // Clear the struct
    new_node->firstgid = current_tileset->firstgid;
    new_node->tilecount = current_tileset->tilecount;
    new_node->tileset_width = current_tileset->imagewidth;
    new_node->tileset_height = current_tileset->imageheight;

    const char *image_path = current_tileset->image.ptr;

    // Load the actual SDL_Texture
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

    // Link the new node into the list
    if (!head_texture_node) // If this is the first node
    {
      head_texture_node = new_node;
      current_texture_node = head_texture_node;
    }
    else // Append to the end of the list
    {
      current_texture_node->next = new_node;
      current_texture_node = new_node;
    }

    current_tileset = current_tileset->next; // Move to next tileset in Tiled data
  }

  // Assign the completed list head to the static variable
  texture = head_texture_node;

  Entity map_e = {
      .name = "map",
      .render = render,
      .cleanup = cleanup};

  // Check result of create_entity
  if (create_entity(map_e) == SDL_APP_FAILURE)
  {
    cleanup(); // Cleanup map resources if entity creation failed
    return SDL_APP_FAILURE;
  }

  return SDL_APP_SUCCESS;
}
