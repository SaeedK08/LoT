#define SDL_MAIN_USE_CALLBACKS // Use SDL's main callback model
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_net/SDL_net.h>

#include "../include/player.h"
#include "../include/damage.h"
#include "../include/map.h"

#include <stdlib.h>

#define WINDOW_W 3200
#define WINDOW_H 1760

struct AppState
{
    SDL_Window *pWindow;     // SDL window structure
    SDL_Renderer *pRenderer; // SDL renderer structure
    Player *player;          // Pointer to the player structure
    int windowWidth;         // Window width
    int windowHeight;        // Window height
};

typedef struct AppState AppState; // Define a type alias for the game structure

// Callback for application shutdown
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    AppState *pState = (AppState *)appstate; // Cast appstate to the correct type
    SDL_DestroyRenderer(pState->pRenderer);  // Clean up the renderer
    pState->pRenderer = NULL;
    SDL_DestroyWindow(pState->pWindow); // Clean up the window
    pState->pWindow = NULL;
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
        AppState *pState = (AppState *)appstate; // Cast appstate to the correct type
        funcCheckHit((SDL_FPoint){event->button.x, event->button.y}, pState->player);
    }

    return SDL_APP_CONTINUE; // Continue processing events
}

void update(AppState *pState)
{
    update_player(pState->player);
}

// // Function to render the scene
void render(AppState *pState)
{
    SDL_RenderClear(pState->pRenderer); // Clear the rendering target

    funcRenderMap(pState->pRenderer);
    render_player(pState->player, pState->pRenderer); // Call the render function for the player

    SDL_RenderPresent(pState->pRenderer); // Update the screen
}

// Callback for each iteration of the main loop
SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *pState = (AppState *)appstate; // Cast appstate to the correct type

    update(pState);
    render(pState);          // Call the render function
    return SDL_APP_CONTINUE; // Continue the main loop
}

// Callback for application initialization
SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    AppState *pState = malloc(sizeof(AppState)); // Allocate memory for the game structure
    if (!pState)
    {
        SDL_Log("Failed to allocate memory for game structure"); // Log error
        return SDL_APP_FAILURE;                                  // Signal initialization failure
    }
    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init() failed: %s", SDL_GetError()); // Log SDL error
        return SDL_APP_FAILURE;                           // Signal initialization failure
    }

    pState->windowWidth = WINDOW_W;  // Set window width
    pState->windowHeight = WINDOW_H; // Set window height
    // Create the main window
    pState->pWindow = SDL_CreateWindow(
        "League Of Tigers",  // Window title
        WINDOW_W,            // Window width
        WINDOW_H,            // Window height
        SDL_WINDOW_RESIZABLE // Window flags
    );

    if (!pState->pWindow)
    {
        SDL_Log("SDL_CreateWindow() failed: %s", SDL_GetError()); // Log SDL error
        return SDL_APP_FAILURE;                                   // Signal initialization failure
    }

    // Create a renderer for the window
    pState->pRenderer = SDL_CreateRenderer(pState->pWindow, NULL); // Use default rendering driver
    if (!pState->pRenderer)
    {
        SDL_Log("SDL_CreateRenderer() failed: %s", SDL_GetError()); // Log SDL error
        return SDL_APP_FAILURE;                                     // Signal initialization failure
    }

    funcInitMap(pState->pRenderer);

    pState->player = createPlayer(pState->pRenderer, WINDOW_W, WINDOW_H); // Create the player

    if (!pState->player)
    {
        SDL_Log("Error creating player: %s \n", SDL_GetError()); // Log error
        return SDL_APP_FAILURE;                                  // Signal initialization failure
    }

    *appstate = pState; // Assign the allocated memory to the appstate pointer

    return SDL_APP_CONTINUE; // Signal successful initialization
}