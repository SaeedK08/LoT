#include "../include/tower.h"

// --- Static Helper Functions ---

/**
 * @brief Renders a single tower instance relative to the camera.
 * @param tower Pointer to the TowerInstance to render.
 * @param state Pointer to the main AppState.
 */
static void render_single_tower(const TowerInstance *tower, AppState *state)
{
    if (!tower || !tower->is_active || !tower->texture || !state || !state->renderer || !state->camera_state)
    {
        return;
    }

    CameraState camera = state->camera_state;
    float cam_x = Camera_GetX(camera);
    float cam_y = Camera_GetY(camera);

    // Calculate screen position, rendering upwards from the defined position point.
    SDL_FRect dst_rect = {
        .x = tower->position.x - cam_x - TOWER_RENDER_WIDTH / 2.0f,
        .y = tower->position.y - cam_y - TOWER_RENDER_HEIGHT / 2.0f,
        .w = TOWER_RENDER_WIDTH,
        .h = TOWER_RENDER_HEIGHT};

    SDL_RenderTexture(state->renderer, tower->texture, NULL, &dst_rect);
}

/**
 * @brief Updates a single tower instance, handling attack cooldowns and initiating attacks (server-only).
 * @param tower Pointer to the TowerInstance to update.
 * @param state Pointer to the main AppState.
 * @param towerIndex The index of this tower within the TowerManager's array.
 */
static void update_single_tower(TowerInstance *tower, AppState *state, int towerIndex)
{
    if (!tower || !tower->is_active || !state)
        return;

    if (tower->attack_cooldown_timer > 0.0f)
    {
        tower->attack_cooldown_timer -= state->delta_time;
    }

    if (!state->is_server)
    {
        return;
    }

    if (tower->attack_cooldown_timer <= 0.0f)
    {
        if (!state->player_manager || !state->attack_manager)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Missing PlayerManager or AttackManager for tower %d attack logic.", towerIndex);
            return;
        }

        PlayerManager pm = state->player_manager;
        AttackManager am = state->attack_manager;
        SDL_FPoint target_pos = {0.0f, 0.0f};
        bool target_found = false;
        float min_dist_sq = TOWER_ATTACK_RANGE * TOWER_ATTACK_RANGE;

        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (pm->players[i].active && pm->players[i].team != tower->team)
            {
                SDL_FPoint player_pos;
                if (PlayerManager_GetPlayerPosition(pm, i, &player_pos))
                {
                    float dx = player_pos.x - tower->position.x;
                    float dy = player_pos.y - tower->position.y;
                    float dist_sq = dx * dx + dy * dy;

                    if (dist_sq <= min_dist_sq)
                    {
                        min_dist_sq = dist_sq;
                        target_pos = player_pos;
                        target_found = true;
                    }
                }
            }
        }

        if (target_found)
        {
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Tower %d targeting player at (%.1f, %.1f)", towerIndex, target_pos.x, target_pos.y);

            AttackManager_ServerSpawnTowerAttack(am, state, TOWER_ATTACK_TYPE, target_pos, towerIndex);

            tower->attack_cooldown_timer = TOWER_ATTACK_COOLDOWN;
        }
    }
}

// --- Static Callback Functions (for EntityManager) ---

/**
 * @brief Internal function to update all active towers.
 * @param tm_state The internal state of the tower manager module.
 * @param state The main application state.
 */
static void Internal_TowerManagerUpdate(TowerManagerState tm_state, AppState *state)
{
    if (!tm_state || !state)
        return;

    // Only run tower updates on the server
    if (!state->is_server)
    {
        return;
    }

    for (int i = 0; i < tm_state->tower_count; ++i)
    {
        // Pass the index 'i' to the update function
        update_single_tower(&tm_state->towers[i], state, i);
    }
}

/**
 * @brief Internal function to render all active towers.
 * @param tm_state The internal state of the tower manager module.
 * @param state The main application state.
 */
static void Internal_TowerManagerRender(TowerManagerState tm_state, AppState *state)
{
    if (!tm_state || !state)
        return;
    for (int i = 0; i < tm_state->tower_count; ++i)
    {
        render_single_tower(&tm_state->towers[i], state);
    }
}

/**
 * @brief Internal function to clean up TowerManager resources.
 * @param tm_state The internal state of the tower manager module.
 */
