#include "../include/camera.h"

// Define and initialize the global camera structure
Camera camera = {0, 0, 320, 180};

// Update function called each frame for the camera entity
static void update(float delta_time)
{
  // Center the camera on the player position
  camera.x = player_position.x - camera.w / 2;
  camera.y = player_position.y - camera.h / 2;

  // Clamp camera position to stay within map boundaries
  if (camera.x < 0)
    camera.x = 0;
  if (camera.y < 0)
    camera.y = 0;

  if (camera.x + camera.w > MAP_WIDTH)
    camera.x = MAP_WIDTH - camera.w; // Prevent camera going too far right
  if (camera.y + camera.h > MAP_HEIGHT)
    camera.y = MAP_HEIGHT - camera.h; // Prevent camera going too far down
}

// Initializes and registers the camera as an entity
void init_camera(SDL_Renderer *renderer)
{
  Entity camera_entity = {
      // Use a different local name to avoid conflict with the global 'camera' variable
      .name = "camera",
      .update = update, // Assign the update function
  };

  create_entity(camera_entity); // Add the camera entity to the game's entity list
}