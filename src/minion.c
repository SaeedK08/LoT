#include "../include/minion.h"

struct MinionData {
    bool team;
    SDL_FPoint position;      /**< Current world position (center). */
    SDL_FRect sprite_portion; /**< The source rect defining the current animation frame. */
    SDL_FlipMode flip_mode;   /**< Rendering flip state (horizontal). */
    bool active;              /**< Whether this minion slot is currently in use. */
    bool is_local;            /**< True if this is the player controlled by this game instance. */
    bool is_attacking;
    SDL_Texture *texture;
    int current_health; /**< Current health points. */
    float anim_timer;
    int current_frame;
};

struct MinionManager_s {
    MinionData minions[MAX_CLIENTS];
    SDL_Texture *red_texture;
    SDL_Texture *blue_texture;
    int local_client_id;
};

static void minion_manager_cleanup_callback(EntityManager manager, AppState *state){
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

static void update_local_minion_movment(MinionData *m, AppState *state) {
    if (!m || !m->active)
        return;

    float move_x = 0.0f;
    float move_y = 0.0f;
    if (m->team) {
        move_x += 1.0f;
        move_y += 1.0f;
        
    } else {
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
    // Creat Rect of Minion
    SDL_FRect minion_rect = {
        m->position.x + move_x - MINION_WIDTH / 2.0f,
        m->position.y + move_y - MINION_HEIGHT / 2.0f,
        MINION_WIDTH,
        MINION_HEIGHT
    };

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
                    damageTower(*state, i, MINON_DAMAGE_VALUE, true, 0);
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

static void update_local_minion_animation(MinionData *m, float delta_time) {
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

static void minion_manager_update_callback(EntityManager manager, AppState *state)
{
    (void)manager;
    MinionManager mm = state ? state->minion_manager : NULL;
    if (!mm || !state)
        return;

    if (mm->local_client_id >= 0)
    {
        update_local_minion_movment(&mm->minions[mm->local_client_id], state);
        update_local_minion_animation(&mm->minions[mm->local_client_id], state->delta_time);
    }
}

static void render_single_minion(MinionData *m, AppState *state) {
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

    // Render all players currently marked as active.
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (mm->minions[i].active)
        {
            render_single_minion(&mm->minions[i], state);
        }
    }
}


MinionManager MinionManager_Init(AppState *state) {
    if (!state || !state->renderer || !state->entity_manager)
    {
        SDL_SetError("Invalid AppState or missing renderer/entity_manager for MinionManager_Init");
        return NULL;
    }
    MinionManager mm = (MinionManager)SDL_calloc(1, sizeof(struct MinionManager_s));
    if(!mm)
    {
        SDL_OutOfMemory();
        SDL_Log("Error [MinionManager_Init] failed to allocate memory for MinionManager: %s\n", SDL_GetError());
        return NULL;
    }

    mm->local_client_id = -1;
    mm->blue_texture = IMG_LoadTexture(state->renderer, BLUE_MINION_PATH);
    mm->red_texture = IMG_LoadTexture(state->renderer, RED_MINION_PATH);

    if (!mm->blue_texture || !mm->red_texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[MinionManager Init] Failed load texture : %s", SDL_GetError());
        SDL_free(mm);
        return NULL;
    }

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        mm->minions[i].active = false;
        mm->minions[i].is_local = false;
        mm->minions[i].team = state->team;
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


void MinionManager_UpdateRemoteMinion(MinionManager mm, const Msg_MinionStateData *data)
{
    if (!mm || !data || data->client_id >= MAX_CLIENTS)
        return;

    // Ignore updates intended for the local player's minion received over the network.
    if (data->client_id == mm->local_client_id) return;

    uint8_t id = data->client_id;
    // Activate the minion slot if this is the first time we hear about this ID
    if (!mm->minions[id].active)
    {
        memset(&mm->minions[id], 0, sizeof(MinionData));    // Claer the slot before use.
        mm->minions[id].active = true;
        mm->minions[id].is_local = false;
        mm->minions[id].team = data->team;
        mm->minions[id].flip_mode = data->flip_mode;
        mm->minions[id].texture = mm->blue_texture;
        mm->minions[id].current_health = data->current_health;
        if (mm->minions[id].team) 
        {
            mm->minions[id].texture = mm->red_texture;
        }
    }

    mm->minions[id].position = data->position;
    mm->minions[id].sprite_portion = data->sprite_portion;
        
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

bool Minion_Init(MinionManager mm, uint8_t client_id, TowerManagerState tm, BaseManagerState bm) {
    if (!mm)
    {
        SDL_SetError("[Minion_Init] Invalid MinonManager\n");
        return false;
    }
    mm->local_client_id = client_id;
    mm->minions[client_id].texture = mm->blue_texture;
    mm->minions[client_id].position = (SDL_FPoint){BASE_BLUE_POS_X - 350, BUILDINGS_POS_Y};
    mm->minions[client_id].flip_mode = SDL_FLIP_HORIZONTAL;

    if (mm->minions[client_id].team) {
         mm->minions[client_id].texture = mm->red_texture;
         mm->minions[client_id].position = (SDL_FPoint){BASE_RED_POS_X + 350, BUILDINGS_POS_Y};
         mm->minions[client_id].flip_mode = SDL_FLIP_NONE;
    }
    if (!mm->minions[client_id].texture) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[ini_minion] Failed load texture : %s", SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode(mm->minions[client_id].texture, SDL_SCALEMODE_NEAREST);
    mm->minions[client_id].sprite_portion = (SDL_FRect) {0,MINION_SPRITE_MOVE,MINION_SPRITE_FRAME_WIDTH, MINION_SPRITE_FRAME_HEIGHT};
    mm->minions[client_id].active = true;
    mm->minions[client_id].is_local = true;
    mm->minions[client_id].current_health = MINION_HEALTH_MAX;
    mm->minions[client_id].anim_timer = 0;
    mm->minions[client_id].current_frame = 0;
    mm->minions[client_id].is_attacking = false;
    SDL_Log("[Minion_Init] Initialized minion locally\n");
    return true;
}


bool MinionManager_GetLocalMinionState(MinionManager mm, Msg_MinionStateData *out_data) {
    if (!mm || !out_data || mm->local_client_id < 0 || !mm->minions[mm->local_client_id].active)
    {
        return false;
    }

    MinionData *m = &mm->minions[mm->local_client_id];

    out_data->message_type = MSG_TYPE_C_MINION_STATE;
    out_data->client_id = (uint8_t) mm->local_client_id;
    out_data->position = m->position;
    out_data->sprite_portion = m->sprite_portion;
    out_data->flip_mode = m->flip_mode;
    out_data->team = m->team;
    out_data->current_health = m->current_health;

    return true;
}
