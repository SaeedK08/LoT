#include "../include/init.h"

// SDL3 Application Initialization callback function
SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
  // Allocate memory for the application's state structure
  AppState *state = SDL_malloc(sizeof(AppState));
  *appstate = state; // Assign the allocated state to the appstate pointer

  // Initialize the SDL video subsystem
  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error initializing SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE; // Return failure if initialization fails
  }

  // Create the application window (Title, Width, Height, Flags)
  state->window = SDL_CreateWindow(
      "SDL3 Game",
      1280,                  // Initial window width
      720,                   // Initial window height
      SDL_WINDOW_RESIZABLE); // Allow the window to be resized

  if (!state->window)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error creating window: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // Create the renderer associated with the window
  state->renderer = SDL_CreateRenderer(state->window, NULL);

  if (!state->renderer)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error creating renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (!SDLNet_Init())
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDLNet_Init failed: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // Initialize game-specific systems
  init_server();
  init_client();
  init_map(state->renderer);
  init_player(state->renderer);
  init_camera(state->renderer);

  // Set the logical size for rendering (affects scaling and zoom)
  // Renders at 320x180, then scales to window size with letterboxing
  SDL_SetRenderLogicalPresentation(state->renderer, 320, 180, SDL_LOGICAL_PRESENTATION_LETTERBOX);

  // Indicate successful initialization and continue the app lifecycle
  return SDL_APP_CONTINUE;
}