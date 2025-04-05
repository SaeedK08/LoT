#include <stdbool.h>
#include <stdio.h>

#define SDL_MAIN_USE_CALLBACKS // Use SDL's main callback model
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include <SDL3_image/SDL_image.h>
#include "player.h"

SDL_Window *window;     // SDL window structure
SDL_Renderer *renderer; // SDL renderer structure
SDL_Texture *mapTexture;
SDL_Texture **animTextures;
SDL_Texture *animTexture;
IMG_Animation *anim;

//int current_frame = 0;

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

    return SDL_APP_CONTINUE; // Continue processing events
}

SDL_AppResult SDL_imgDemo()
{

    char mapLoc[] = "./resources/Sprites/map.png";

    mapTexture = IMG_LoadTexture(renderer, mapLoc);

    if (!mapTexture)
    {
        SDL_Log("Couldn't load %s: %s\n", mapLoc, SDL_GetError());
        return SDL_APP_FAILURE; // Signal initialization failure
    }

    // char animLoc[] = "./resources/Sprites/Red_Team/Fire_Wizard/Idle_Animation.gif";

    // animTexture = IMG_LoadTexture(renderer, animLoc);

    // if (!mapTexture)
    // {
    //     SDL_Log("Couldn't load %s: %s\n", animLoc, SDL_GetError());
    //     return SDL_APP_FAILURE; // Signal initialization failure
    // }

    // anim = IMG_LoadAnimation(animLoc);

    // if (!anim)
    // {
    //     SDL_Log("Couldn't load %s: %s\n", animLoc, SDL_GetError());
    //     return SDL_APP_FAILURE; // Signal initialization failure
    // }

    // int w = anim->w;
    // int h = anim->h;

    // animTextures = (SDL_Texture **)SDL_calloc(anim->count, sizeof(*animTextures));
    // if (!animTextures)
    // {
    //     SDL_Log("Couldn't allocate animTextures\n");
    //     IMG_FreeAnimation(anim);
    //     return SDL_APP_FAILURE; // Signal initialization failure
    // }

    // for (int j = 0; j < anim->count; ++j)
    // {
    //     animTextures[j] = SDL_CreateTextureFromSurface(renderer, anim->frames[j]);
    // }
}

void renderIMGDemo()
{
    float textureWidth, textureHeight;

    SDL_GetTextureSize(animTexture, &textureWidth, &textureHeight);

    // 2. Define the destination rectangle (SDL_FRect) on the screen
    SDL_FRect destinationRect;

    // Set the top-left corner position where the mapTexture will be drawn
    destinationRect.x = 50.0f;  // Draw starting 50 pixels from the left edge
    destinationRect.y = 500.0f; // Draw starting 100 pixels from the top edge

    // Set the width and height for the mapTexture on the screen
    // Example 1: Draw the mapTexture at its original size
    destinationRect.h = textureHeight;
    destinationRect.w = textureWidth;

    SDL_RenderTexture(renderer, mapTexture, NULL, NULL);
    //SDL_RenderTexture(renderer, animTextures[current_frame], NULL, &destinationRect);
}

// void delayrenderIMGDemo()
// {
//     int delay;
//     if (anim->delays[current_frame])
//     {
//         delay = anim->delays[current_frame];
//     }
//     else
//     {
//         delay = 100;
//     }
//     SDL_Delay(delay);

//     current_frame = (current_frame + 1) % anim->count;
// }

void update() {
    update_player;
}

// Function to render the scene
void render()
{
    SDL_RenderClear(renderer); // Clear the rendering target
    renderIMGDemo();
    render_player(renderer);
    SDL_RenderPresent(renderer); // Update the screen
    //delayrenderIMGDemo();
}

// Callback for each iteration of the main loop
SDL_AppResult SDL_AppIterate(void *appstate)
{
    update();
    render();                // Call the render function
    return SDL_APP_CONTINUE; // Continue the main loop
}

// Function to get and print local network addresses
bool getLocalAddrs()
{
    SDLNet_Address **addrs = NULL;
    int num_addrs = 0;
    int i;

    // Get local network addresses
    addrs = SDLNet_GetLocalAddresses(&num_addrs);
    if (addrs == NULL)
    {
        SDL_Log("SDLNet_GetLocalAddresses() failed: %s", SDL_GetError());
        SDLNet_Quit(); // Shutdown SDL_net subsystem
        return false;
    }

    SDL_Log("We saw %d local addresses:", num_addrs); // SDL logging function
    for (i = 0; i < num_addrs; i++)
    {
        // Get string representation of an address
        SDL_Log("  - %s", SDLNet_GetAddressString(addrs[i]));
    }

    SDLNet_FreeLocalAddresses(addrs); // Free the address list
    SDLNet_Quit();                    // Shutdown SDL_net subsystem
    return true;                      // Indicate success
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
        WINDOW_W,                // Window width
        WINDOW_H,                // Window height
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

    // Initialize SDL_net subsystem
    if (!SDLNet_Init())
    {
        SDL_Log("SDLNet_Init() failed: %s", SDL_GetError()); // Log SDL error
        return SDL_APP_FAILURE;                              // Signal initialization failure
    }

    getLocalAddrs(); // Get and log local addresses

    if (!SDL_imgDemo())
    {
        SDL_Log("SDL_imgDemo() failed: %s", SDL_GetError()); // Log SDL error
        return SDL_APP_FAILURE;                              // Signal initialization failure
    }
    if (!init_player(renderer)) {
        SDL_Log("Erorr initializing a player: %s \n" ,SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE; // Signal successful initialization
}