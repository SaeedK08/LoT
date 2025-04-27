
#include "../include/tower.h"
#define MAX_TOWERS 4

// Tower positions
static SDL_FPoint tower_positions[MAX_TOWERS] = {
    {700.0f, 850.0f},
    {400.0f, 850.0f},
    {2800.0f, 850.0f},
    {2500.0f, 850.0f}};

static SDL_Texture *tower_textures[MAX_TOWERS] = {NULL};

// Tower Dimensions
static const float TOWER_WIDTH = 100.0f;
static const float TOWER_HEIGHT = 200.0f;

static void cleanup()
{
    for (int i = 0; i < MAX_TOWERS; i++)
    {
        if (tower_textures[i])
        {
            SDL_DestroyTexture(tower_textures[i]);
            tower_textures[i] = NULL;
            SDL_Log("All tower texture cleaned up.");
        }
    }
}

static void render(SDL_Renderer *renderer)
{
    // Check that the texture has loaded
    for (int i = 0; i < MAX_TOWERS; i++)
    {

        if (!tower_textures[i])
            continue;

        // Calculate screen position towerd on world position and camera
        float screen_x = tower_positions[i].x - camera.x - TOWER_WIDTH / 2.0f;
        float screen_y = tower_positions[i].y - camera.y - TOWER_HEIGHT / 2.0f;

        SDL_FRect tower_dest_rect = {
            screen_x,
            screen_y,
            TOWER_WIDTH,
            TOWER_HEIGHT};

        // Render towers
        SDL_RenderTexture(renderer, tower_textures[i], NULL, &tower_dest_rect);
    }
}

SDL_AppResult init_tower(SDL_Renderer *renderer)
{
    const char *paths[MAX_TOWERS] = {
        "./resources/Sprites/Blue_Team/Tower_Blue.png",
        "./resources/Sprites/Blue_Team/Tower_Blue.png",
        "./resources/Sprites/Red_Team/Tower_Red.png",
        "./resources/Sprites/Red_Team/Tower_Red.png"};

    for (int i = 0; i < MAX_TOWERS; i++)
    {
        tower_textures[i] = IMG_LoadTexture(renderer, paths[i]);

        if (!tower_textures[i])
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Failed to load tower texture '%s': %s", __func__, paths[i], SDL_GetError());
            return SDL_APP_FAILURE;
        }
        SDL_SetTextureScaleMode(tower_textures[i], SDL_SCALEMODE_NEAREST);
    }

    Entity tower_entity = {
        .name = "tower",
        .cleanup = cleanup,
        .render = render};

    if (create_entity(tower_entity) == SDL_APP_FAILURE)
    {
        // SDL_DestroyTexture(tower_textures[i]);

        for (int i = 0; i < MAX_TOWERS; i++)
        {
            cleanup();
        }
        // tower_texture = NULL;
        return SDL_APP_FAILURE;
    }

    SDL_Log("Tower entity initialized.");
    return SDL_APP_SUCCESS;
}