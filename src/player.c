#include "../include/player.h"

// --- Internal Structures ---

// --- Static Helper Functions ---

static void playerDeathTimer(PlayerInstance *p)
{
    if ((SDL_GetTicks() - p->deathTime) >= PLAYER_DEATH_TIMER)
    {
        SDL_Log("Player is back to life");
        p->dead = false;
        p->playDeathAnim = false;
        p->current_health = p->max_health;
    }
}

/**
 * @brief Handles input processing (movement) for the local player.
 * Reads keyboard state, calculates new position based on input and delta time,
 * updates movement state, and clamps position to map boundaries.
 * @param pm The PlayerManager instance.
 * @param state The main application state.
 */
static void handle_local_player_input(PlayerManager pm, AppState *state)
{
    if (!pm || pm->local_player_client_id < 0 || !state || pm->players[pm->local_player_client_id].dead)
        return;

    PlayerInstance *p = &pm->players[pm->local_player_client_id];
    const bool *keyboard_state = SDL_GetKeyboardState(NULL);
    bool was_moving = p->is_moving; // Track previous state to detect changes for animation reset.
    p->is_moving = false;

    float move_x = 0.0f;
    float move_y = 0.0f;

    // --- Read Input ---
    // Accumulate input direction components.
    if (keyboard_state[SDL_SCANCODE_W])
    {
        move_y -= 1.0f;
        p->is_moving = true;
    }
    if (keyboard_state[SDL_SCANCODE_S])
    {
        move_y += 1.0f;
        p->is_moving = true;
    }
    if (keyboard_state[SDL_SCANCODE_A])
    {
        move_x -= 1.0f;
        p->flip_mode = SDL_FLIP_HORIZONTAL; // Face left when moving left.
        p->is_moving = true;
    }
    if (keyboard_state[SDL_SCANCODE_D])
    {
        move_x += 1.0f;
        p->flip_mode = SDL_FLIP_NONE; // Face right when moving right.
        p->is_moving = true;
    }

    // --- Normalize and Apply Movement ---
    float len_sq = move_x * move_x + move_y * move_y;
    // Normalize the movement vector only if there is input, prevents division by zero
    // and ensures consistent speed regardless of direction (diagonal vs cardinal).
    if (len_sq > 0.001f)
    {
        float len = sqrtf(len_sq);
        move_x = (move_x / len) * PLAYER_SPEED * state->delta_time;
        move_y = (move_y / len) * PLAYER_SPEED * state->delta_time;
    }

    // Create Rect of the player
    SDL_FRect player_bounds = {
        move_x - PLAYER_WIDTH / 2.0f + p->position.x,
        move_y - PLAYER_HEIGHT / 2.0f + p->position.y,
        PLAYER_WIDTH,
        PLAYER_HEIGHT};

    bool collision = false;

    for (int i = 0; i < MAX_TOTAL_TOWERS; i++)
    {
        if (SDL_HasRectIntersectionFloat(&player_bounds, &state->tower_manager->towers[i].rect))
        {
            collision = true;
        }
    }

    for (int i = 0; i < MAX_BASES; i++)
    {
        if (SDL_HasRectIntersectionFloat(&player_bounds, &state->base_manager->bases[i].rect))
        {
            collision = true;
        }
    }

    if (!collision) // If player doesn't intersect, update position
    {
        p->position.x += move_x;
        p->position.y += move_y;
    }

    // --- Clamp Position ---
    if (state->map_state)
    {
        // Prevent player from moving outside the map horizontally.
        p->position.x = fmaxf(PLAYER_WIDTH / 2.0f, fminf(p->position.x, Map_GetWidthPixels(state->map_state) - PLAYER_WIDTH / 2.0f));
        // Prevent player from moving outside the map vertically.
        p->position.y = fmaxf(CLIFF_BOUNDARY, fminf(p->position.y, WATER_BOUNDARY));
    }

    p->rect = (SDL_FRect){
        p->position.x - PLAYER_WIDTH / 2.0f,
        p->position.y - PLAYER_HEIGHT / 2.0f,
        PLAYER_WIDTH,
        PLAYER_HEIGHT};

    // --- Animation State Reset ---
    // If movement state changed (started/stopped moving), reset animation to the beginning.
    if (p->is_moving != was_moving)
    {
        p->current_frame = 0;
        p->anim_timer = 0.0f;
    }
}
/**
 * @brief Updates the animation state (current frame, sprite portion) for a specific player.
 * Prioritizes animations: Dead > Hurt > Attack > Walk > Idle.
 * Handles looping for Idle/Walk and playing once for Hurt/Attack/Dead.
 * @param p Pointer to the PlayerInstance to update.
 * @param delta_time Time since the last frame.
 */
