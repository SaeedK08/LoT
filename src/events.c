#include "../include/common.h"
#include "../include/net_server.h"

// --- Public Functions ---

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  AppState *state = (AppState *)appstate;

  if (event->type == SDL_EVENT_QUIT)
  {
    state->quit_requested = true;
    return SDL_APP_SUCCESS;
  }

  if (state->entity_manager)
  {
    EntityManager_HandleEventsAll(state->entity_manager, state, event);
  }
  else
  {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "EntityManager not initialized in SDL_AppEvent.");
  }

  return state->quit_requested ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}