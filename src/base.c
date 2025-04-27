
#include "../include/base.h"
#define MAX_BASES 2

// Base position
static SDL_FPoint base_positions[MAX_BASES] = {
    {100.0f, 850.0f},
    {3100.0f, 850.0f}};

static SDL_Texture *base_textures[MAX_BASES] = {NULL};

// Base Dimensions
static const float BASE_WIDTH = 200.0f;
static const float BASE_HEIGHT = 266.0f

static void cleanup()
{

  for (int i = 0; i < MAX_BASES; i++)
  {

    if (base_textures[i])
    {
      SDL_DestroyTexture(base_textures[i]);
      base_textures[i] = NULL;
      SDL_Log("All base texture cleaned up.");
    }
  }
}

static void render(SDL_Renderer *renderer)
{
  // Check that the texture has loaded
  for (int i = 0; i < MAX_BASES; i++)
  {

    if (!base_textures[i])
      continue;

  // Calculate screen position based on world position and camera
    float screen_x = base_positions[i].x - camera.x - BASE_WIDTH / 2.0f;
    float screen_y = base_positions[i].y - camera.y - BASE_HEIGHT / 2.0f;

    SDL_FRect base_dest_rect = {
        screen_x,
        screen_y,
        BASE_WIDTH,
        BASE_HEIGHT};

    // Render castle
    SDL_RenderTexture(renderer, base_textures[i], NULL, &base_dest_rect);
  }
}

SDL_AppResult init_base(SDL_Renderer *renderer)
{
  const char *paths[MAX_BASES] = {
      "./resources/Sprites/Blue_Team/Castle_Blue.png",
      "./resources/Sprites/Red_Team/Castle_Red.png"};

  for (int i = 0; i < MAX_BASES; i++)
  {

    base_textures[i] = IMG_LoadTexture(renderer, paths[i]);

    if (!base_textures[i])
    {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Failed to load base texture '%s': %s", __func__, paths[i], SDL_GetError());
      return SDL_APP_FAILURE;
    }
    SDL_SetTextureScaleMode(base_textures[i], SDL_SCALEMODE_NEAREST);
  }

  Entity base_entity = {
      .name = "base",
      .cleanup = cleanup,
      .render = render};

  if (create_entity(base_entity) == SDL_APP_FAILURE)
  {
    // SDL_DestroyTexture(base_textures[i]);

    for (int i = 0; i < MAX_BASES; i++)
    {
      cleanup();
    }
    // base_texture = NULL;
    return SDL_APP_FAILURE;
  }

  SDL_Log("Tower entity initialized.");
  return SDL_APP_SUCCESS;
}