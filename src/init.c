#include "../include/init.h"

// --- Static Helper Functions ---

/**
 * @brief Cleans up resources initialized *before* a failure occurred during SDL_AppInit.
 * This function is called internally by SDL_AppInit if an initialization step fails.
 * It attempts to destroy modules and subsystems in the reverse order they were created,
 * up to the point of failure.
 * @param state The main application state.
 * @param failure_stage A string identifying which initialization stage failed.
 */
static void cleanup_on_failure(AppState *state, const char *failure_stage)
{
  SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Initialization failed at stage '%s', cleaning up...", failure_stage);

  // --- Destroy ADT Modules (Reverse Order of Creation) ---
  // The comparisons check if the failure happened *before* the respective module's init.
  if (strcmp(failure_stage, "Camera_Init") != 0)
  {
    Camera_Destroy(state->camera_state);
  }
  if (strcmp(failure_stage, "PlayerManager_Init") != 0 && strcmp(failure_stage, "Camera_Init") != 0)
  {
    PlayerManager_Destroy(state->player_manager);
  }
  if (strcmp(failure_stage, "Attack_Init") != 0 && strcmp(failure_stage, "PlayerManager_Init") != 0 && strcmp(failure_stage, "Camera_Init") != 0)
  {
    AttackManager_Destroy(state->attack_manager);
  }
  if (strcmp(failure_stage, "Tower_Init") != 0 && strcmp(failure_stage, "Attack_Init") != 0 && strcmp(failure_stage, "PlayerManager_Init") != 0 && strcmp(failure_stage, "Camera_Init") != 0)
  {
    TowerManager_Destroy(state->tower_manager);
  }
  if (strcmp(failure_stage, "Base_Init") != 0 && strcmp(failure_stage, "Tower_Init") != 0 && strcmp(failure_stage, "Attack_Init") != 0 && strcmp(failure_stage, "PlayerManager_Init") != 0 && strcmp(failure_stage, "Camera_Init") != 0)
  {
    BaseManager_Destroy(state->base_manager);
  }
  if (strcmp(failure_stage, "Map_Init") != 0 && strcmp(failure_stage, "Base_Init") != 0 && strcmp(failure_stage, "Tower_Init") != 0 && strcmp(failure_stage, "Attack_Init") != 0 && strcmp(failure_stage, "PlayerManager_Init") != 0 && strcmp(failure_stage, "Camera_Init") != 0)
  {
    Map_Destroy(state->map_state);
  }
  if (strcmp(failure_stage, "NetClient_Init") != 0 && strcmp(failure_stage, "Map_Init") != 0 && strcmp(failure_stage, "Base_Init") != 0 && strcmp(failure_stage, "Tower_Init") != 0 && strcmp(failure_stage, "Attack_Init") != 0 && strcmp(failure_stage, "PlayerManager_Init") != 0 && strcmp(failure_stage, "Camera_Init") != 0)
  {
    NetClient_Destroy(state->net_client_state);
  }
  if (strcmp(failure_stage, "NetClient_Init") != 0 && strcmp(failure_stage, "Map_Init") != 0 && strcmp(failure_stage, "Base_Init") != 0 && strcmp(failure_stage, "Tower_Init") != 0 && strcmp(failure_stage, "Attack_Init") != 0 && strcmp(failure_stage, "PlayerManager_Init") != 0 && strcmp(failure_stage, "Camera_Init") != 0 && strcmp(failure_stage, "MinionManager_Init") != 0)
  {
    MinionManager_Destroy(state->minion_manager);
  }
  // Only attempt server cleanup if it was supposed to be initialized and didn't fail before client init
  if (state->is_server && state->net_server_state && strcmp(failure_stage, "NetServer_Init") != 0 && strcmp(failure_stage, "NetClient_Init") != 0 && strcmp(failure_stage, "Map_Init") != 0 && strcmp(failure_stage, "Base_Init") != 0 && strcmp(failure_stage, "Tower_Init") != 0 && strcmp(failure_stage, "Attack_Init") != 0 && strcmp(failure_stage, "PlayerManager_Init") != 0 && strcmp(failure_stage, "Camera_Init") != 0 && strcmp(failure_stage, "MinionManager_Init") != 0)
  {
    NetServer_Destroy(state->net_server_state);
  }
  // Only destroy EntityManager if it was created successfully
  if (state->entity_manager && strcmp(failure_stage, "EntityManager_Create") != 0)
  {
    EntityManager_Destroy(state->entity_manager, state);
  }

  // --- SDL Subsystem Cleanup ---
  if (strcmp(failure_stage, "SDLNet_Init") != 0)
  {
    SDLNet_Quit();
  }
  if (state->renderer)
    SDL_DestroyRenderer(state->renderer);
  if (state->window)
    SDL_DestroyWindow(state->window);
  if (strcmp(failure_stage, "SDL_Init") != 0)
  {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
  }
  SDL_free(state);
}