static void update_player_animation(PlayerInstance *p, float delta_time)
{
    if (!p || !p->active)
        return;

    // --- Local Player Animation Logic ---
    if (p->is_local)
    {
        float target_row_y = PLAYER_SPRITE_IDLE_ROW_Y;  // Default to Idle
        int num_frames = PLAYER_SPRITE_NUM_IDLE_FRAMES; // Default to Idle
        bool should_loop = true;                        // Default to looping (Idle/Walk)
        bool is_one_shot_anim_finished = false;         // Track if Hurt/Attack/Dead is done

        // --- Determine Target Animation State ---
        if (p->dead)
        {
            target_row_y = PLAYER_SPRITE_DEAD_ROW_Y;
            num_frames = PLAYER_SPRITE_NUM_DEAD_FRAMES;
            should_loop = false;
            is_one_shot_anim_finished = (p->sprite_portion.y == target_row_y && p->current_frame == num_frames - 1);
        }
        else if (p->playHurtAnim)
        {
            target_row_y = PLAYER_SPRITE_HURT_ROW_Y;
            num_frames = PLAYER_SPRITE_NUM_HURT_FRAMES;
            should_loop = false;
            is_one_shot_anim_finished = (p->sprite_portion.y == target_row_y && p->current_frame == num_frames - 1);
        }
        else if (p->playAttackAnim)
        {
            target_row_y = PLAYER_SPRITE_ATTACK_ROW_Y;
            num_frames = PLAYER_SPRITE_NUM_ATTACK_FRAMES;
            should_loop = false;
            is_one_shot_anim_finished = (p->sprite_portion.y == target_row_y && p->current_frame == num_frames - 1);
        }
        else if (p->is_moving) // Default moving state
        {
            target_row_y = PLAYER_SPRITE_WALK_ROW_Y;
            num_frames = PLAYER_SPRITE_NUM_WALK_FRAMES;
            should_loop = true;
        }
        // Default Idle state vars are already set if none of the above are true

        // --- Handle Animation Transition / Reset ---
        if (p->sprite_portion.y != target_row_y)
        {
            // Switched to a new animation type, reset frame and timer
            p->current_frame = 0;
            p->anim_timer = 0.0f;
            p->sprite_portion.y = target_row_y; // Set the new row for the source rect
            is_one_shot_anim_finished = false;  // New animation isn't finished yet
        }

        // --- Advance Frame Timer ---
        // Only advance timer if the animation is not a finished one-shot animation
        if (!is_one_shot_anim_finished)
        {
            p->anim_timer += delta_time;
            if (p->anim_timer >= PLAYER_SPRITE_TIME_PER_FRAME)
            {
                p->anim_timer -= PLAYER_SPRITE_TIME_PER_FRAME;

                if (should_loop)
                {
                    // Loop animation
                    p->current_frame = (p->current_frame + 1) % num_frames;
                }
                else
                {
                    // Play-once animation: increment frame, but don't exceed last frame
                    if (p->current_frame < num_frames - 1)
                    {
                        p->current_frame++;
                    }
                    // Check AGAIN if we just reached the last frame *after* incrementing
                    is_one_shot_anim_finished = (p->current_frame == num_frames - 1);
                }
            }
        }

        // --- Reset One-Shot Flags if finished ---
        if (is_one_shot_anim_finished)
        {
            if (p->playHurtAnim)
                p->playHurtAnim = false;
            if (p->playAttackAnim)
                p->playAttackAnim = false;
        }

        // --- Update Source Rect ---
        p->sprite_portion.x = (float)p->current_frame * PLAYER_SPRITE_FRAME_WIDTH;
        p->sprite_portion.w = PLAYER_SPRITE_FRAME_WIDTH;
        p->sprite_portion.h = PLAYER_SPRITE_FRAME_HEIGHT;
        // Y value was potentially set during state transition
    }
    // --- Remote Player Animation ---
    else
    {
        p->sprite_portion.w = PLAYER_SPRITE_FRAME_WIDTH;
        p->sprite_portion.h = PLAYER_SPRITE_FRAME_HEIGHT;
        p->dead = (fabsf(p->sprite_portion.y - PLAYER_SPRITE_DEAD_ROW_Y) < 0.1f);
    }
}

