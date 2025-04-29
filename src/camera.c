#include "../include/camera.h"

// Define and initialize the global camera structure
Camera camera = {0, 0, CAMERA_VIEW_WIDTH, CAMERA_VIEW_HEIGHT}; // Default view size

static void update(AppState *state)
{
  (void)state;

  SDL_FPoint player_position = funcGetPlayerPos();
  // Center the camera on the player position
  camera.x = player_position.x - camera.w / 2.0f;
  camera.y = player_position.y - camera.h / 2.0f;

  // Clamp camera position to stay within map boundaries
  if (camera.x < 0)
    camera.x = 0;
  if (camera.y < 0)
    camera.y = 0;

  // Prevent camera showing area outside the map borders
  if (camera.x + camera.w > MAP_WIDTH)
    camera.x = MAP_WIDTH - camera.w;
  if (camera.y + camera.h > MAP_HEIGHT)
    camera.y = MAP_HEIGHT - camera.h;
}

SDL_AppResult init_camera(SDL_Renderer *renderer)
{
  (void)renderer;

  Entity camera_entity = {
      .name = "camera",
      .update = update};

  // Create the entity and check the result
  if (create_entity(camera_entity) == SDL_APP_FAILURE)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Failed to create camera entity.", __func__);
    return SDL_APP_FAILURE;
  }

  return SDL_APP_SUCCESS;
}