// --- SDL Application Callback Definitions ---

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
  // --- Argument Parsing ---
  bool is_server_arg = true;                   // Default to server unless --client is specified
  bool team_arg = BLUE_TEAM;                   // Default team
  const char *hostname_arg = DEFAULT_HOSTNAME; // Default hostname

  for (int i = 1; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--client"))
    {
      is_server_arg = false;
    }
    else if (!strcmp(argv[i], "--red"))
    {
      team_arg = RED_TEAM;
    }
    else if (!strcmp(argv[i], "--host") && (i + 1 < argc))
    {
      hostname_arg = argv[i + 1];
      i++;
    }
  }

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Running as %s.", is_server_arg ? "server" : "client");
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Playing for team %s.", team_arg ? "RED" : "BLUE");
  if (!is_server_arg)
  {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Connecting to host: %s", hostname_arg);
  }

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Running as %s.", is_server_arg ? "server (default)" : "client");
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Playing for team %s.", team_arg ? "RED" : "BLUE");

  // --- State Allocation ---
  AppState *state = (AppState *)SDL_calloc(1, sizeof(AppState));
  state->team = team_arg;

  if (!state)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Init] Failed to allocate AppState.");
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }
  state->is_server = is_server_arg;
  state->quit_requested = false;
  *appstate = state;

  // --- SDL Initialization ---
  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Init] SDL_Init(VIDEO) failed: %s", SDL_GetError());
    cleanup_on_failure(state, "SDL_Init");
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  // --- Window Creation ---
  state->window = SDL_CreateWindow("League of Tigers", WINDOW_W, WINDOW_H, SDL_WINDOW_RESIZABLE);
  if (!state->window)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Init] SDL_CreateWindow failed: %s", SDL_GetError());
    cleanup_on_failure(state, "SDL_CreateWindow");
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  // --- Renderer Creation ---
  state->renderer = SDL_CreateRenderer(state->window, NULL);
  if (!state->renderer)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Init] SDL_CreateRenderer failed: %s", SDL_GetError());
    cleanup_on_failure(state, "SDL_CreateRenderer");
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  // --- Set Logical Presentation ---
  if (!SDL_SetRenderLogicalPresentation(state->renderer, (int)CAMERA_VIEW_WIDTH, (int)CAMERA_VIEW_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX))
  {
    SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "[Init] SDL_SetRenderLogicalPresentation failed: %s", SDL_GetError());
  }

  // --- SDL_net Initialization ---
  if (!SDLNet_Init())
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Init] SDLNet_Init failed: %s", SDL_GetError());
    cleanup_on_failure(state, "SDLNet_Init");
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  // --- Create Entity Manager ---
  state->entity_manager = EntityManager_Create(MAX_MANAGED_ENTITIES);
  if (!state->entity_manager)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Init] EntityManager_Create failed: %s", SDL_GetError());
    cleanup_on_failure(state, "EntityManager_Create");
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  // --- Initialize Core Modules (Order Matters!) ---
  if (state->is_server)
  {
    state->net_server_state = NetServer_Init(state);
    if (!state->net_server_state)
    {
      cleanup_on_failure(state, "NetServer_Init");
      *appstate = NULL;
      return SDL_APP_FAILURE;
    }
  }

  // Always initialize the client module
  state->net_client_state = NetClient_Init(state, hostname_arg);
  if (!state->net_client_state)
  {
    cleanup_on_failure(state, "NetClient_Init");
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  state->map_state = Map_Init(state);
  if (!state->map_state)
  {
    cleanup_on_failure(state, "Map_Init");
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  state->HUD_manager = HUDManager_Init(state);
  if (!state->HUD_manager)
  {
    cleanup_on_failure(state, "HUDManager_Init");
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  state->base_manager = BaseManager_Init(state);
  if (!state->base_manager)
  {
    cleanup_on_failure(state, "Base_Init");
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  state->tower_manager = TowerManager_Init(state);
  if (!state->tower_manager)
  {
    cleanup_on_failure(state, "Tower_Init");
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  state->attack_manager = AttackManager_Init(state);
  if (!state->attack_manager)
  {
    cleanup_on_failure(state, "Attack_Init");
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  state->player_manager = PlayerManager_Init(state);
  if (!state->player_manager)
  {
    cleanup_on_failure(state, "PlayerManager_Init");
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  state->minion_manager = MinionManager_Init(state);
  if (!state->minion_manager)
  {
    cleanup_on_failure(state, "MinionManager_Init");
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  state->camera_state = Camera_Init(state);
  if (!state->camera_state)
  {
    cleanup_on_failure(state, "Camera_Init");
    *appstate = NULL;
    return SDL_APP_FAILURE;
  }

  state->currentGameState = GAME_STATE_LOBBY;

  if (state->is_server)
  {
    create_hud_instace(state, get_hud_element_count(state->HUD_manager), "lobby_host_msg", true, "Host: Type 'start' then enter", (SDL_Color){255, 255, 255, 255}, false, (SDL_FPoint){0.0f, 0.0f}, 0);
    create_hud_instace(state, get_hud_element_count(state->HUD_manager), "lobby_host_input", true, "", (SDL_Color){255, 255, 255, 255}, true, (SDL_FPoint){0.0f, 50.0f}, 0);
    SDL_StartTextInput(state->window);
  }
  else
  {
    create_hud_instace(state, get_hud_element_count(state->HUD_manager), "lobby_client_msg", true, "Client: Wating for host to start the game", (SDL_Color){255, 255, 255, 255}, false, (SDL_FPoint){0.0f, 0.0f}, 0);
  }

  state->cursor_surface = IMG_Load("./resources/cursor_scaled.png");
  if (!state->cursor_surface)
  {
    SDL_Log("Error: Failed to load a surface for cursor: %s\n" ,SDL_GetError());
    SDL_DestroySurface(state->cursor_surface);
  }
  state->cursor = SDL_CreateColorCursor(state->cursor_surface, 0, 0);
  if (!state->cursor)
  {
    SDL_Log("Error: Failed to create a cursor: %s\n" ,SDL_GetError());
    SDL_DestroyCursor(state->cursor);
  }
  SDL_SetCursor(state->cursor);


  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Init] Application initialized successfully.");
  return SDL_APP_CONTINUE;
}