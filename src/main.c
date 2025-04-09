#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define SDL_MAIN_USE_CALLBACKS // Use SDL's main callback model
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include <SDL3_image/SDL_image.h>
#include "../include/player.h"
#include "../include/damage.h"

SDL_Window *window;     // SDL window structure
SDL_Renderer *renderer; // SDL renderer structure
SDL_Texture *mapTexture;

// Callback for application shutdown
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    SDL_DestroyRenderer(renderer); // Clean up the renderer
    renderer = NULL;
    SDL_DestroyWindow(window); // Clean up the window
    window = NULL;
    SDL_QuitSubSystem(SDL_INIT_VIDEO); // Shut down SDL video subsystem
}

// Callback for handling SDL events
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    // Check for the quit event, closing the window
    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS; // Signal to quit the app
    }

    // Check for left click
    if (event->button.button == 1)
    {
        funcCheckHit((SDL_FPoint){event->button.x, event->button.y});
    }

    return SDL_APP_CONTINUE; // Continue processing events
}

SDL_AppResult loadMapTexture()
{
    char mapLoc[] = "./resources/Sprites/map.png";

    mapTexture = IMG_LoadTexture(renderer, mapLoc);

    if (!mapTexture)
    {
        SDL_Log("Couldn't load %s: %s\n", mapLoc, SDL_GetError());
        return SDL_APP_FAILURE; // Signal initialization failure
    }
}

void renderMap()
{
    SDL_RenderTexture(renderer, mapTexture, NULL, NULL);
}

void update()
{
    update_player();
}

// Function to render the scene
void render()
{
    SDL_RenderClear(renderer); // Clear the rendering target

    renderMap();
    render_player(renderer);

    SDL_RenderPresent(renderer); // Update the screen
}

// Callback for each iteration of the main loop
SDL_AppResult SDL_AppIterate(void *appstate)
{
    update();
    render();                // Call the render function
    return SDL_APP_CONTINUE; // Continue the main loop
}

// Callback for application initialization
SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    // Initialize SDL video subsystem
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init() failed: %s", SDL_GetError()); // Log SDL error
        return SDL_APP_FAILURE;                           // Signal initialization failure
    }

    // Create the main window
    window = SDL_CreateWindow(
        "SDL3 Game",         // Window title
        WINDOW_W,            // Window width
        WINDOW_H,            // Window height
        SDL_WINDOW_RESIZABLE // Window flags
    );

    if (!window)
    {
        SDL_Log("SDL_CreateWindow() failed: %s", SDL_GetError()); // Log SDL error
        return SDL_APP_FAILURE;                                   // Signal initialization failure
    }

    // Create a renderer for the window
    renderer = SDL_CreateRenderer(window, NULL); // Use default rendering driver

    if (!renderer)
    {
        SDL_Log("SDL_CreateRenderer() failed: %s", SDL_GetError()); // Log SDL error
        return SDL_APP_FAILURE;                                     // Signal initialization failure
    }

    if (!loadMapTexture())
    {
        SDL_Log("loadMapTexture() failed: %s", SDL_GetError()); // Log SDL error
        return SDL_APP_FAILURE;                                 // Signal initialization failure
    }

    if (!init_player(renderer))
    {
        SDL_Log("Erorr initializing a player: %s \n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE; // Signal successful initialization
}