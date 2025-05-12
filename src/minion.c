#include "../include/minion.h"

struct MinionData
{
    bool team;
    SDL_FPoint position;      /**< Current world position (center). */
    SDL_FRect sprite_portion; /**< The source rect defining the current animation frame. */
    SDL_FlipMode flip_mode;   /**< Rendering flip state (horizontal). */
    bool active;              /**< Whether this minion slot is currently in use. */
    bool is_attacking;
    SDL_Texture *texture;
    int current_health; /**< Current health points. */
    float anim_timer;
    int current_frame;
    float attack_cooldown_timer;
};

struct MinionManager_s
{
    MinionData minions[MINION_MAX_AMOUNT];
    SDL_Texture *red_texture;
    SDL_Texture *blue_texture;
    Uint64 minionWaveTimer;
    Uint64 recentMinionTimer;
    int activeMinionAmount;
    int currentMinionWaveAmount;
    bool spawnNextMinion;
};

static void minion_manager_cleanup_callback(EntityManager manager, AppState *state)
{
    (void)manager;
    MinionManager mm = state ? state->minion_manager : NULL;
    if (!mm)
        return;

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "MinionManager entity cleanup callback triggered.");
    if (mm->red_texture || mm->blue_texture)
    {
        SDL_DestroyTexture(mm->blue_texture);
        SDL_DestroyTexture(mm->red_texture);
        mm->red_texture = NULL;
        mm->blue_texture = NULL;
    }
    SDL_free(mm);
}

static void update_local_minion_movment(MinionData *m, AppState *state)
{
    if (!m || !m->active)
        return;

    float move_x = 0.0f;
    float move_y = 0.0f;
    if (m->team)
    {
        move_x += 1.0f;
        move_y += 1.0f;
    }
    else
    {
        move_x -= 1.0f;
        move_y += 1.0f;
    }

    // --- Normalize and Apply Movement ---
    float len_sq = move_x * move_x + move_y * move_y;
    // Normalize the movement vector only if there is input, prevents division by zero
    // and ensures consistent speed regardless of direction (diagonal vs cardinal).
    if (len_sq > 0.001f)
    {
        float len = sqrtf(len_sq);
        move_x = (move_x / len) * MINION_SPEED * state->delta_time;
        move_y = (move_y / len) * MINION_SPEED * state->delta_time;
    }
    // Create Rect of Minion
    SDL_FRect minion_rect = {
        m->position.x + move_x - MINION_WIDTH / 2.0f,
        m->position.y + move_y - MINION_HEIGHT / 2.0f,
        MINION_WIDTH,
        MINION_HEIGHT};

    bool collision = false;
    for (int i = 0; i < MAX_TOTAL_TOWERS; i++)
    {
        TowerInstance temp_tower = state->tower_manager->towers[i];
        if (SDL_HasRectIntersectionFloat(&minion_rect, &temp_tower.rect))
        {
            if (temp_tower.team != m->team)
            {
                if (temp_tower.current_health > 0)
                {
                    m->is_attacking = true;
                    if ((SDL_GetTicks() - m->attack_cooldown_timer) > MINON_ATTACK_COOLDOWN)
                    {
                        damageTower(*state, i, MINON_DAMAGE_VALUE, true, 0);
                        m->attack_cooldown_timer = SDL_GetTicks();
                    }
                }
                else
                {
                    m->is_attacking = false;
                }
            }
            collision = true;
        }
    }

    if (!collision && !m->is_attacking)
    {
        m->position.x += move_x;
        if (m->position.y > BUILDINGS_POS_Y)
            m->position.y -= move_y;
    }
    else if (collision && !m->is_attacking)
    {
        m->position.y += move_y;
    }
}

static void update_local_minion_animation(MinionData *m, float delta_time)
{
    if (!m || !m->active)
        return;

    float target_row_y = m->is_attacking ? MINION_SPRITE_ATTACK : MINION_SPRITE_MOVE;

    // Switch animation sequence row if movement state changed.
    if (m->sprite_portion.y != target_row_y)
    {
        m->current_frame = 0;
        m->anim_timer = 0;
        m->sprite_portion.y = target_row_y;
    }
    m->anim_timer += delta_time;
    if (m->anim_timer >= MINION_SPRITE_TIME_PER_FRAME)
    {
        m->anim_timer -= MINION_SPRITE_TIME_PER_FRAME; // Subtract, don't reset, to handle frame skips.
        m->current_frame = (m->current_frame + 1) % MINION_SPRITE_NUM_FRAMES;
    }
    // Update the source rectangle for rendering based on the current frame.
    m->sprite_portion.x = (float)m->current_frame * MINION_SPRITE_FRAME_WIDTH;
    m->sprite_portion.w = MINION_SPRITE_FRAME_WIDTH;
    m->sprite_portion.h = MINION_SPRITE_FRAME_HEIGHT;
}

