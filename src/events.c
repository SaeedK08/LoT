#include "../include/common.h"

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  (void)appstate;

  // Handle global quit event
  if (event->type == SDL_EVENT_QUIT)
  {
    return SDL_APP_SUCCESS; // Signal to exit the application loop
  }

  // Pass event to all entities that have an event handler
  for (int i = 0; i < entities_count; i++)
  {
    if (entities[i].handle_events) // Check if handler exists
    {
      entities[i].handle_events(appstate, event);
    }
  }

  // Continue the application loop
  return SDL_APP_CONTINUE;
}