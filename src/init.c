#include "../include/init.h"

// Cleans up initialized resources in reverse order upon failure during SDL_AppInit.
static void cleanup_on_failure(AppState *state, bool net_quit, bool server_cleanup, bool client_cleanup, bool map_cleanup, bool player_cleanup, bool camera_cleanup)
{
  // Entity Cleanup
  if (camera_cleanup)
  {
    int idx = find_entity("camera");
    if (idx != -1 && entities[idx].cleanup)
      entities[idx].cleanup();
  }
  if (player_cleanup)
  {
    int idx = find_entity("player");
    if (idx != -1 && entities[idx].cleanup)
      entities[idx].cleanup();
  }
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

  // SDL Subsystem Cleanup
  if (net_quit)
  {
    SDLNet_Quit();
  }
  if (state->renderer)
  {
    SDL_DestroyRenderer(state->renderer);
  }
  if (state->window)
  {
    SDL_DestroyWindow(state->window);
  }
  SDL_QuitSubSystem(SDL_INIT_VIDEO); // Assumes SDL_Init(VIDEO) succeeded if this point is reached

  SDL_free(state);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
  // Argument Parsing
  bool is_server = false;
  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "--server") == 0)
    {
      is_server = true;
      break;
    }
  }
  // Default to server role if no arguments are given
  if (argc <= 1)
  {
    is_server = true;
    SDL_Log("No arguments provided, assuming server role.");
  }
  else if (is_server)
  {
    SDL_Log("Argument '--server' found, running as server.");
  }
  else
  {
    SDL_Log("Running as client only.");
  }

  // State Allocation
  AppState *state = SDL_malloc(sizeof(AppState));
  if (!state)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Failed to allocate AppState.", __func__);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }
  memset(state, 0, sizeof(AppState));
  *appstate = state;

  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] SDL_Init(SDL_INIT_VIDEO) failed: %s", __func__, SDL_GetError());
    SDL_free(state);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  state->window = SDL_CreateWindow("League of Tigers", 1280, 720, SDL_WINDOW_RESIZABLE);
  if (!state->window)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] SDL_CreateWindow failed: %s", __func__, SDL_GetError());
    cleanup_on_failure(state, false, false, false, false, false, false);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  state->renderer = SDL_CreateRenderer(state->window, NULL);
  if (!state->renderer)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] SDL_CreateRenderer failed: %s", __func__, SDL_GetError());
    cleanup_on_failure(state, false, false, false, false, false, false);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  if (!SDLNet_Init())
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] SDLNet_Init failed: %s", __func__, SDL_GetError());
    cleanup_on_failure(state, false, false, false, false, false, false);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  // Conditional Server Init
  if (is_server)
  {
    if (init_server() == SDL_APP_FAILURE)
    {
      cleanup_on_failure(state, true, false, false, false, false, false);
      *appstate = NULL;
      return SDL_APP_FAILURE;
    }
  }

  // Client Init (Always)
  if (init_client() == SDL_APP_FAILURE)
  {
    cleanup_on_failure(state, true, is_server, false, false, false, false);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  if (init_map(state->renderer) == SDL_APP_FAILURE)
  {
    cleanup_on_failure(state, true, is_server, true, false, false, false);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  if (init_player(state->renderer) == SDL_APP_FAILURE)
  {
    cleanup_on_failure(state, true, is_server, true, true, false, false);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  if (init_camera(state->renderer) == SDL_APP_FAILURE)
  {
    cleanup_on_failure(state, true, is_server, true, true, true, false);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  if (init_base(state->renderer) == SDL_APP_FAILURE)
  {
    cleanup_on_failure(state, true, is_server, true, true, true, true);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  if (init_tower(state->renderer) == SDL_APP_FAILURE)
  {
    cleanup_on_failure(state, true, is_server, true, true, true, true);
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  SDL_SetRenderLogicalPresentation(state->renderer, CAMER_VIEW_WIDTH, CAMER_VIEW_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

  return SDL_APP_CONTINUE;
}