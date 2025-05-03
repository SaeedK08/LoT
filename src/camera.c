#include "../include/camera.h"

// --- Static Variables ---
Camera camera = {0.0f, 0.0f, CAMERA_VIEW_WIDTH, CAMERA_VIEW_HEIGHT};

// --- Static Helper Functions ---

/**
 * @brief Updates the camera's position to follow the player, clamped within map boundaries.
 * @param state Pointer to the global application state (unused in this function).
 * @return void
 */
static void update(AppState *state)
{
  (void)state; // Explicitly mark state as unused.

  SDL_FPoint player_position = funcGetPlayerPos();

  // Only update camera if a valid player position is returned.
  // funcGetPlayerPos returns {-1, -1} if the local player isn't initialized yet.
  if (player_position.x != -1.0f || player_position.y != -1.0f)
  {
    // Center the camera view on the player's world position.
    camera.x = player_position.x - camera.w / 2.0f;
    camera.y = player_position.y - camera.h / 2.0f;

    // Clamp camera coordinates to ensure the view stays within the map limits.
    camera.x = fmaxf(0.0f, camera.x);                  // Prevent scrolling past the left edge.
    camera.y = fmaxf(0.0f, camera.y);                  // Prevent scrolling past the top edge.
    camera.x = fminf(camera.x, MAP_WIDTH - camera.w);  // Prevent scrolling past the right edge.
    camera.y = fminf(camera.y, MAP_HEIGHT - camera.h); // Prevent scrolling past the bottom edge.
  }
}

// --- Public API Function Implementations ---
SDL_AppResult init_camera(SDL_Renderer *renderer)
{
  (void)renderer; // Explicitly mark renderer as unused.

  // Avoid creating duplicate entities.
  if (find_entity("camera") != -1)
  {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Camera] Entity already exists.");
    return SDL_APP_SUCCESS; // Not a failure if it already exists.
  }

  Entity camera_entity = {
      .name = "camera",
      .update = update,
      .cleanup = NULL,
      .render = NULL,
      .handle_events = NULL};

  if (create_entity(camera_entity) == SDL_APP_FAILURE)
  {
    // Error logged within create_entity.
    return SDL_APP_FAILURE;
  }

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Camera] Camera entity initialized.");
  return SDL_APP_SUCCESS;
}