/**
 * @brief Renders a single player (local or remote) to the screen.
 * Calculates screen position based on player world position and camera state.
 * Uses the player's current sprite portion and flip mode.
 * @param pm The PlayerManager instance.
 * @param p Pointer to the PlayerInstance to render.
 * @param state The main application state.
 */
static void render_single_player(PlayerManager pm, PlayerInstance *p, AppState *state)
{
    if (!pm || !p || !p->active || !pm->player_texture || !state || !state->camera_state)
    {
        return;
    }

    if (p->dead)
    {
        playerDeathTimer(p);
    }

    CameraState camera = state->camera_state;
    float cam_x = Camera_GetX(camera);
    float cam_y = Camera_GetY(camera);

    // Calculate screen coordinates relative to the camera's view.
    float screen_x = p->position.x - cam_x - PLAYER_WIDTH / 2.0f;
    float screen_y = p->position.y - cam_y - PLAYER_HEIGHT / 2.0f;

    SDL_FRect dst_rect = {screen_x, screen_y, PLAYER_WIDTH, PLAYER_HEIGHT};

    SDL_RenderTextureRotated(state->renderer,
                             p->texture,
                             &p->sprite_portion, // Source rect from atlas
                             &dst_rect,          // Destination rect on screen
                             0.0,                // No rotation needed for player sprite
                             NULL,               // Render around center
                             p->flip_mode);      // Horizontal flip state
}

// --- Static Callback Functions (for EntityManager) ---

/**
 * @brief Entity update callback for the PlayerManager.
 * Updates local player input and animation for all active players.
 * @param manager The EntityManager instance (unused).
 * @param state Pointer to the main AppState.
 */
static void player_manager_update_callback(EntityManager manager, AppState *state)
{
    (void)manager;

    PlayerManager pm = state ? state->player_manager : NULL;
    if (!pm || !state)
        return;

    // --- Update Local Player ---
    if (pm->local_player_client_id >= 0)
    {
        handle_local_player_input(pm, state);
        update_player_animation(&pm->players[pm->local_player_client_id], state->delta_time);
    }
}

/**
 * @brief Entity render callback for the PlayerManager.
 * Renders all active players (local and remote).
 * @param manager The EntityManager instance (unused).
 * @param state Pointer to the main AppState.
 */
static void player_manager_render_callback(EntityManager manager, AppState *state)
{
    (void)manager;
    PlayerManager pm = state ? state->player_manager : NULL;
    if (!pm || !state)
        return;

    // Render all players currently marked as active.
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (pm->players[i].active)
        {
            render_single_player(pm, &pm->players[i], state);
        }
    }
}

/**
 * @brief Entity event handling callback for the PlayerManager.
 * Handles input events specifically for the local player (triggering attacks).
 * @param manager The EntityManager instance (unused).
 * @param state Pointer to the main AppState.
 * @param event The SDL_Event to handle.
 */
