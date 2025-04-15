#include "../include/camera.h"

// Define and initialize the global camera structure using constants
Camera camera = {400, 400, LOGICAL_WIDTH, LOGICAL_HEIGHT};

// Update function called each frame for the camera entity
static void update(float delta_time)
{
  // Calculate target position to center the middle of the player sprite
  float targetX = (player_position.x + PLAYER_WIDTH / 2.0f) - camera.w / 2.0f;
  float targetY = (player_position.y + PLAYER_HEIGHT / 2.0f) - camera.h / 2.0f;

  // Smoothly interpolate camera position towards the target position
  camera.x += (targetX - camera.x) * CAMERA_SMOOTH_SPEED * delta_time;
  camera.y += (targetY - camera.y) * CAMERA_SMOOTH_SPEED * delta_time;

  // Clamp camera position to stay within map boundaries AFTER interpolation
  if (camera.x < 0)
  {
    camera.x = 0;
  }
  if (camera.y < 0)
  {
    camera.y = 0;
  }
  if (camera.x + camera.w > MAP_WIDTH)
  {
    camera.x = MAP_WIDTH - camera.w;
  }
  if (camera.y + camera.h > MAP_HEIGHT)
  {
    camera.y = MAP_HEIGHT - camera.h;
  }
}

void init_camera(SDL_Renderer *renderer)
{
  Entity camera_entity = {
      .name = "camera",
      .update = update,
  };

  create_entity(camera_entity);
}