static void Internal_TowerManagerCleanup(TowerManagerState tm_state)
{
    if (!tm_state)
        return;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Cleaning up TowerManager resources...");
    if (tm_state->red_texture)
    {
        SDL_DestroyTexture(tm_state->red_texture);
        tm_state->red_texture = NULL;
    }
    if (tm_state->blue_texture)
    {
        SDL_DestroyTexture(tm_state->blue_texture);
        tm_state->blue_texture = NULL;
    }
}

/**
 * @brief Wrapper function conforming to EntityFunctions.update signature.
 * @param manager The EntityManager instance.
 * @param state Pointer to the main AppState.
 */
static void tower_manager_update_callback(EntityManager manager, AppState *state)
{
    (void)manager; // Manager instance is not used in this specific implementation
    Internal_TowerManagerUpdate(state->tower_manager, state);
}

/**
 * @brief Wrapper function conforming to EntityFunctions.render signature.
 * @param manager The EntityManager instance.
 * @param state Pointer to the main AppState.
 */
static void tower_manager_render_callback(EntityManager manager, AppState *state)
{
    (void)manager; // Manager instance is not used in this specific implementation
    Internal_TowerManagerRender(state->tower_manager, state);
}

/**
 * @brief Wrapper function conforming to EntityFunctions.cleanup signature.
 * @param manager The EntityManager instance.
 * @param state Pointer to the main AppState.
 */
static void tower_manager_cleanup_callback(EntityManager manager, AppState *state)
{
    (void)manager; // Manager instance is not used in this specific implementation
    Internal_TowerManagerCleanup(state ? state->tower_manager : NULL);
    if (state)
    {
        state->tower_manager = NULL; // Indicate cleanup happened
    }
}

// --- Public API Function Implementations ---

TowerManagerState TowerManager_Init(AppState *state)
{
    if (!state || !state->renderer || !state->entity_manager)
    {
        SDL_SetError("Invalid AppState or missing renderer/entity_manager for TowerManager_Init");
        return NULL;
    }

    // --- Allocate State ---
    TowerManagerState tm_state = (TowerManagerState)SDL_calloc(1, sizeof(struct TowerManagerState_s));
    if (!tm_state)
    {
        SDL_OutOfMemory();
        return NULL;
    }
    tm_state->tower_count = 0;

    // --- Load Resources ---
    const char red_tower_path[] = "./resources/Sprites/Red_Team/Tower_Red.png";
    const char blue_tower_path[] = "./resources/Sprites/Blue_Team/Tower_Blue.png";
    const char destroyed_tower_path[] = "./resources/Sprites/Tower_Destroyed.png";

    tm_state->red_texture = IMG_LoadTexture(state->renderer, red_tower_path);
    if (!tm_state->red_texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Tower Init] Failed load texture '%s': %s", red_tower_path, SDL_GetError());
        SDL_free(tm_state);
        return NULL;
    }
    SDL_SetTextureScaleMode(tm_state->red_texture, SDL_SCALEMODE_NEAREST);

    tm_state->blue_texture = IMG_LoadTexture(state->renderer, blue_tower_path);
    if (!tm_state->blue_texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Tower Init] Failed load texture '%s': %s", blue_tower_path, SDL_GetError());
        SDL_DestroyTexture(tm_state->red_texture); // Clean up already loaded texture
        SDL_free(tm_state);
        return NULL;
    }
    SDL_SetTextureScaleMode(tm_state->blue_texture, SDL_SCALEMODE_NEAREST);

    tm_state->destroyed_texture = IMG_LoadTexture(state->renderer, destroyed_tower_path);
    if (!tm_state->destroyed_texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Tower Init] Failed load texture '%s': %s", destroyed_tower_path, SDL_GetError());
        SDL_DestroyTexture(tm_state->red_texture); // Clean up already loaded texture
        SDL_free(tm_state);
        return NULL;
    }
    SDL_SetTextureScaleMode(tm_state->destroyed_texture, SDL_SCALEMODE_NEAREST);

    for (int i = 0; i < MAX_TOWERS_PER_TEAM; i++)
    {
        tm_state->towers[i] = (TowerInstance){
            .position = {TOWER_RED_1_X + i * TOWER_DISTANCE_X, BUILDINGS_POS_Y},
            .texture = tm_state->red_texture,
            .max_health = TOWER_HEALTH_MAX,
            .current_health = TOWER_HEALTH_MAX,
            .is_active = true,
            .attack_cooldown_timer = 0.0f,
            .team = RED_TEAM};

        tm_state->towers[i].rect = (SDL_FRect){(TOWER_RED_1_X + i * TOWER_DISTANCE_X) - TOWER_RENDER_WIDTH / 2.0f,
                                               BUILDINGS_POS_Y - TOWER_RENDER_HEIGHT / 2.0f,
                                               TOWER_RENDER_WIDTH,
                                               TOWER_RENDER_HEIGHT};
    }
    for (int i = MAX_TOWERS_PER_TEAM; i < MAX_TOTAL_TOWERS; i++)
    {
        tm_state->towers[i] = (TowerInstance){.position = {TOWER_BLUE_1_X - (i - MAX_TOWERS_PER_TEAM) * TOWER_DISTANCE_X, BUILDINGS_POS_Y},
                                              .texture = tm_state->blue_texture,
                                              .max_health = TOWER_HEALTH_MAX,
                                              .current_health = TOWER_HEALTH_MAX,
                                              .is_active = true,
                                              .attack_cooldown_timer = 0.0f,
                                              .team = BLUE_TEAM};

        tm_state->towers[i].rect = (SDL_FRect){(TOWER_BLUE_1_X - (i - MAX_TOWERS_PER_TEAM) * TOWER_DISTANCE_X) - TOWER_RENDER_WIDTH / 2.0f,
                                               BUILDINGS_POS_Y - TOWER_RENDER_HEIGHT / 2.0f,
                                               TOWER_RENDER_WIDTH,
                                               TOWER_RENDER_HEIGHT};
    }

    tm_state->tower_count = MAX_TOTAL_TOWERS;

    // --- Register with EntityManager ---
    EntityFunctions tower_funcs = {
        .name = "tower_manager",
        .update = tower_manager_update_callback,
        .render = tower_manager_render_callback,
        .cleanup = tower_manager_cleanup_callback,
        .handle_events = NULL};

    if (!EntityManager_Add(state->entity_manager, &tower_funcs))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Tower Init] Failed to add entity to manager: %s", SDL_GetError());
        Internal_TowerManagerCleanup(tm_state); // Cleanup loaded textures
        SDL_free(tm_state);
        return NULL;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "TowerManager initialized and entity registered.");
    return tm_state;
}

