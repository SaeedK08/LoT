#include "../include/map.h"

#define CUTE_TILED_IMPLEMENTATION
#include "../include/cute_tiled.h"

// --- Internal Structures ---

/**
 * @brief Internal structure representing a loaded tileset texture and its metadata.
 * Used within the MapState.
 */
typedef struct TilesetTexture
{
  SDL_Texture *texture;
  int firstgid;
  int tilecount;
  int tileset_width_px;  // Width of the tileset image in pixels
  int tileset_height_px; // Height of the tileset image in pixels
  int columns;           // Number of tile columns in the tileset image
  struct TilesetTexture *next;
} TilesetTexture;

/**
 * @brief Internal state for the Map module.
 */
struct MapState_s
{
  cute_tiled_map_t *map_data;       /**< Parsed Tiled map data. */
  TilesetTexture *tileset_textures; /**< Linked list of loaded tileset textures. */
};

// --- Static Callback Functions (for EntityManager) ---

/**
 * @brief Internal function to render all visible tile layers of the map.
 * @param map_state The internal state of the map module.
 * @param state The main application state.
 */
static void Internal_MapRenderImplementation(MapState map_state, AppState *state)
{
  if (!map_state || !map_state->map_data || !map_state->tileset_textures || !state->camera_state)
  {
    return;
  }

  cute_tiled_map_t *map = map_state->map_data;
  CameraState camera = state->camera_state;
  float cam_x = Camera_GetX(camera);
  float cam_y = Camera_GetY(camera);
  float cam_w = Camera_GetWidth(camera);
  float cam_h = Camera_GetHeight(camera);

  // Determine the range of tiles visible on screen
  int start_col = (int)floorf(cam_x / map->tilewidth);
  int end_col = (int)ceilf((cam_x + cam_w) / map->tilewidth);
  int start_row = (int)floorf(cam_y / map->tileheight);
  int end_row = (int)ceilf((cam_y + cam_h) / map->tileheight);

  // Clamp tile range to map boundaries
  start_col = CLAMP(start_col, 0, map->width - 1);
  end_col = CLAMP(end_col, 0, map->width);
  start_row = CLAMP(start_row, 0, map->height - 1);
  end_row = CLAMP(end_row, 0, map->height);

  cute_tiled_layer_t *layer = map->layers;
  while (layer)
  {
    // Only render visible tile layers
    if (strcmp(layer->type.ptr, "tilelayer") == 0 && layer->visible && layer->data)
    {
      for (int y = start_row; y < end_row; ++y)
      {
        for (int x = start_col; x < end_col; ++x)
        {
          int tile_index = y * map->width + x;
          int gid = layer->data[tile_index];

          if (gid == 0)
            continue; // Skip empty tiles

          // Find the correct tileset texture for the given GID
          TilesetTexture *tex_node = map_state->tileset_textures;
          TilesetTexture *tileset_to_use = NULL;
          while (tex_node)
          {
            if (gid >= tex_node->firstgid && gid < tex_node->firstgid + tex_node->tilecount)
            {
              tileset_to_use = tex_node;
              break;
            }
            tex_node = tex_node->next;
          }

          if (!tileset_to_use || !tileset_to_use->texture)
            continue; // Skip if tileset not found

          // Calculate source rect from the tileset image
          int local_id = gid - tileset_to_use->firstgid;
          SDL_FRect src_rect = {
              .x = (float)((local_id % tileset_to_use->columns) * map->tilewidth),
              .y = (float)((local_id / tileset_to_use->columns) * map->tileheight),
              .w = (float)map->tilewidth,
              .h = (float)map->tileheight};

          // Calculate destination rect on the screen, adjusted for camera position
          SDL_FRect dst_rect = {
              .x = (float)(x * map->tilewidth) - cam_x,
              .y = (float)(y * map->tileheight) - cam_y,
              .w = (float)map->tilewidth,
              .h = (float)map->tileheight};

          SDL_RenderTexture(state->renderer, tileset_to_use->texture, &src_rect, &dst_rect);
        }
      }
    }
    layer = layer->next;
  }
}

/**
 * @brief Internal function to clean up map resources.
 * @param map_state The internal state of the map module.
 */
static void Internal_MapCleanupImplementation(MapState map_state)
{
  if (!map_state)
    return;

  // Free the linked list of TilesetTexture structs and associated SDL_Textures
  TilesetTexture *current_texture = map_state->tileset_textures;
  TilesetTexture *next_texture = NULL;
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
  map_state->tileset_textures = NULL;

  // Free the Tiled map data
  if (map_state->map_data)
  {
    cute_tiled_free_map(map_state->map_data);
    map_state->map_data = NULL;
  }
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Map resources cleaned up.");
}

/**
 * @brief Wrapper function conforming to EntityFunctions.render signature.
 * @param manager The EntityManager instance.
 * @param state Pointer to the main AppState.
 */