static bool Minion_Init(MinionManager mm, uint8_t minionIndex, bool team)
{
    if (!mm)
    {
        SDL_SetError("[Minion_Init] Invalid MinonManager\n");
        return false;
    }
    MinionData *currentMinion = &mm->minions[minionIndex];
    currentMinion->texture = mm->blue_texture;
    currentMinion->position = (SDL_FPoint){BASE_BLUE_POS_X - 350, BUILDINGS_POS_Y};
    currentMinion->flip_mode = SDL_FLIP_HORIZONTAL;

    if (team)
    {
        currentMinion->texture = mm->red_texture;
        currentMinion->position = (SDL_FPoint){BASE_RED_POS_X + 350, BUILDINGS_POS_Y};
        currentMinion->flip_mode = SDL_FLIP_NONE;
    }
    if (!currentMinion->texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Minion_Init] Failed load texture : %s", SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode(currentMinion->texture, SDL_SCALEMODE_NEAREST);
    currentMinion->sprite_portion = (SDL_FRect){0, MINION_SPRITE_MOVE, MINION_SPRITE_FRAME_WIDTH, MINION_SPRITE_FRAME_HEIGHT};
    currentMinion->current_health = MINION_HEALTH_MAX;
    currentMinion->anim_timer = 0;
    currentMinion->current_frame = 0;
    currentMinion->is_attacking = false;
    currentMinion->active = true;
    currentMinion->team = team;
    mm->activeMinionAmount++;

    SDL_Log("[Minion_Init] Initialized minion locally\n");

    return true;
}

static void minion_manager_update_callback(EntityManager manager, AppState *state)
{
    (void)manager;
    MinionManager mm = state ? state->minion_manager : NULL;
    if (!mm || !state)
        return;

    if ((SDL_GetTicks() - mm->minionWaveTimer) > 10000)
    {
        if ((SDL_GetTicks() - mm->recentMinionTimer) > 500)
        {
            Minion_Init(mm, mm->activeMinionAmount, BLUE_TEAM);
            Minion_Init(mm, mm->activeMinionAmount, RED_TEAM);
            mm->recentMinionTimer = SDL_GetTicks();
            mm->currentMinionWaveAmount++;

            if (mm->currentMinionWaveAmount == 6)
            {
                mm->currentMinionWaveAmount = 0;
                mm->minionWaveTimer = SDL_GetTicks();
            }
        }
    }

    for (int i = 0; i < MINION_MAX_AMOUNT; i++)
    {
        if (mm->minions[i].active)
        {
            update_local_minion_movment(&mm->minions[i], state);
            update_local_minion_animation(&mm->minions[i], state->delta_time);
        }
    }
}

static void render_single_minion(MinionData *m, AppState *state)
{
    CameraState camera = state->camera_state;
    float cam_x = Camera_GetX(camera);
    float cam_y = Camera_GetY(camera);
    // Calculate screen coordinates relative to the camera's view.
    float screen_x = m->position.x - cam_x - MINION_WIDTH / 2.0f;
    float screen_y = m->position.y - cam_y - MINION_HEIGHT / 2.0f;

    SDL_FRect dst_rect = {screen_x, screen_y, MINION_WIDTH, MINION_HEIGHT};
    SDL_RenderTextureRotated(state->renderer,
                             m->texture,
                             &m->sprite_portion, // Source rect from atlas
                             &dst_rect,          // Destination rect on screen
                             0.0,                // No rotation needed for player sprite
                             NULL,               // Render around center
                             m->flip_mode);      // Horizontal flip state
}

static void minion_manager_render_callback(EntityManager manager, AppState *state)
{
    (void)manager;
    MinionManager mm = state ? state->minion_manager : NULL;
    if (!mm || !state)
        return;

    // Render all minions currently marked as active.
    for (int i = 0; i < MINION_MAX_AMOUNT; i++)
    {
        if (mm->minions[i].active)
        {
            render_single_minion(&mm->minions[i], state);
        }
    }
}

MinionManager MinionManager_Init(AppState *state)
{
    if (!state || !state->renderer || !state->entity_manager)
    {
        SDL_SetError("Invalid AppState or missing renderer/entity_manager for MinionManager_Init");
        return NULL;
    }
    MinionManager mm = (MinionManager)SDL_calloc(1, sizeof(struct MinionManager_s));
    if (!mm)
    {
        SDL_OutOfMemory();
        SDL_Log("Error [MinionManager_Init] failed to allocate memory for MinionManager: %s\n", SDL_GetError());
        return NULL;
    }

    mm->minionWaveTimer = 0;
    mm->recentMinionTimer = 0;
    mm->activeMinionAmount = 0;
    mm->currentMinionWaveAmount = 0;
    mm->spawnNextMinion = false;

    mm->blue_texture = IMG_LoadTexture(state->renderer, BLUE_MINION_PATH);
    mm->red_texture = IMG_LoadTexture(state->renderer, RED_MINION_PATH);

    if (!mm->blue_texture || !mm->red_texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[MinionManager Init] Failed load texture : %s", SDL_GetError());
        SDL_free(mm);
        return NULL;
    }

    for (int i = 0; i < MINION_MAX_AMOUNT; i++)
    {
        mm->minions[i].active = false;
    }

    EntityFunctions minion_funcs = {
        .name = "minion_manager",
        .update = minion_manager_update_callback,
        .render = minion_manager_render_callback,
        .cleanup = minion_manager_cleanup_callback,
        .handle_events = NULL};

    if (!EntityManager_Add(state->entity_manager, &minion_funcs))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[MinionManager Init] Failed to add entity to manager: %s", SDL_GetError());
        if (mm->red_texture || mm->red_texture)
            SDL_DestroyTexture(mm->red_texture);
        SDL_DestroyTexture(mm->blue_texture);
        SDL_free(mm);
        return NULL;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "MinionManager initialized and entity registered.");
    return mm;
}

void MinionManager_Destroy(MinionManager mm)
{
    if (mm)
    {
        mm->blue_texture = NULL;
        mm->red_texture = NULL;
        SDL_free(mm);
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "MinonManager state container destroyed.");
    }
}