static void player_manager_event_callback(EntityManager manager, AppState *state, SDL_Event *event)
{
    (void)manager;
    PlayerManager pm = state ? state->player_manager : NULL;
    // Ensure local player exists and required managers are available.
    if (!pm || pm->local_player_client_id < 0 || !state || !state->attack_manager || !state->camera_state || !state->net_client_state || pm->players[pm->local_player_client_id].dead)
    {
        return;
    }

    PlayerInstance *local_player = &pm->players[pm->local_player_client_id];
    CameraState camera = state->camera_state;

    // --- Handle Attack Input ---
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT)
    {
        int window_w, window_h;
        SDL_GetWindowSize(state->window, &window_w, &window_h);
        float cam_w = Camera_GetWidth(camera);
        float cam_h = Camera_GetHeight(camera);
        if (cam_w <= 0 || cam_h <= 0)
            return; // Avoid division by zero if camera isn't ready.

        // Calculate scaling factor between window size and logical camera size.
        float scale_x = (float)window_w / cam_w;
        float scale_y = (float)window_h / cam_h;

        // Convert mouse screen coordinates to camera view coordinates.
        float mouse_view_x = event->button.x / scale_x;
        float mouse_view_y = event->button.y / scale_y;

        // Convert camera view coordinates to world coordinates.
        float target_world_x = mouse_view_x + Camera_GetX(camera);
        float target_world_y = mouse_view_y + Camera_GetY(camera);

        // Check if the target is within the player's attack range.
        float dist_x = target_world_x - local_player->position.x;
        float dist_y = target_world_y - local_player->position.y;
        float distance = sqrtf(dist_x * dist_x + dist_y * dist_y);

        if (distance <= PLAYER_ATTACK_RANGE)
        {
            state->player_manager->players[state->player_manager->local_player_client_id].playAttackAnim = true;
            // Send request to the network client module to inform the server.
            NetClient_SendSpawnAttackRequest(state->net_client_state, PLAYER_ATTACK_TYPE_FIREBALL, target_world_x, target_world_y, local_player->team);
        }
    }
}

/**
 * @brief Entity cleanup callback for the PlayerManager.
 * Cleans up resources managed by the PlayerManager.
 * @param manager The EntityManager instance (unused).
 * @param state Pointer to the main AppState.
 */
static void player_manager_cleanup_callback(EntityManager manager, AppState *state)
{
    (void)manager;
    PlayerManager pm = state ? state->player_manager : NULL;
    if (!pm)
        return;

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "PlayerManager entity cleanup callback triggered.");
    if (pm->player_texture)
    {
        SDL_DestroyTexture(pm->player_texture);
        pm->player_texture = NULL;
    }
}

// --- Public API Function Implementations ---

PlayerManager PlayerManager_Init(AppState *state)
{
    if (!state || !state->renderer || !state->entity_manager)
    {
        SDL_SetError("Invalid AppState or missing renderer/entity_manager for PlayerManager_Init");
        return NULL;
    }

    PlayerManager pm = (PlayerManager)SDL_calloc(1, sizeof(struct PlayerManager_s));
    if (!pm)
    {
        SDL_OutOfMemory();
        return NULL;
    }

    pm->local_player_client_id = -1; // Initialize as having no local player yet.
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        pm->players[i].active = false;
        pm->players[i].is_local = false;
        pm->players[i].team = state->team;
    }

    pm->blue_texture = IMG_LoadTexture(state->renderer, BLUE_WIZARD_PATH);
    pm->red_texture = IMG_LoadTexture(state->renderer, RED_WIZARD_PATH);

    pm->player_texture = pm->blue_texture;

    if (state->team)
    {
        pm->player_texture = pm->red_texture;
    }

    if (!pm->player_texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[PlayerManager Init] Failed load texture : %s", SDL_GetError());
        SDL_free(pm);
        return NULL;
    }
    // Use nearest neighbor scaling for pixel art.
    SDL_SetTextureScaleMode(pm->player_texture, SDL_SCALEMODE_NEAREST);

    // --- Register with EntityManager ---
    EntityFunctions player_funcs = {
        .name = "player_manager",
        .update = player_manager_update_callback,
        .render = player_manager_render_callback,
        .cleanup = player_manager_cleanup_callback,
        .handle_events = player_manager_event_callback};

    if (!EntityManager_Add(state->entity_manager, &player_funcs))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[PlayerManager Init] Failed to add entity to manager: %s", SDL_GetError());
        if (pm->player_texture)
            SDL_DestroyTexture(pm->player_texture);
        SDL_free(pm);
        return NULL;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "PlayerManager initialized and entity registered.");
    return pm;
}

