#include <stdbool.h>
#include <stdio.h>

#define SDL_MAIN_USE_CALLBACKS // Use SDL's main callback model
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
// #include <SDL3_image/SDL_image.h>

SDL_Window *window;     // SDL window structure
SDL_Renderer *renderer; // SDL renderer structure

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

// Function to render the scene
void render()
{
    SDL_RenderClear(renderer);                      // Clear the rendering target
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Set draw color to black
    SDL_RenderPresent(renderer);                    // Update the screen
}

// Callback for each iteration of the main loop
SDL_AppResult SDL_AppIterate(void *appstate)
{
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
        800,                 // Window width
        600,                 // Window height
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
        // Note: Should probably return SDL_APP_FAILURE here too for consistency
        return 1; // Indicate failure (non-zero usually means error)
    }

    getLocalAddrs(); // Get and log local addresses

    return SDL_APP_CONTINUE; // Signal successful initialization
}