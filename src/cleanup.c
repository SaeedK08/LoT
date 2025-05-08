#include "../include/cleanup.h"

// --- Public Functions ---

// Renamed from SDL_AppQuit
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
  AppState *state = (AppState *)appstate;
  (void)result; // Result not currently used

  if (!state)
  {
    return;
  }

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Application cleaning up...");

  // --- Destroy ADT Modules (Reverse Order of Creation from init.c) ---
  // EntityManager_Destroy calls the cleanup callbacks for all registered entities
  // in reverse order, so we just need to destroy the manager itself last.
  // The individual Destroy functions primarily free the manager's state struct.
  Camera_Destroy(state->camera_state);
  PlayerManager_Destroy(state->player_manager);
  AttackManager_Destroy(state->attack_manager);
  TowerManager_Destroy(state->tower_manager);
  BaseManager_Destroy(state->base_manager);
  Map_Destroy(state->map_state);
  NetClient_Destroy(state->net_client_state);
  if (state->net_server_state)
  {
    NetServer_Destroy(state->net_server_state);
  }
  EntityManager_Destroy(state->entity_manager, state); // This calls the cleanup callbacks

  // --- Destroy Core SDL Resources ---
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

  // --- Quit SDL Subsystems ---
  SDLNet_Quit();
  SDL_QuitSubSystem(SDL_INIT_VIDEO);

  // --- Free AppState ---
  SDL_free(state);

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Application cleanup complete.");
}