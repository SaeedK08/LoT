#include "../include/update.h"

// --- Public Functions ---

void app_update(void *appstate)
{
  AppState *state = (AppState *)appstate;

  // --- Calculate Delta Time ---
  state->last_tick = state->current_tick;
  state->current_tick = SDL_GetTicks();
  // Handle first frame case to avoid large initial delta_time
  if (state->last_tick == 0)
  {
    state->last_tick = state->current_tick - 16; // Assume ~60 fps for first frame
  }
  Uint64 delta_ticks = state->current_tick - state->last_tick;
  state->delta_time = (float)delta_ticks / 1000.0f;
  // Clamp delta_time to prevent large jumps (e.g., after debugging pause)
  const float max_delta_time = 0.1f; // Max delta = 100ms (10 FPS)
  if (state->delta_time > max_delta_time)
  {
    state->delta_time = max_delta_time;
  }

  // --- Update Entities ---
  // Delegate entity updates to the EntityManager.
  if (state->entity_manager)
  {
    EntityManager_UpdateAll(state->entity_manager, state);
  }
  else
  {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "EntityManager not initialized in app_update.");
  }
}
