#include "../include/base.h"

// --- Static Helper Functions ---

/*
 * @brief Renders a single base instance.
 * @param base Pointer to the BaseInstance to render.
 * @param state Pointer to the main AppState.
 */
static void render_single_base(const BaseInstance *base, AppState *state)
{
  if (!base || !base->texture || !state || !state->renderer || !state->camera_state)
  {
    return;
  }

  CameraState camera = state->camera_state;
  float cam_x = Camera_GetX(camera);
  float cam_y = Camera_GetY(camera);

  float baseX = base->position.x - cam_x - BASE_RENDER_WIDTH / 2.0f;
  float baseY = base->position.y - cam_y - BASE_RENDER_HEIGHT / 2.0f;

  // Calculate screen position (rendering from top-left based on center position)
  SDL_FRect dst_rect = {
      .x = baseX,
      .y = baseY,
      .w = BASE_RENDER_WIDTH,
      .h = BASE_RENDER_HEIGHT};

  SDL_RenderTexture(state->renderer, base->texture, NULL, &dst_rect);

  char text_buffer[16];
  snprintf(text_buffer, sizeof(text_buffer), "%d/%d", base->current_health, BASE_HEALTH_MAX);

  char base_name[32];
  snprintf(base_name, sizeof(base_name), "base_%d_health_value", base->index);

  SDL_Color team_color = base->team ? (SDL_Color){255, 0, 0, 255} : (SDL_Color){0, 0, 255, 255};

  update_hud_instance(state, get_hud_index_by_name(state, base_name), text_buffer,
                      team_color, (SDL_FPoint){baseX, baseY - 50}, 0);
}

// --- Static Callback Functions (for EntityManager) ---

/**
 * @brief Internal function to render all bases.
 * @param bm_state The internal state of the base manager module.
 * @param state The main application state.
 */
static void Internal_BaseManagerRender(BaseManagerState bm_state, AppState *state)
{
  if (!bm_state || !state)
    return;
  render_single_base(&bm_state->bases[0], state); // Render Red Base
  render_single_base(&bm_state->bases[1], state); // Render Blue Base
}

/**
 * @brief Internal function to clean up BaseManager resources.
 * @param bm_state The internal state of the base manager module.
 */
static void Internal_BaseManagerCleanup(BaseManagerState bm_state)
{
  if (!bm_state)
    return;
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Cleaning up BaseManager resources...");
  if (bm_state->red_texture)
  {
    SDL_DestroyTexture(bm_state->red_texture);
    bm_state->red_texture = NULL;
  }
  if (bm_state->blue_texture)
  {
    SDL_DestroyTexture(bm_state->blue_texture);
    bm_state->blue_texture = NULL;
  }
}

/**
 * @brief Wrapper function conforming to EntityFunctions.render signature.
 * @param manager The EntityManager instance.
 * @param state Pointer to the main AppState.
 */
static void base_manager_render_callback(EntityManager manager, AppState *state)
{
  (void)manager; // Manager instance is not used in this specific implementation
  Internal_BaseManagerRender(state->base_manager, state);
}

/**
 * @brief Wrapper function conforming to EntityFunctions.cleanup signature.
 * @param manager The EntityManager instance.
 * @param state Pointer to the main AppState.
 */
static void base_manager_cleanup_callback(EntityManager manager, AppState *state)
{
  (void)manager; // Manager instance is not used in this specific implementation
  Internal_BaseManagerCleanup(state ? state->base_manager : NULL);
  if (state)
  {
    state->base_manager = NULL; // Indicate cleanup happened
  }
}

// --- Public API Function Implementations ---

