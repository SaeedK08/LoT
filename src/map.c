#define CUTE_TILED_IMPLEMENTATION
#include "../include/map.h"

// Static variables for map data
static cute_tiled_map_t *map = NULL;
static TextureNode *texture_list_head = NULL;              // Head of linked list for cleanup
static TextureNode *g_texture_array[MAX_TILESET_TEXTURES]; // Array for faster lookup
static int g_texture_count = 0;                            // Number of textures loaded

// Cleanup function for map resources
static void cleanup()
{
  // Free the linked list of TextureNodes and their SDL_Textures
  TextureNode *current_texture = texture_list_head;
  while (current_texture)
  {
    TextureNode *next_texture = current_texture->next;
    if (current_texture->texture)
    {
      SDL_DestroyTexture(current_texture->texture);
    }
    SDL_free(current_texture); // Use SDL_free for memory allocated with SDL_malloc
    current_texture = next_texture;
  }
  texture_list_head = NULL;
  g_texture_count = 0; // Reset count

  // Free the Tiled map data structure
  if (map)
  {
    cute_tiled_free_map(map);
    map = NULL;
  }
}

// Render function for the map entity
static void render(SDL_Renderer *renderer)
{
  if (!map)
    return; // Don't render if map isn't loaded

  cute_tiled_layer_t *current_layer = map->layers;

  // Iterate through each layer in the Tiled map
  while (current_layer)
  {
    // Only process tile layers with data
    if (strcmp(current_layer->type.ptr, "tilelayer") == 0 && current_layer->data)
    {
      // --- Tile Culling Calculations ---
      int tile_w = map->tilewidth;
      int tile_h = map->tileheight;

      // Calculate the visible tile range based on camera
      int startCol = floorf(camera.x / tile_w);
      int endCol = ceilf((camera.x + camera.w) / tile_w);
      int startRow = floorf(camera.y / tile_h);
      int endRow = ceilf((camera.y + camera.h) / tile_h);

      // Clamp range to map boundaries
      if (startCol < 0)
        startCol = 0;
      if (startRow < 0)
        startRow = 0;
      if (endCol > map->width)
        endCol = map->width;
      if (endRow > map->height)
        endRow = map->height;

      // --- Render Only Visible Tiles ---
      for (int i = startRow; i < endRow; i++) // Loop through visible rows
      {
        for (int j = startCol; j < endCol; j++) // Loop through visible columns
        {
          int tile_index = i * map->width + j; // Calculate index in the layer data array
          int tile_id = current_layer->data[tile_index];

          if (tile_id == 0)
            continue; // Skip empty tiles

          TextureNode *texture_to_use = NULL;
          for (int tex_idx = 0; tex_idx < g_texture_count; ++tex_idx)
          {
            // Check if tile GID falls within the range of this tileset texture
            if (tile_id >= g_texture_array[tex_idx]->firstgid &&
                tile_id < g_texture_array[tex_idx]->firstgid + g_texture_array[tex_idx]->tilecount)
            {
              texture_to_use = g_texture_array[tex_idx];
              break; // Found the correct texture
            }
          }

          if (!texture_to_use || !texture_to_use->texture)
            continue; // Skip if no valid texture found

          // GID relative to the tileset's first GID
          int local_id = tile_id - texture_to_use->firstgid;

          // Calculate source rectangle in the tileset texture
          SDL_FRect src = {
              .x = (float)((local_id % texture_to_use->columns) * tile_w),
              .y = (float)((local_id / texture_to_use->columns) * tile_h),
              .w = (float)tile_w,
              .h = (float)tile_h};

          // Calculate destination rectangle on the screen (relative to camera)
          SDL_FRect dst = {
              .x = (float)(j * tile_w) - camera.x,
              .y = (float)(i * tile_h) - camera.y,
              .w = (float)tile_w,
              .h = (float)tile_h};

          // Render the tile
          SDL_RenderTexture(renderer, texture_to_use->texture, &src, &dst);
        }
      }
    }
    // Move to the next layer
    current_layer = current_layer->next;
  }
}

// Initializes the map entity, returns 1 on success, 0 on failure
int init_map(SDL_Renderer *renderer)
{
  const char map_path[] = "./resources/Map/tiledMap.json";
  map = cute_tiled_load_map_from_file(map_path, NULL);

  if (!map)
  {
    SDL_Log("Error loading map: %s. Error: %s", map_path, cute_tiled_error_reason);
    return 0; // Indicate failure
  }

  // Reset global texture state
  texture_list_head = NULL;
  g_texture_count = 0;
  TextureNode *current_node = NULL;

  // Load textures for each tileset defined in the map
  cute_tiled_tileset_t *current_tileset = map->tilesets;
  while (current_tileset)
  {
    if (g_texture_count >= MAX_TILESET_TEXTURES)
    {
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Maximum tileset textures (%d) reached, skipping remaining tilesets.", MAX_TILESET_TEXTURES);
      break;
    }

    // Allocate memory for the texture node structure
    TextureNode *new_node = SDL_malloc(sizeof(TextureNode));
    if (!new_node)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate memory for TextureNode.");
      cleanup(); // Clean up already loaded resources
      return 0;  // Indicate failure
    }
    new_node->next = NULL;

    // Load the tileset image file into an SDL_Texture
    new_node->texture = IMG_LoadTexture(renderer, current_tileset->image.ptr);
    if (!new_node->texture)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error loading texture for tileset '%s' (%s): %s",
                   current_tileset->name.ptr ? current_tileset->name.ptr : "Unnamed",
                   current_tileset->image.ptr ? current_tileset->image.ptr : "No Path",
                   SDL_GetError());
      SDL_free(new_node); // Free the allocated node since texture loading failed
      current_tileset = current_tileset->next;
      continue; // Skip this tileset
    }

    // Set texture scaling mode for pixel art
    SDL_SetTextureScaleMode(new_node->texture, SDL_SCALEMODE_NEAREST);

    // Store tileset metadata in our structure
    new_node->firstgid = current_tileset->firstgid;
    new_node->tilecount = current_tileset->tilecount;
    new_node->tileset_width = current_tileset->imagewidth;
    new_node->tileset_height = current_tileset->imageheight;
    new_node->columns = current_tileset->columns > 0 ? current_tileset->columns : 1; // Ensure columns > 0

    // Link the new node into the linked list
    if (texture_list_head == NULL)
    {
      texture_list_head = new_node; // First node
    }
    else
    {
      current_node->next = new_node; // Link previous node to this one
    }
    current_node = new_node; // Update the tail pointer

    // Add the new node pointer to the array for faster lookup
    g_texture_array[g_texture_count] = new_node;
    g_texture_count++;

    // Move to the next tileset in the Tiled data
    current_tileset = current_tileset->next;
  }

  if (g_texture_count == 0 && map->tilesets != NULL)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No tileset textures were loaded successfully.");
    cleanup();
    return 0; // Treat as failure if no textures loaded
  }

  // Define the map entity
  Entity map_e = {
      .name = "map",
      .update = NULL,
      .render = render,
      .cleanup = cleanup,
      .handle_events = NULL};

  // Register the map entity
  create_entity(map_e);

  SDL_Log("Map initialized successfully.");
  return 1; // Indicate success
}