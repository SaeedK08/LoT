#include "../include/tower.h"

// --- Static Variables ---

typedef struct tower
{
    SDL_FPoint position;
    SDL_Texture *texture;
    SDL_Texture *destroyed;
    float health;
} Tower;
Tower *towers[MAX_TOWERS];

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
        if (towers[i]->texture)
        {
            SDL_DestroyTexture(towers[i]->texture);
            towers[i]->texture = NULL;
        }
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Tower] All tower textures cleaned up.");
}

void damageTower(float tower_posx)
{
    for (int i = 0; i < MAX_TOWERS; i++)
    {
        if (towers[i]->position.x - camera.x - TOWER_WIDTH / 2.0f == tower_posx)
        {
            SDL_Log("Tower %d is getting damaged", i);

            towers[i]->health -= 10.0f;
            if (towers[i]->health <= 0)
            {
                destroyTower(i);
            }
        }
    }
}

// --- This function is for now unnecessary, but could be usefull for modularity when sending and receiving data ---
void destroyTower(int towerIndex)
{
    towers[towerIndex]->texture = towers[towerIndex]->destroyed;
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
        if (!towers[i]->texture)
            continue;

        // Calculate screen position based on world position, camera offset, and sprite dimensions.
        // Subtracting half width/height centers the sprite on its world position.
        float screen_x = towers[i]->position.x - camera.x - TOWER_WIDTH / 2.0f;
        float screen_y = towers[i]->position.y - camera.y - TOWER_HEIGHT / 2.0f;

        SDL_FRect tower_dest_rect = {
            screen_x,
            screen_y,
            TOWER_WIDTH,
            TOWER_HEIGHT};

        // Render the tower texture. NULL source rect uses the entire texture.
        SDL_RenderTexture(state->renderer, towers[i]->texture, NULL, &tower_dest_rect);
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
    const char *path_destroyed = {"./resources/Sprites/Tower_Destroyed.png"};

    for (int i = 0; i < MAX_TOWERS; i++)
    {
        towers[i] = SDL_malloc(sizeof(Tower));
        towers[i]->texture = IMG_LoadTexture(renderer, paths[i]);
        towers[i]->destroyed = IMG_LoadTexture(renderer, path_destroyed);
        if (!towers[i]->texture)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Tower] Failed to load tower texture '%s': %s", paths[i], SDL_GetError());
            // Clean up textures loaded so far before failing.
            cleanup();
            return SDL_APP_FAILURE;
        }
        if (!towers[i]->destroyed)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Tower] Failed to load destroyed tower texture '%s': %s", paths[i], SDL_GetError());
            // Clean up textures loaded so far before failing.
            cleanup();
            return SDL_APP_FAILURE;
        }
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Tower] Loaded texture: %s", paths[i]);

        // Use nearest neighbor scaling for pixel art.
        SDL_SetTextureScaleMode(towers[i]->texture, SDL_SCALEMODE_NEAREST);
        switch (i)
        {
        case 0:
            towers[i]->position = (SDL_FPoint){500.0f, BUILDINGS_POS_Y}; // Position of tower 1, Blue Team
            break;
        case 1:
            towers[i]->position = (SDL_FPoint){800.0f, BUILDINGS_POS_Y}; // Position of tower 2, Blue Team
            break;
        case 2:
            towers[i]->position = (SDL_FPoint){2400.0f, BUILDINGS_POS_Y}; // Position of tower 1, Red Team
            break;
        case 3:
            towers[i]->position = (SDL_FPoint){2700.0f, BUILDINGS_POS_Y}; // Position of tower 2, Red Team
            break;
        default:
            break;
        }
        towers[i]->health = 100.0f;
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

SDL_FRect getTowerPos(int towerIndex)
{
    if (towerIndex < 0 || towerIndex >= MAX_TOWERS)
    {
        return (SDL_FRect){0, 0, 0, 0}; // Return empty rect on error
    }

    // Calculate bounds based on center position and dimensions
    SDL_FRect bounds = {
        towers[towerIndex]->position.x - TOWER_WIDTH / 2.0f,
        towers[towerIndex]->position.y - TOWER_HEIGHT / 2.0f,
        TOWER_WIDTH,
        TOWER_HEIGHT};
    return bounds;
}