void TowerManager_Destroy(TowerManagerState tm_state)
{
    // Cleanup callback handles texture destruction.
    if (tm_state)
    {
        // Prevent dangling pointers after cleanup callback potentially ran via EntityManager
        tm_state->red_texture = NULL;
        tm_state->blue_texture = NULL;
        SDL_free(tm_state);
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "TowerManagerState container destroyed.");
    }
}

void damageTower(AppState state, int towerIndex, float damageValue, bool sendToServer, float current_health)
{
    // if (state.tower_manager->towers[towerIndex].current_health > 0)
    // {
    //     state.tower_manager->towers[towerIndex].current_health -= damageValue;
    //     SDL_Log("Tower %d health %f", towerIndex, state.tower_manager->towers[towerIndex].current_health);
    // }
    if (!sendToServer)
    {   
        state.tower_manager->towers[towerIndex].current_health = current_health;
        SDL_Log("Received tower index %d from server | current health after taking damange %f\n", towerIndex, state.tower_manager->towers[towerIndex].current_health);
    }
    if (sendToServer && state.tower_manager->towers[towerIndex].current_health > 0)
    {
        state.tower_manager->towers[towerIndex].current_health -= damageValue;
        current_health = state.tower_manager->towers[towerIndex].current_health;
        SDL_Log("Tower %d health %f", towerIndex, state.tower_manager->towers[towerIndex].current_health);
        NetClient_SendDamageTowerRequest(state.net_client_state, towerIndex, damageValue, current_health);
    }

    if (state.tower_manager->towers[towerIndex].current_health <= 0)
    {
        if (!sendToServer) SDL_Log("\n\n[client] Received Destroyed Tower.\n\n");
        state.tower_manager->towers[towerIndex].texture = state.tower_manager->destroyed_texture;
        SDL_Log("Tower %d Destroyed", towerIndex);
    }
}
