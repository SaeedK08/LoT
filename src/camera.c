#include "../include/camera.h"

// --- Internal Structures ---

/**
 * @brief Internal state for the Camera module.
 */
struct CameraState_s
{
  float x; /**< World X coordinate of the camera's top-left corner. */
  float y; /**< World Y coordinate of the camera's top-left corner. */
  float w; /**< Width of the camera's view. */
  float h; /**< Height of the camera's view. */
};

// --- Static Callback Functions (for EntityManager) ---

/**
 * @brief Internal function to update the camera state based on target and map bounds.
 * @param camera_state The internal state of the camera module.
 * @param state The main application state.
 */
static void Internal_CameraUpdateImplementation(CameraState camera_state, AppState *state)
{
  if (!camera_state || !state || !state->player_manager || !state->map_state)
  {
    return;
  }

  PlayerManager player_mgr = state->player_manager;
  MapState map_state = state->map_state;

  SDL_FPoint player_pos;
  if (PlayerManager_GetLocalPlayerPosition(player_mgr, &player_pos))
  {
    // Center the camera on the player position
    camera_state->x = player_pos.x - camera_state->w / 2.0f;
    camera_state->y = player_pos.y - camera_state->h / 2.0f;

    int map_width_px = Map_GetWidthPixels(map_state);
    int map_height_px = Map_GetHeightPixels(map_state);

    // Clamp camera to map boundaries
    camera_state->x = CLAMP(camera_state->x, 0.0f, (float)map_width_px - camera_state->w);
    camera_state->y = CLAMP(camera_state->y, 0.0f, (float)map_height_px - camera_state->h);
  }
}

/**
 * @brief Wrapper function conforming to EntityFunctions.update signature.
 * @param manager The EntityManager instance.
 * @param state Pointer to the main AppState.
 */
static void camera_update_callback(EntityManager manager, AppState *state)
{
  (void)manager; // Manager instance is not used in this specific implementation
  Internal_CameraUpdateImplementation(state->camera_state, state);
}

/**
 * @brief Wrapper function conforming to EntityFunctions.cleanup signature.
 * @param manager The EntityManager instance.
 * @param state Pointer to the main AppState.
 */
static void camera_cleanup_callback(EntityManager manager, AppState *state)
{
  (void)manager; // Manager instance is not used in this specific implementation
  if (state)
  {
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Camera entity cleanup callback triggered.");
  }
}

// --- Public API Function Implementations ---

CameraState Camera_Init(AppState *state)
{
  if (!state || !state->entity_manager)
  {
    SDL_SetError("Invalid AppState or missing entity_manager for Camera_Init");
    return NULL;
  }

  // --- Allocate and Initialize State ---
  CameraState camera_state = (CameraState)SDL_calloc(1, sizeof(struct CameraState_s));
  if (!camera_state)
  {
    SDL_OutOfMemory();
    return NULL;
  }
  camera_state->x = 0.0f;
  camera_state->y = 0.0f;
  camera_state->w = CAMERA_VIEW_WIDTH;
  camera_state->h = CAMERA_VIEW_HEIGHT;

  // --- Register with EntityManager ---
  EntityFunctions camera_entity_funcs = {
      .name = "camera",
      .update = camera_update_callback,
      .cleanup = camera_cleanup_callback,
      .render = NULL,
      .handle_events = NULL};

  if (!EntityManager_Add(state->entity_manager, &camera_entity_funcs))
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Camera Init] Failed to add camera entity to manager: %s", SDL_GetError());
    SDL_free(camera_state);
    return NULL;
  }

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Camera module initialized and entity registered.");
  return camera_state;
}

void Camera_Destroy(CameraState camera_state)
{
  if (camera_state)
  {
    SDL_free(camera_state);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "CameraState container destroyed.");
  }
}

void Camera_Update(CameraState camera_state, AppState *state)
{
  Internal_CameraUpdateImplementation(camera_state, state);
}

float Camera_GetX(CameraState camera_state)
{
  return camera_state ? camera_state->x : 0.0f;
}

float Camera_GetY(CameraState camera_state)
{
  return camera_state ? camera_state->y : 0.0f;
}

float Camera_GetWidth(CameraState camera_state)
{
  return camera_state ? camera_state->w : 0.0f;
}

float Camera_GetHeight(CameraState camera_state)
{
  return camera_state ? camera_state->h : 0.0f;
}