static void map_render_callback(EntityManager manager, AppState *state)
{
  (void)manager; // Manager instance is not used in this specific implementation
  Internal_MapRenderImplementation(state->map_state, state);
}

/**
 * @brief Wrapper function conforming to EntityFunctions.cleanup signature.
 * @param manager The EntityManager instance.
 * @param state Pointer to the main AppState.
 */
static void map_cleanup_callback(EntityManager manager, AppState *state)
{
  (void)manager; // Manager instance is not used in this specific implementation
  Internal_MapCleanupImplementation(state->map_state);
  if (state)
  {
    state->map_state = NULL; // Indicate cleanup happened
  }
}

// --- Public API Function Implementations ---

MapState Map_Init(AppState *state)
{
  if (!state || !state->renderer || !state->entity_manager)
  {
    SDL_SetError("Invalid AppState or missing renderer/entity_manager for Map_Init");
    return NULL;
  }

  const char map_path[] = "./resources/Map/tiledMap.json";

  // --- Allocate State ---
  MapState map_state = (MapState)SDL_calloc(1, sizeof(struct MapState_s));
  if (!map_state)
  {
    SDL_OutOfMemory();
    return NULL;
  }

  // --- Load Tiled Map Data ---
  map_state->map_data = cute_tiled_load_map_from_file(map_path, NULL);
  if (!map_state->map_data)
  {
    SDL_SetError("Failed to load map '%s': %s", map_path, cute_tiled_error_reason);
    SDL_free(map_state);
    return NULL;
  }

  // --- Load Tileset Textures ---
  cute_tiled_tileset_t *tiled_tileset = map_state->map_data->tilesets;
  TilesetTexture *list_head = NULL;
  TilesetTexture *list_tail = NULL;

  while (tiled_tileset)
  {
    TilesetTexture *new_node = (TilesetTexture *)SDL_malloc(sizeof(TilesetTexture));
    if (!new_node)
    {
      SDL_OutOfMemory();
      Internal_MapCleanupImplementation(map_state); // Cleanup already loaded stuff
      SDL_free(map_state);
      return NULL;
    }
    memset(new_node, 0, sizeof(TilesetTexture));

    new_node->firstgid = tiled_tileset->firstgid;
    new_node->tilecount = tiled_tileset->tilecount;
    new_node->tileset_width_px = tiled_tileset->imagewidth;
    new_node->tileset_height_px = tiled_tileset->imageheight;
    new_node->columns = tiled_tileset->columns;
    new_node->next = NULL;

    const char *image_path = tiled_tileset->image.ptr;
    if (!image_path)
    {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Map Init] Tileset (gid %d) has no image path.", tiled_tileset->firstgid);
      SDL_free(new_node);
      Internal_MapCleanupImplementation(map_state); // Cleanup
      SDL_free(map_state);
      return NULL;
    }

    new_node->texture = IMG_LoadTexture(state->renderer, image_path);
    if (!new_node->texture)
    {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Map Init] Failed to load texture '%s': %s", image_path, SDL_GetError());
      SDL_free(new_node);
      Internal_MapCleanupImplementation(map_state); // Cleanup
      SDL_free(map_state);
      return NULL;
    }
    SDL_SetTextureScaleMode(new_node->texture, SDL_SCALEMODE_NEAREST);

    // Append to linked list
    if (!list_head)
    {
      list_head = new_node;
    }
    else
    {
      list_tail->next = new_node;
    }
    list_tail = new_node;

    tiled_tileset = tiled_tileset->next;
  }
  map_state->tileset_textures = list_head;

  // --- Register with EntityManager ---
  EntityFunctions map_entity_funcs = {
      .name = "map",
      .render = map_render_callback,
      .cleanup = map_cleanup_callback,
      .update = NULL,
      .handle_events = NULL};

  if (!EntityManager_Add(state->entity_manager, &map_entity_funcs))
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Map Init] Failed to add map entity to manager: %s", SDL_GetError());
    Internal_MapCleanupImplementation(map_state); // Cleanup map resources
    SDL_free(map_state);                          // Free the state struct itself
    return NULL;
  }

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Map module initialized and entity registered.");
  return map_state;
}

void Map_Destroy(MapState map_state)
{
  // Cleanup callback handles texture and map data destruction.
  if (map_state)
  {
    // Prevent dangling pointers after cleanup callback potentially ran via EntityManager
    map_state->map_data = NULL;
    map_state->tileset_textures = NULL;
    SDL_free(map_state);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "MapState container destroyed.");
  }
}

int Map_GetWidthPixels(MapState map_state)
{
  if (map_state && map_state->map_data)
  {
    return map_state->map_data->width * map_state->map_data->tilewidth;
  }
  return 0;
}

int Map_GetHeightPixels(MapState map_state)
{
  if (map_state && map_state->map_data)
  {
    return map_state->map_data->height * map_state->map_data->tileheight;
  }
  return 0;
}