BaseManagerState BaseManager_Init(AppState *state)
{
  if (!state || !state->renderer || !state->entity_manager)
  {
    SDL_SetError("Invalid AppState or missing renderer/entity_manager for BaseManager_Init");
    return NULL;
  }

  // --- Allocation and Resource Loading ---
  BaseManagerState bm_state = (BaseManagerState)SDL_calloc(1, sizeof(struct BaseManagerState_s));
  if (!bm_state)
  {
    SDL_OutOfMemory();
    return NULL;
  }

  bm_state->red_texture = IMG_LoadTexture(state->renderer, RED_BASE_PATH);
  if (!bm_state->red_texture)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Base Init] Failed load texture '%s': %s", RED_BASE_PATH, SDL_GetError());
    SDL_free(bm_state);
    return NULL;
  }
  SDL_SetTextureScaleMode(bm_state->red_texture, SDL_SCALEMODE_NEAREST);

  bm_state->blue_texture = IMG_LoadTexture(state->renderer, BLUE_BASE_PATH);
  if (!bm_state->blue_texture)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Base Init] Failed load texture '%s': %s", BLUE_BASE_PATH, SDL_GetError());
    SDL_DestroyTexture(bm_state->red_texture); // Clean up already loaded texture
    SDL_free(bm_state);
    return NULL;
  }
  SDL_SetTextureScaleMode(bm_state->blue_texture, SDL_SCALEMODE_NEAREST);

  bm_state->destroyed_texture = IMG_LoadTexture(state->renderer, DESTROYED_BASE_PATH);
  if (!bm_state->destroyed_texture)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Base Init] Failed load texture '%s': %s", DESTROYED_BASE_PATH, SDL_GetError());
    SDL_DestroyTexture(bm_state->red_texture); // Clean up already loaded texture
    SDL_free(bm_state);
    return NULL;
  }
  SDL_SetTextureScaleMode(bm_state->destroyed_texture, SDL_SCALEMODE_NEAREST);

  for (int i = 0; i < MAX_BASES; i++)
  {
    // --- Initialize Base Instances ---
    bm_state->bases[i].position = (SDL_FPoint){(i ? BASE_RED_POS_X : BASE_BLUE_POS_X), BUILDINGS_POS_Y};
    bm_state->bases[i].texture = i ? bm_state->red_texture : bm_state->blue_texture;
    bm_state->bases[i].current_health = BASE_HEALTH_MAX;
    bm_state->bases[i].rect = (SDL_FRect){(i ? BASE_RED_POS_X : BASE_BLUE_POS_X) - BASE_RENDER_WIDTH / 2.0f, BUILDINGS_POS_Y - BASE_RENDER_HEIGHT / 2.0f, BASE_RENDER_WIDTH, BASE_RENDER_HEIGHT};
    bm_state->bases[i].team = i;
    bm_state->bases[i].immune = true;
    bm_state->bases[i].index = i;

    char base_name[32];
    snprintf(base_name, sizeof(base_name), "base_%d_health_value", i);

    create_hud_instance(state, get_hud_element_count(state->HUD_manager), base_name, true);
  }

  // --- Register with EntityManager ---
  EntityFunctions base_funcs = {
      .name = "base_manager",
      .render = base_manager_render_callback,
      .cleanup = base_manager_cleanup_callback,
      .update = NULL,
      .handle_events = NULL};

  if (!EntityManager_Add(state->entity_manager, &base_funcs))
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Base Init] Failed to add entity to manager: %s", SDL_GetError());
    Internal_BaseManagerCleanup(bm_state); // Cleanup loaded textures
    SDL_free(bm_state);
    return NULL;
  }

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "BaseManager initialized and entity registered.");
  return bm_state;
}

void BaseManager_Destroy(BaseManagerState bm_state)
{
  // Cleanup callback handles texture destruction.
  if (bm_state)
  {
    // Prevent dangling pointers after cleanup callback potentially ran via EntityManager
    bm_state->red_texture = NULL;
    bm_state->blue_texture = NULL;
    SDL_free(bm_state);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "BaseManagerState container destroyed.");
  }
}

void damageBase(AppState *state, int baseIndex, float damageValue, bool sendToServer)
{

  BaseInstance *tempBase = &state->base_manager->bases[baseIndex];

  if (tempBase->immune)
  {
    return;
  }

  if (tempBase->current_health > 0)
  {
    tempBase->current_health -= damageValue;
    SDL_Log("Base %d health %d", baseIndex, tempBase->current_health);
  }

  if (tempBase->current_health <= 0)
  {
    tempBase->texture = state->base_manager->destroyed_texture;

    if (sendToServer)
    {
      bool winningTeam = (tempBase->team == BLUE_TEAM) ? RED_TEAM : BLUE_TEAM;

      NetClient_SendMatchResult(state->net_client_state, winningTeam);

      state->currentGameState = GAME_STATE_FINISHED;
      state->winningTeam = winningTeam;
      hud_finish_msg(state);
    }

    SDL_Log("Base %d Destroyed", baseIndex);
  }

  if (sendToServer)
  {
    NetClient_SendDamageBaseRequest(state->net_client_state, baseIndex, damageValue);
  }
}