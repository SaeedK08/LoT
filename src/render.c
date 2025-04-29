#include "../include/render.h"

void app_render(void *appstate)
{
  AppState *state = (AppState *)appstate;

  SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
  SDL_RenderClear(state->renderer);

  for (int i = 0; i < entities_count; i++)
  {
    if (entities[i].render)
    {
      entities[i].render(state);
    }
  }

  SDL_RenderPresent(state->renderer);
}