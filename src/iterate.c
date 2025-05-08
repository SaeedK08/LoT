#include "../include/iterate.h"

void app_wait_for_next_frame(void *appstate)
{
  AppState *state = (AppState *)appstate;

  // Calculate time spent on the current frame
  Uint64 frame_duration_ms = SDL_GetTicks() - state->current_tick;

  // Delay if the frame finished faster than the target time
  if (frame_duration_ms < TARGET_FRAME_TIME_MS)
  {
    SDL_Delay(TARGET_FRAME_TIME_MS - frame_duration_ms);
  }
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
  AppState *state = (AppState *)appstate;

  app_update(state);
  app_render(state);
  app_wait_for_next_frame(state);

  return state->quit_requested ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}