#include "../include/base.h"

// --- Static Variables ---

// World coordinates for the center of each base.
static SDL_FPoint base_positions[MAX_BASES] = {
    {160.0f, 850.0f}, // Base 0 position
    {3040.0f, 850.0f} // Base 1 position
};

// Textures for each base.
static SDL_Texture *base_textures[MAX_BASES] = {NULL, NULL};

// Dimensions for rendering the base sprites.

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
    if (base_textures[i])
    {
      SDL_DestroyTexture(base_textures[i]);
      base_textures[i] = NULL;
    }
  }
  SDL_Log("All base textures cleaned up.");
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
    if (!base_textures[i])
    {
      continue; // Skip rendering if texture isn't loaded.
    }

    // Calculate screen position based on world position, camera offset, and sprite dimensions.
    float screen_x = base_positions[i].x - camera.x - BASE_WIDTH / 2.0f;
    float screen_y = base_positions[i].y - camera.y - BASE_HEIGHT / 2.0f;

    SDL_FRect base_dest_rect = {
        screen_x,
        screen_y,
        BASE_WIDTH,
        BASE_HEIGHT};

    // Render the base texture.
    SDL_RenderTexture(state->renderer, base_textures[i], NULL, &base_dest_rect);
  }
}

// --- Public API Function Implementations ---

SDL_AppResult init_base(SDL_Renderer *renderer)
{
  // --- Load Textures ---
  const char *paths[MAX_BASES] = {
      "./resources/Sprites/Blue_Team/Castle_Blue.png",
      "./resources/Sprites/Red_Team/Castle_Red.png"};

  for (int i = 0; i < MAX_BASES; i++)
  {
    if (base_textures[i] == NULL) // Load only if not already loaded.
    {
      base_textures[i] = IMG_LoadTexture(renderer, paths[i]);
      if (!base_textures[i])
      {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Base] Failed to load base texture '%s': %s", paths[i], SDL_GetError());
        // Clean up any textures loaded so far before failing.
        cleanup();
        return SDL_APP_FAILURE;
      }
      // Use nearest neighbor scaling for pixel art.
      SDL_SetTextureScaleMode(base_textures[i], SDL_SCALEMODE_NEAREST);
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Base] Loaded texture: %s", paths[i]);
    }
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
  if (baseIndex < 0 || baseIndex >= MAX_TOWERS)
  {
    return (SDL_FRect){0, 0, 0, 0}; // Return empty rect on error
  }

  // Calculate bounds based on center position and dimensions
  SDL_FRect bounds = {
      base_positions[baseIndex].x - BASE_WIDTH / 2.0f,
      base_positions[baseIndex].y - BASE_HEIGHT / 2.0f,
      BASE_WIDTH,
      BASE_HEIGHT};
  return bounds;
}