void PlayerManager_Destroy(PlayerManager pm)
{
    if (pm)
    {
        pm->player_texture = NULL; // Texture is destroyed in the cleanup callback.
        SDL_free(pm);
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "PlayerManager state container destroyed.");
    }
}

bool PlayerManager_SetLocalPlayerID(PlayerManager pm, uint8_t client_id)
{
    if (!pm || client_id >= MAX_CLIENTS)
    {
        SDL_SetError("Invalid PlayerManager or client ID (%u)", (unsigned int)client_id);
        return false;
    }
    if (pm->local_player_client_id != -1)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Local player ID already set to %d, ignoring new ID %u", pm->local_player_client_id, (unsigned int)client_id);
        return false; // Avoid setting local player twice.
    }

    pm->local_player_client_id = client_id;

    PlayerInstance *current_player = &pm->players[client_id];

    // Set initial state for the newly identified local player.
    current_player->active = true;
    current_player->is_local = true;
    current_player->sprite_portion = (SDL_FRect){0.0f, PLAYER_SPRITE_IDLE_ROW_Y, PLAYER_SPRITE_FRAME_WIDTH, PLAYER_SPRITE_FRAME_HEIGHT};
    current_player->flip_mode = SDL_FLIP_NONE;
    current_player->is_moving = false;
    current_player->current_frame = 0;
    current_player->anim_timer = 0.0f;
    current_player->texture = pm->player_texture;
    current_player->max_health = PLAYER_HEALTH_MAX;
    current_player->current_health = PLAYER_HEALTH_MAX;

    // Initial spawn position.
    current_player->position = (SDL_FPoint){BASE_BLUE_POS_X - 300, BUILDINGS_POS_Y};

    if (current_player->team)
    {
        current_player->position = (SDL_FPoint){BASE_RED_POS_X + 300, BUILDINGS_POS_Y};
    }

    current_player->rect = (SDL_FRect){
        current_player->position.x - PLAYER_WIDTH / 2.0f,
        current_player->position.y - PLAYER_HEIGHT / 2.0f,
        PLAYER_WIDTH,
        PLAYER_HEIGHT};

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Local player ID set to %d", client_id);
    return true;
}

void PlayerManager_UpdateRemotePlayer(PlayerManager pm, const Msg_PlayerStateData *data)
{
    if (!pm || !data || data->client_id >= MAX_CLIENTS)
        return;

    // Ignore updates intended for the local player received over the network.
    if (data->client_id == pm->local_player_client_id)
        return;

    uint8_t id = data->client_id;
    // Activate the player slot if this is the first time we hear about this ID.
    if (!pm->players[id].active)
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Activating remote player %u", (unsigned int)id);
        memset(&pm->players[id], 0, sizeof(PlayerInstance)); // Clear slot before use.
        pm->players[id].active = true;
        pm->players[id].is_local = false;
        pm->players[id].team = data->team;
        pm->players[id].texture = pm->blue_texture;
        pm->players[id].current_health = data->current_health;

        if (pm->players[id].team)
        {
            pm->players[id].texture = pm->red_texture;
        }
    }

    // Apply the received state directly.
    pm->players[id].position = data->position;
    pm->players[id].sprite_portion = data->sprite_portion;
    pm->players[id].flip_mode = data->flip_mode;
    // Infer movement state from the received sprite row for animation purposes.
    pm->players[id].is_moving = (fabsf(data->sprite_portion.y - PLAYER_SPRITE_WALK_ROW_Y) < 0.1f);

    pm->players[id].rect = (SDL_FRect){
        pm->players[id].position.x - PLAYER_WIDTH / 2.0f,
        pm->players[id].position.y - PLAYER_HEIGHT / 2.0f,
        PLAYER_WIDTH,
        PLAYER_HEIGHT};
}

