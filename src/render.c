#include "../include/render.h"

// --- Public Functions ---

void app_render(void *appstate)
{
  AppState *state = (AppState *)appstate;

  // --- Clear Screen ---
  SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
  SDL_RenderClear(state->renderer);

  // --- Render Entities ---
  // Delegate entity rendering to the EntityManager.
  if (state->entity_manager)
  {
    EntityManager_RenderAll(state->entity_manager, state);
  }
  else
  {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "EntityManager not initialized in app_render.");
  }

  // --- Present Renderer ---
  SDL_RenderPresent(state->renderer);
}
