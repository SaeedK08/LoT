#include "../include/init.h"

// SDL3 Application Initialization callback function
SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
  // Allocate memory for the application's state structure
  AppState *state = SDL_malloc(sizeof(AppState));
  if (!state)
  {
    SDL_Log("Error allocating AppState");
    return SDL_APP_FAILURE;
  }
  *appstate = state;    // Assign the allocated state
  state->window = NULL; // Initialize pointers
  state->renderer = NULL;

  // Initialize the SDL video subsystem
  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    SDL_Log("Error initializing SDL: %s", SDL_GetError());
    SDL_free(state);
    return SDL_APP_FAILURE;
  }

  // Create the application window
  state->window = SDL_CreateWindow(
      "SDL3 Game",
      1280,
      720,
      SDL_WINDOW_FULLSCREEN);

  if (!state->window)
  {
    SDL_Log("Error creating window: %s", SDL_GetError());
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_free(state);
    return SDL_APP_FAILURE;
  }

  // Create the renderer
  state->renderer = SDL_CreateRenderer(state->window, NULL);
  if (!state->renderer)
  {
    SDL_Log("Error creating renderer: %s", SDL_GetError());
    SDL_DestroyWindow(state->window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_free(state);
    return SDL_APP_FAILURE;
  }

  // Set the logical size for rendering using defined constants
  SDL_SetRenderLogicalPresentation(state->renderer, LOGICAL_WIDTH, LOGICAL_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

  // --- Initialize game-specific systems, checking for errors ---
  if (!init_map(state->renderer))
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Map initialization failed!");
    SDL_DestroyRenderer(state->renderer);
    SDL_DestroyWindow(state->window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_free(state);
    return SDL_APP_FAILURE;
  }

  init_player(state->renderer);
  init_camera(state->renderer);

  // Initialize timing variables
  state->last_tick = 0;
  state->current_tick = SDL_GetPerformanceCounter();
  state->delta_time = 0;

  // Indicate successful initialization and continue the app lifecycle
  return SDL_APP_CONTINUE;
}