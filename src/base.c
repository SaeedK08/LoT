#include "../include/base.h"

typedef struct base
{
  SDL_FPoint position;
  SDL_Texture *texture;
  SDL_Texture *destroyed;
  float health;
} Base;

Base *bases[MAX_BASES];

// --- Static Helper Functions ---

/**
 * @brief Cleans up resources used by the base system, specifically the textures.
 * @param void
 * @return void
 */
static void cleanup()
{
  for (int i = 0; i < MAX_BASES; i++)
  {
    if (bases[i]->texture)
    {
      SDL_DestroyTexture(bases[i]->texture);
      bases[i]->texture = NULL;
    }
  }
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Base] All base textures cleaned up.");
}

void damageBase(float base_posx)
{
  for (int i = 0; i < MAX_BASES; i++)
  {
    if (bases[i]->position.x - camera.x - BASE_WIDTH / 2.0f == base_posx)
    {
      SDL_Log("Base %d is getting damaged", i);
      bases[i]->health -= 10.0f;
      if (bases[i]->health <= 0)
      {
        destroyBase(i);
      }
    }
  }
}

// --- This function is for now unnecessary, but could be usefull for modularity when sending and receiving data ---
void destroyBase(int baseIndex)
{
  bases[baseIndex]->texture = bases[baseIndex]->destroyed;
  if ((baseIndex == BLUE_TEAM))
  {
    SDL_Log("Blue team has won");
    send_match_result(MSG_TYPE_BLUE_WON);
  }
  else
  {
    SDL_Log("Red team has won");
    send_match_result(MSG_TYPE_RED_WON);
  }
}

/**
 * @brief Renders the base sprites at their calculated screen positions.
 * @param state Pointer to the global application state, containing the renderer.
 * @return void
 */
static void render(AppState *state)
{
  for (int i = 0; i < MAX_BASES; i++)
  {
    // Skip rendering if the texture for this base isn't loaded.
    if (!bases[i]->texture)
      continue;

    // Calculate screen position based on world position, camera offset, and sprite dimensions.
    // Subtracting half width/height centers the sprite on its world position.
    float screen_x = bases[i]->position.x - camera.x - BASE_WIDTH / 2.0f;
    float screen_y = bases[i]->position.y - camera.y - BASE_HEIGHT / 2.0f;

    SDL_FRect base_dest_rect = {
        screen_x,
        screen_y,
        BASE_WIDTH,
        BASE_HEIGHT};

    // Render the base texture. NULL source rect uses the entire texture.
    SDL_RenderTexture(state->renderer, bases[i]->texture, NULL, &base_dest_rect);
  }
}
// --- Public API Function Implementations ---

SDL_AppResult init_base(SDL_Renderer *renderer)
{
  // --- Load Textures ---
  const char *paths[MAX_BASES] = {
      "./resources/Sprites/Blue_Team/Castle_Blue.png",
      "./resources/Sprites/Red_Team/Castle_Red.png"};
  const char *path_destroyed = {"./resources/Sprites/Castle_Destroyed.png"};

  for (int i = 0; i < MAX_BASES; i++)
  {
    bases[i] = SDL_malloc(sizeof(Base));
    bases[i]->texture = IMG_LoadTexture(renderer, paths[i]);
    bases[i]->destroyed = IMG_LoadTexture(renderer, path_destroyed);
    if (!bases[i]->texture)
    {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Base] Failed to load base texture '%s': %s", paths[i], SDL_GetError());
      // Clean up textures loaded so far before failing.
      cleanup();
      return SDL_APP_FAILURE;
    }
    if (!bases[i]->destroyed)
    {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Base] Failed to load destroyed base texture '%s': %s", paths[i], SDL_GetError());
      // Clean up textures loaded so far before failing.
      cleanup();
      return SDL_APP_FAILURE;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Base] Loaded texture: %s", paths[i]);

    // Use nearest neighbor scaling for pixel art.
    SDL_SetTextureScaleMode(bases[i]->texture, SDL_SCALEMODE_NEAREST);
    switch (i)
    {
    case 0:
      bases[i]->position = (SDL_FPoint){BLUE_BASE_POS_X, BUILDINGS_POS_Y}; // Position of base 1, Blue Team
      break;
    case 1:
      bases[i]->position = (SDL_FPoint){RED_BASE_POS_X, BUILDINGS_POS_Y}; // Position of base 2, Red Team
      break;
    default:
      break;
    }
    bases[i]->health = 200.0f;
  }

  // --- Register Entity ---
  // Register the base entity system if it hasn't been registered yet.
  if (find_entity("base") == -1)
  {
    Entity base_entity = {
        .name = "base",
        .update = NULL,
        .render = render,
        .cleanup = cleanup,
        .handle_events = NULL};

    if (create_entity(base_entity) == SDL_APP_FAILURE)
    {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Base] Failed to create base entity.");
      cleanup(); // Clean up textures if entity registration fails.
      return SDL_APP_FAILURE;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Base] Base entity created.");
  }

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Base] Base system initialized.");
  return SDL_APP_SUCCESS;
}

SDL_FRect getBasePos(int baseIndex)
{
  if (baseIndex < 0 || baseIndex >= bases)
  {
    return (SDL_FRect){0, 0, 0, 0}; // Return empty rect on error
  }

  // Calculate bounds based on center position and dimensions
  SDL_FRect bounds = {
      bases[baseIndex]->position.x - BASE_WIDTH / 2.0f,
      bases[baseIndex]->position.y - BASE_HEIGHT / 2.0f,
      BASE_WIDTH,
      BASE_HEIGHT};
  return bounds;
}
