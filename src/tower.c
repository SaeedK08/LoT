#include "../include/tower.h"

// --- Static Variables ---

// World coordinates for the center of each tower.
static SDL_FPoint tower_positions[MAX_TOWERS] = {
    {700.0f, 850.0f},  // Tower 0 (Blue Team)
    {400.0f, 850.0f},  // Tower 1 (Blue Team)
    {2800.0f, 850.0f}, // Tower 2 (Red Team)
    {2500.0f, 850.0f}  // Tower 3 (Red Team)
};

// Textures for the towers. Indices correspond to tower_positions.
static SDL_Texture *tower_textures[MAX_TOWERS] = {NULL, NULL, NULL, NULL};

// Dimensions used for rendering the tower sprites.
static const float TOWER_WIDTH = 100.0f;
static const float TOWER_HEIGHT = 200.0f;

// --- Static Helper Functions ---

/**
 * @brief Cleans up resources used by the tower system, specifically the tower textures.
 * @param void
 * @return void
 */
static void cleanup()
{
    for (int i = 0; i < MAX_TOWERS; i++)
    {
        if (tower_textures[i])
        {
            SDL_DestroyTexture(tower_textures[i]);
            tower_textures[i] = NULL;
        }
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Tower] All tower textures cleaned up.");
}

/**
 * @brief Renders the tower sprites at their calculated screen positions.
 * @param state Pointer to the global application state, containing the renderer.
 * @return void
 */
static void render(AppState *state)
{
    for (int i = 0; i < MAX_TOWERS; i++)
    {
        // Skip rendering if the texture for this tower isn't loaded.
        if (!tower_textures[i])
            continue;

        // Calculate screen position based on world position, camera offset, and sprite dimensions.
        // Subtracting half width/height centers the sprite on its world position.
        float screen_x = tower_positions[i].x - camera.x - TOWER_WIDTH / 2.0f;
        float screen_y = tower_positions[i].y - camera.y - TOWER_HEIGHT / 2.0f;

        SDL_FRect tower_dest_rect = {
            screen_x,
            screen_y,
            TOWER_WIDTH,
            TOWER_HEIGHT};

        // Render the tower texture. NULL source rect uses the entire texture.
        SDL_RenderTexture(state->renderer, tower_textures[i], NULL, &tower_dest_rect);
    }
}

// --- Public API Function Implementations ---

SDL_AppResult init_tower(SDL_Renderer *renderer)
{
    // --- Load Textures ---
    const char *paths[MAX_TOWERS] = {
        "./resources/Sprites/Blue_Team/Tower_Blue.png",
        "./resources/Sprites/Blue_Team/Tower_Blue.png",
        "./resources/Sprites/Red_Team/Tower_Red.png",
        "./resources/Sprites/Red_Team/Tower_Red.png"};

    for (int i = 0; i < MAX_TOWERS; i++)
    {
        // Only load if not already loaded (although init should only run once).
        if (tower_textures[i] == NULL)
        {
            tower_textures[i] = IMG_LoadTexture(renderer, paths[i]);
            if (!tower_textures[i])
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Tower] Failed to load tower texture '%s': %s", paths[i], SDL_GetError());
                // Clean up textures loaded so far before failing.
                cleanup();
                return SDL_APP_FAILURE;
            }
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Tower] Loaded texture: %s", paths[i]);
            // Use nearest neighbor scaling for pixel art.
            SDL_SetTextureScaleMode(tower_textures[i], SDL_SCALEMODE_NEAREST);
        }
    }

    // --- Register Entity ---
    // Register the tower entity system if it hasn't been registered yet.
    if (find_entity("tower") == -1)
    {
        Entity tower_entity = {
            .name = "tower",
            .update = NULL,
            .render = render,
            .cleanup = cleanup,
            .handle_events = NULL};

        if (create_entity(tower_entity) == SDL_APP_FAILURE)
        {
            // Error logged within create_entity.
            cleanup(); // Clean up loaded textures if entity registration fails.
            return SDL_APP_FAILURE;
        }
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Tower] Tower entity created.");
    }
    else
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Tower] Tower entity already exists.");
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Tower] Tower system initialized.");
    return SDL_APP_SUCCESS;
}