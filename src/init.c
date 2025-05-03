#include "../include/init.h"

// --- Static Helper Functions ---

/**
 * @brief Cleans up resources initialized before a failure point during app init.
 * @param state The application state containing pointers to initialized resources.
 * @param net_quit Flag indicating if SDLNet_Quit should be called.
 * @param server_cleanup Flag indicating if server entity cleanup is needed.
 * @param client_cleanup Flag indicating if client entity cleanup is needed.
 * @param map_cleanup Flag indicating if map entity cleanup is needed.
 * @param base_cleanup Flag indicating if base entity cleanup is needed.
 * @param tower_cleanup Flag indicating if tower entity cleanup is needed.
 * @param attack_cleanup Flag indicating if attack entity cleanup is needed.
 * @return void
 */
static void cleanup_on_failure(AppState *state, bool net_quit, bool server_cleanup, bool client_cleanup, bool map_cleanup, bool base_cleanup, bool tower_cleanup, bool attack_cleanup)
{
  SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Initialization failed, cleaning up...");

  // --- Entity Cleanup (Reverse Order of Creation) ---
  if (attack_cleanup)
  {
    int idx = find_entity("fireball");
    if (idx != -1 && entities[idx].cleanup)
      entities[idx].cleanup();
  }
  if (tower_cleanup)
  {
    int idx = find_entity("tower");
    if (idx != -1 && entities[idx].cleanup)
      entities[idx].cleanup();
  }
  if (base_cleanup)
  {
    int idx = find_entity("base");
    if (idx != -1 && entities[idx].cleanup)
      entities[idx].cleanup();
  }
  // Player and Camera cleanup are handled by net_client cleanup now
  if (map_cleanup)
  {
    int idx = find_entity("map");
    if (idx != -1 && entities[idx].cleanup)
      entities[idx].cleanup();
  }
  if (client_cleanup)
  {
    int idx = find_entity("net_client");
    if (idx != -1 && entities[idx].cleanup)
      entities[idx].cleanup();
  }
  if (server_cleanup)
  {
    int idx = find_entity("net_server");
    if (idx != -1 && entities[idx].cleanup)
      entities[idx].cleanup();
  }

  // --- SDL Subsystem Cleanup ---
  if (net_quit)
    SDLNet_Quit();
  if (state->renderer)
    SDL_DestroyRenderer(state->renderer);
  if (state->window)
    SDL_DestroyWindow(state->window);
  SDL_QuitSubSystem(SDL_INIT_VIDEO); // Assumes SDL_Init(VIDEO) succeeded
  SDL_free(state);
}

// --- Public API Function Implementations ---

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
  // --- Argument Parsing ---
  bool is_server_arg = (argc <= 1); // Default to server if no args
  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "--server") == 0)
    {
      is_server_arg = true;
      break;
    }
  }
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Running as %s.", is_server_arg ? "server" : "client");

  // --- State Allocation ---
  AppState *state = SDL_malloc(sizeof(AppState));
  if (!state)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Init] Failed to allocate AppState.");
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }
  memset(state, 0, sizeof(AppState));
  state->is_server = is_server_arg;
  *appstate = state;

  // --- SDL Initialization ---
  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Init] SDL_Init(VIDEO) failed: %s", SDL_GetError());
    SDL_free(state);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  // --- Window Creation ---
  state->window = SDL_CreateWindow("League of Tigers", 1280, 720, SDL_WINDOW_RESIZABLE);
  if (!state->window)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Init] SDL_CreateWindow failed: %s", SDL_GetError());
    cleanup_on_failure(state, false, false, false, false, false, false, false);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  // --- Renderer Creation ---
  state->renderer = SDL_CreateRenderer(state->window, NULL);
  if (!state->renderer)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Init] SDL_CreateRenderer failed: %s", SDL_GetError());
    cleanup_on_failure(state, false, false, false, false, false, false, false);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  // --- Set Logical Presentation ---
  if (!SDL_SetRenderLogicalPresentation(state->renderer, (int)CAMERA_VIEW_WIDTH, (int)CAMERA_VIEW_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX))
  {
    // This might not be fatal, but log a warning.
    SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "[Init] SDL_SetRenderLogicalPresentation failed: %s", SDL_GetError());
  }

  // --- SDL_net Initialization ---
  if (!SDLNet_Init())
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Init] SDLNet_Init failed: %s", SDL_GetError());
    cleanup_on_failure(state, false, false, false, false, false, false, false);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  // --- Server Initialization (Conditional) ---
  if (state->is_server)
  {
    if (init_server() == SDL_APP_FAILURE)
    {
      cleanup_on_failure(state, true, false, false, false, false, false, false);
      *appstate = NULL;
      return SDL_APP_FAILURE;
    }
  }

  // --- Client Initialization (Always) ---
  if (init_client() == SDL_APP_FAILURE)
  {
    cleanup_on_failure(state, true, state->is_server, false, false, false, false, false);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  // --- Map Initialization ---
  if (init_map(state->renderer) == SDL_APP_FAILURE)
  {
    cleanup_on_failure(state, true, state->is_server, true, false, false, false, false);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  // --- Base Initialization ---
  if (init_base(state->renderer) == SDL_APP_FAILURE)
  {
    cleanup_on_failure(state, true, state->is_server, true, true, false, false, false);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  // --- Tower Initialization ---
  if (init_tower(state->renderer) == SDL_APP_FAILURE)
  {
    cleanup_on_failure(state, true, state->is_server, true, true, true, false, false);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  // --- Attack System Initialization ---
  if (init_fireball(state->renderer) == SDL_APP_FAILURE)
  {
    cleanup_on_failure(state, true, state->is_server, true, true, true, true, false);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Init] Application initialized successfully.");
  return SDL_APP_CONTINUE;
}