void PlayerManager_RemovePlayer(PlayerManager pm, uint8_t client_id)
{
    if (!pm || client_id >= MAX_CLIENTS)
    {
        return;
    }
    if (pm->players[client_id].active)
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Deactivating player %u", (unsigned int)client_id);
        pm->players[client_id].active = false;
        memset(&pm->players[client_id], 0, sizeof(PlayerInstance)); // Clear data for the inactive slot.
        // If the local player is somehow removed, update the local ID tracker.
        if (client_id == pm->local_player_client_id)
        {
            pm->local_player_client_id = -1;
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Local player %u removed.", (unsigned int)client_id);
        }
    }
}

bool PlayerManager_GetLocalPlayerPosition(PlayerManager pm, SDL_FPoint *out_pos)
{
    if (pm && out_pos && pm->local_player_client_id >= 0 && pm->players[pm->local_player_client_id].active)
    {
        *out_pos = pm->players[pm->local_player_client_id].position;
        return true;
    }
    return false;
}

bool PlayerManager_GetPlayerPosition(PlayerManager pm, uint8_t client_id, SDL_FPoint *out_pos)
{
    if (!pm || !out_pos || client_id >= MAX_CLIENTS)
    {
        return false;
    }
    // Only return position if the requested player is currently active.
    if (pm->players[client_id].active)
    {
        *out_pos = pm->players[client_id].position;
        return true;
    }
    return false;
}

bool PlayerManager_GetLocalPlayerState(PlayerManager pm, Msg_PlayerStateData *out_data)
{
    if (!pm || !out_data || pm->local_player_client_id < 0 || !pm->players[pm->local_player_client_id].active)
    {
        return false;
    }

    PlayerInstance *p = &pm->players[pm->local_player_client_id];

    // Populate the network message struct with current local player state.
    out_data->message_type = MSG_TYPE_C_PLAYER_STATE; // Set message type for server identification.
    out_data->client_id = (uint8_t)pm->local_player_client_id;
    out_data->position = p->position;
    out_data->sprite_portion = p->sprite_portion;
    out_data->flip_mode = p->flip_mode;
    out_data->team = p->team;
    out_data->current_health = p->current_health;

    return true;
}

void damagePlayer(AppState state, int playerIndex, float damageValue, bool sendToServer)
{
    if (state.player_manager->players[playerIndex].current_health > 0)
    {
        state.player_manager->players[playerIndex].current_health -= damageValue;
        state.player_manager->players[playerIndex].playHurtAnim = true;
        SDL_Log("Player %d health %d", playerIndex, state.player_manager->players[playerIndex].current_health);
    }

    if (sendToServer)
    {
        NetClient_SendDamagePlayerRequest(state.net_client_state, playerIndex, damageValue);
    }

    if (state.player_manager->players[playerIndex].current_health <= 0 && !state.player_manager->players[playerIndex].dead)
    {
        state.player_manager->players[playerIndex].dead = true;
        state.player_manager->players[playerIndex].playDeathAnim = true;
        state.player_manager->players[playerIndex].deathTime = SDL_GetTicks();
        SDL_Log("Player %d Destroyed", playerIndex);
    }
}