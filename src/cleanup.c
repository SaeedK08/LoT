#include "../include/common.h"

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
  AppState *state = (AppState *)appstate;
  (void)result;

  // Call cleanup for all registered entities
  for (int i = 0; i < entities_count; i++)
  {
    if (entities[i].cleanup) // Check if cleanup function exists
    {
      entities[i].cleanup();
    }
  }

  // Destroy SDL resources
  if (state->renderer)
  {
    SDL_DestroyRenderer(state->renderer);
    state->renderer = NULL;
  }
  if (state->window)
  {
    SDL_DestroyWindow(state->window);
    state->window = NULL;
  }

  // Quit SDL subsystems
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
  SDLNet_Quit();
}