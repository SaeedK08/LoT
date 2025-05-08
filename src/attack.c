#include "../include/attack.h"

// --- Internal Structures ---

/**
 * @brief Type-specific data for a fireball attack.
 */
typedef struct FireballData_s
{
    int _placeholder; // To avoid empty struct issues if nothing is needed yet
} FireballData;

/**
 * @brief Represents a single active attack instance in the game world.
 */
typedef struct AttackInstance
{
    // --- Common Data ---
    bool active;         /**< Whether this attack slot is currently in use and updated/rendered. */
    uint32_t id;         /**< Unique identifier assigned by the server. */
    AttackType type;     /**< The type of attack. */
    uint8_t owner_id;    /**< The client ID of the player who launched the attack. */
    SDL_FPoint position; /**< Current world position (center). */
    SDL_FPoint target;
    SDL_FPoint velocity;  /**< Current velocity vector (pixels per second). */
    float angle_deg;      /**< Current rendering angle in degrees. */
    SDL_Texture *texture; /**< Texture used for rendering this attack. */
    float render_width;   /**< Width used for rendering. */
    float render_height;  /**< Height used for rendering. */
    float hit_range;      /**< Radius or bounding box size used for collision detection. */
    ObjectType attacker;

    // --- Type-Specific Data ---
    union
    {
        FireballData fireball;
    } specific_data;

} AttackInstance;

/**
 * @brief Internal state for the AttackManager module ADT.
 */
struct AttackManager_s
{
    AttackInstance attacks[MAX_ATTACKS]; /**< Pool of attack instances. */
    int active_attack_count;             /**< Number of currently active attacks in the pool. */
    SDL_Texture *fireball_texture;       /**< Shared texture for fireball attacks. */
    uint32_t next_attack_id;             /**< Counter for assigning unique attack IDs. */
};

// --- Static Helper Functions ---

/**
 * @brief Finds the index of an active attack instance by its unique ID.
 * Linearly searches the active portion of the attacks array.
 * @param am The AttackManager instance.
 * @param attack_id The unique ID of the attack to find.
 * @return The index of the attack if found, otherwise -1.
 */
static int find_attack_by_id(AttackManager am, uint32_t attack_id)
{
    if (!am)
        return -1;
    for (int i = 0; i < am->active_attack_count; ++i)
    {
        if (am->attacks[i].id == attack_id)
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Finds the first inactive slot in the attacks array using the compacting approach.
 * If the active count is less than the max capacity, the next slot is available.
 * @param am The AttackManager instance.
 * @return The index of the next available slot, or -1 if the pool is full.
 */
static int find_inactive_attack_slot(AttackManager am)
{
    if (!am)
        return -1;
    if (am->active_attack_count < MAX_ATTACKS)
    {
        return am->active_attack_count;
    }
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Attack pool is full (%d/%d)", am->active_attack_count, MAX_ATTACKS);
    return -1;
}

/**
 * @brief Updates the state of a single active attack instance for one frame.
 * Handles movement, boundary checks, and potential collision/lifetime logic.
 * @param attack Pointer to the AttackInstance to update.
 * @param state Pointer to the main AppState.
 */
static void update_single_attack(AttackInstance *attack, AppState *state)
{
    if (!attack || !attack->active || !state)
        return;

    // --- Movement ---
    attack->position.x += attack->velocity.x * state->delta_time;
    attack->position.y += attack->velocity.y * state->delta_time;

    // --- Boundary Check / Lifetime ---
    if (state->map_state)
    {
        float dist_x = attack->position.x - attack->target.x;
        float dist_y = attack->position.y - attack->target.y;

        float distance_to_target = sqrtf(dist_x * dist_x + dist_y * dist_y);

        if (distance_to_target < FIREBALL_HIT_RANGE)
        {
            if (attack->attacker == OBJECT_TYPE_PLAYER)
            {
                if (attack->owner_id == NetClient_GetClientID(state->net_client_state))
                {
                    for (int i = 0; i < MAX_TOTAL_TOWERS; i++)
                    {
                        TowerInstance tempTower = state->tower_manager->towers[i];
                        if (tempTower.team != state->team)
                        {
                            if (SDL_PointInRectFloat(&attack->position, &tempTower.rect))
                            {
                                SDL_Log("Fireball Hit Tower %d", i);
                                damageTower(*state, i, FIREBALL_DAMAGE_VALUE, true);
                            }
                        }
                    }

                    for (int i = 0; i < MAX_BASES; i++)
                    {
                        BaseInstance tempBase = state->base_manager->bases[i];
                        if (tempBase.team != state->team)
                        {
                            if (SDL_PointInRectFloat(&attack->position, &tempBase.rect))
                            {
                                SDL_Log("Fireball Hit Base %d", i);
                                damageBase(*state, i, FIREBALL_DAMAGE_VALUE, true);
                            }
                        }
                    }
                }
            }
            else if (attack->attacker == OBJECT_TYPE_TOWER)
            {
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    SDL_FRect tempPlayerRect;
                    if (PlayerManager_GetPlayerRect(state->player_manager, i, &tempPlayerRect))
                    {
                        if (SDL_PointInRectFloat(&attack->position, &tempPlayerRect))
                        {
                            SDL_Log("Fireball Hit Player %d", i);
                            damagePlayer(*state, i, FIREBALL_DAMAGE_VALUE, true);
                        }
                    }
                }
            }

            attack->active = false;
            return;
        }
    }

    // --- Collision Detection ---

    // --- Type-Specific Update Logic ---
    switch (attack->type)
    {
    case ATTACK_TYPE_FIREBALL:
        break;
    }
}

/**
 * @brief Renders a single active attack instance to the screen.
 * @param attack Pointer to the AttackInstance to render.
 * @param state Pointer to the main AppState.
 */
static void render_single_attack(const AttackInstance *attack, AppState *state)
{
    if (!attack || !attack->active || !attack->texture || !state || !state->renderer || !state->camera_state)
    {
        return;
    }

    CameraState camera = state->camera_state;
    float cam_x = Camera_GetX(camera);
    float cam_y = Camera_GetY(camera);

    SDL_FRect dst_rect = {
        .x = attack->position.x - cam_x - attack->render_width / 2.0f,
        .y = attack->position.y - cam_y - attack->render_height / 2.0f,
        .w = attack->render_width,
        .h = attack->render_height};

    SDL_RenderTextureRotated(state->renderer,
                             attack->texture,
                             NULL,
                             &dst_rect,
                             attack->angle_deg,
                             NULL,
                             SDL_FLIP_NONE);
}

// --- Static Callback Functions (for EntityManager) ---

/**
 * @brief Internal function to update all active attacks and remove inactive ones.
 * Iterates backwards to allow safe removal using the compacting array method.
 * @param am The AttackManager instance.
 * @param state The main application state.
 */
static void Internal_AttackManagerUpdate(AttackManager am, AppState *state)
{
    if (!am || !state)
        return;
    // Iterate backwards to allow safe removal of elements during iteration
    // without skipping the next element after a removal.
    for (int i = am->active_attack_count - 1; i >= 0; --i)
    {
        if (am->attacks[i].active)
        {
            update_single_attack(&am->attacks[i], state);
        }

        // If attack became inactive, remove it by swapping with the last active element.
        if (!am->attacks[i].active)
        {
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Removing inactive attack at index %d (ID: %u). New count: %d", i, am->attacks[i].id, am->active_attack_count - 1);
            // This compaction keeps all active elements contiguous at the start of the array,
            // allowing simpler iteration for rendering and finding slots.
            if (i < am->active_attack_count - 1)
            {
                am->attacks[i] = am->attacks[am->active_attack_count - 1];
            }
            memset(&am->attacks[am->active_attack_count - 1], 0, sizeof(AttackInstance));
            am->active_attack_count--;
        }
    }
}

/**
 * @brief Internal function to render all active attacks.
 * @param am The AttackManager instance.
 * @param state The main application state.
 */
static void Internal_AttackManagerRender(AttackManager am, AppState *state)
{
    if (!am || !state)
        return;
    // Only need to iterate up to the active count due to compaction in update.
    for (int i = 0; i < am->active_attack_count; ++i)
    {
        render_single_attack(&am->attacks[i], state);
    }
}

/**
 * @brief Internal function to clean up AttackManager resources.
 * @param am The AttackManager instance.
 */
static void Internal_AttackManagerCleanup(AttackManager am)
{
    if (!am)
        return;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Cleaning up AttackManager resources...");
    if (am->fireball_texture)
    {
        SDL_DestroyTexture(am->fireball_texture);
        am->fireball_texture = NULL;
    }
}

/**
 * @brief Wrapper function conforming to EntityFunctions.update signature.
 * @param manager The EntityManager instance (unused).
 * @param state Pointer to the main AppState.
 */
static void attack_manager_update_callback(EntityManager manager, AppState *state)
{
    (void)manager;
    Internal_AttackManagerUpdate(state->attack_manager, state);
}

/**
 * @brief Wrapper function conforming to EntityFunctions.render signature.
 * @param manager The EntityManager instance (unused).
 * @param state Pointer to the main AppState.
 */
static void attack_manager_render_callback(EntityManager manager, AppState *state)
{
    (void)manager;
    Internal_AttackManagerRender(state->attack_manager, state);
}

/**
 * @brief Wrapper function conforming to EntityFunctions.cleanup signature.
 * @param manager The EntityManager instance (unused).
 * @param state Pointer to the main AppState.
 */
static void attack_manager_cleanup_callback(EntityManager manager, AppState *state)
{
    (void)manager;
    Internal_AttackManagerCleanup(state ? state->attack_manager : NULL);
    if (state)
    {
        state->attack_manager = NULL;
    }
}

// --- Public API Function Implementations ---

/**
 * @brief Initializes the AttackManager module and registers its entity functions.
 * Loads resources for known attack types.
 * @param state Pointer to the main AppState.
 * @return A new AttackManager instance on success, NULL on failure (use SDL_GetError()).
 * @sa AttackManager_Destroy
 */
AttackManager AttackManager_Init(AppState *state)
{
    if (!state || !state->renderer || !state->entity_manager)
    {
        SDL_SetError("Invalid AppState or missing renderer/entity_manager for AttackManager_Init");
        return NULL;
    }

    AttackManager am = (AttackManager)SDL_calloc(1, sizeof(struct AttackManager_s));
    if (!am)
    {
        SDL_OutOfMemory();
        return NULL;
    }
    am->active_attack_count = 0;
    am->next_attack_id = 1;

    // --- Load Resources ---
    const char fireball_path[] = "./resources/Sprites/Red_Team/Fire_Wizard/Charge.png";
    am->fireball_texture = IMG_LoadTexture(state->renderer, fireball_path);
    if (!am->fireball_texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Attack Init] Failed load texture '%s': %s", fireball_path, SDL_GetError());
        SDL_free(am);
        return NULL;
    }
    SDL_SetTextureScaleMode(am->fireball_texture, SDL_SCALEMODE_NEAREST);

    // --- Register with EntityManager ---
    EntityFunctions attack_funcs = {
        .name = "attack_manager",
        .update = attack_manager_update_callback,
        .render = attack_manager_render_callback,
        .cleanup = attack_manager_cleanup_callback,
        .handle_events = NULL};

    if (!EntityManager_Add(state->entity_manager, &attack_funcs))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Attack Init] Failed to add entity to manager: %s", SDL_GetError());
        Internal_AttackManagerCleanup(am);
        SDL_free(am);
        return NULL;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "AttackManager initialized and entity registered.");
    return am;
}

/**
 * @brief Destroys the AttackManager instance and associated resources.
 * Cleanup is primarily handled via the EntityManager callback. This frees the manager state itself.
 * @param am The AttackManager instance to destroy.
 * @sa AttackManager_Init
 */
void AttackManager_Destroy(AttackManager am)
{
    if (am)
    {
        am->fireball_texture = NULL;
        SDL_free(am);
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "AttackManager state container destroyed.");
    }
}

/**
 * @brief Handles a spawn message received from the server for a new attack instance.
 * Finds an available slot and initializes the attack based on the received data.
 * Called by the NetClient when receiving MSG_TYPE_S_SPAWN_ATTACK.
 * @param am The AttackManager instance.
 * @param data Pointer to the received Msg_ServerSpawnAttackData containing spawn details.
 */
void AttackManager_HandleServerSpawn(AttackManager am, const Msg_ServerSpawnAttackData *data)
{
    if (!am || !data)
        return;
    int slot = find_inactive_attack_slot(am);
    if (slot == -1)
        return;

    AttackInstance *attack = &am->attacks[slot];
    // Ensure the slot is clean before populating, preventing leftover data from previous use.
    memset(attack, 0, sizeof(AttackInstance));

    // --- Populate Common Data ---
    attack->active = true;
    attack->id = data->attack_id;
    attack->type = (AttackType)data->attack_type;
    attack->owner_id = data->owner_id;
    attack->position = data->start_pos;
    attack->target = data->target_pos;
    attack->velocity = data->velocity;
    attack->attacker = data->attacker;

    // --- Populate Type-Specific Data ---
    switch (attack->type)
    {
    case ATTACK_TYPE_FIREBALL:
        attack->texture = am->fireball_texture;
        if (!attack->texture)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Fireball texture missing for attack %u", attack->id);
            attack->active = false;
            return;
        }
        attack->render_width = FIREBALL_RENDER_WIDTH;
        attack->render_height = FIREBALL_RENDER_HEIGHT;
        attack->hit_range = FIREBALL_HIT_RANGE;
        attack->angle_deg = atan2f(attack->velocity.y, attack->velocity.x) * (180.0f / (float)M_PI);
        break;

    default:
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Attempted to spawn unknown attack type via network: %u", (unsigned int)attack->type);
        attack->active = false;
        return;
    }

    am->active_attack_count++;
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Spawned attack ID %u (type %u) at index %d. Active count: %d",
                 attack->id, (unsigned int)attack->type, slot, am->active_attack_count);
}

/**
 * @brief Handles a request from a client (forwarded by NetServer) to spawn an attack.
 * Performs validation, calculates start position and velocity, assigns an ID,
 * and requests the NetServer to broadcast the spawn message.
 * @param am The AttackManager instance.
 * @param state The main AppState.
 * @param owner_id The ID of the client requesting the attack.
 * @param type The type of attack requested (from AttackType enum).
 * @param target_pos The world position the attack is aimed at.
 */
void AttackManager_HandleClientSpawnRequest(AttackManager am, AppState *state, uint8_t owner_id, AttackType type, SDL_FPoint target_pos)
{
    if (!am || !state || !state->player_manager || !state->net_server_state)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Attack Handle Req] Missing manager state (Attack, Player, or NetServer)");
        return;
    }

    // --- 1. Validation ---
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[Attack Handle Req] Validating request from client %u...", owner_id);

    // --- 2. Get Start Position ---
    SDL_FPoint start_pos;
    if (!PlayerManager_GetPlayerPosition(state->player_manager, owner_id, &start_pos))
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Attack Handle Req] Could not get position for owner client %u", (unsigned int)owner_id);
        return;
    }
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[Attack Handle Req] Start position for client %u: (%.1f, %.1f)", owner_id, start_pos.x, start_pos.y);

    // --- 3. Calculate Velocity ---
    float dx = target_pos.x - start_pos.x;
    float dy = target_pos.y - start_pos.y;
    float len = sqrtf(dx * dx + dy * dy);
    SDL_FPoint velocity = {0.0f, 0.0f};
    float speed = 0.0f;

    switch (type)
    {
    case ATTACK_TYPE_FIREBALL:
        speed = FIREBALL_SPEED;
        break;
    default:
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Attack Handle Req] Unknown attack type %u for velocity calculation", (unsigned int)type);
        return;
    }

    // Avoid division by zero or near-zero if start and target positions are the same.
    if (len > 0.01f)
    {
        velocity.x = (dx / len) * speed;
        velocity.y = (dy / len) * speed;
    }
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[Attack Handle Req] Calculated velocity: (%.1f, %.1f)", velocity.x, velocity.y);

    // --- 4. Generate Attack ID ---
    uint32_t new_attack_id = am->next_attack_id++;
    if (am->next_attack_id == 0)
    {
        am->next_attack_id = 1; // Prevent ID 0, though unlikely with uint32_t.
    }
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[Attack Handle Req] Assigned attack ID: %u", new_attack_id);

    // --- 5. Prepare Spawn Message ---
    Msg_ServerSpawnAttackData spawn_msg;
    spawn_msg.message_type = MSG_TYPE_S_SPAWN_ATTACK;
    spawn_msg.attack_type = (uint8_t)type;
    spawn_msg.attack_id = new_attack_id;
    spawn_msg.owner_id = owner_id;
    spawn_msg.start_pos = start_pos;
    spawn_msg.target_pos = target_pos;
    spawn_msg.velocity = velocity;
    spawn_msg.attacker = OBJECT_TYPE_PLAYER;

    // --- 6. Broadcast via NetServer ---
    NetServer_BroadcastMessage(state->net_server_state, &spawn_msg, sizeof(Msg_ServerSpawnAttackData), -1);

    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[Attack Handle Req] Client %u requested spawn type %u, broadcasting attack ID %u", (unsigned int)owner_id, (unsigned int)type, new_attack_id);

    // --- 7. Spawn Locally on Server Immediately ---
    // If running as server, spawn the attack in the server's own simulation
    // immediately, otherwise clients won't see attacks spawned by the server player.
    if (state->is_server)
    {
        AttackManager_HandleServerSpawn(am, &spawn_msg);
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[Attack Handle Req] Spawning attack locally on server.");
    }
}

/**
 * @brief Handles a request to spawn an attack originating from a specific tower.
 * Performs validation, calculates start position and velocity, assigns an ID,
 * requests the NetServer to broadcast the spawn message, and spawns locally on the server.
 * @param am The AttackManager instance.
 * @param state The main AppState (must be the server's state).
 * @param type The type of attack requested (from AttackType enum).
 * @param target_pos The world position the attack is aimed at (e.g., enemy player position).
 * @param towerIndex The index of the tower initiating the attack.
 */
void AttackManager_ServerSpawnTowerAttack(AttackManager am, AppState *state, AttackType type, SDL_FPoint target_pos, int towerIndex)
{
    if (!state->is_server)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Attack Spawn Tower] Attempted to call server-only function from client context.");
        return;
    }
    if (!am || !state || !state->tower_manager || !state->net_server_state)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Attack Spawn Tower] Missing required manager state (Attack, Tower, or NetServer)");
        return;
    }
    if (towerIndex < 0 || towerIndex >= MAX_TOTAL_TOWERS)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Attack Spawn Tower] Invalid towerIndex: %d", towerIndex);
        return;
    }

    // --- 1. Get Tower ---
    TowerInstance *firingTower = &state->tower_manager->towers[towerIndex];
    if (!firingTower->is_active)
    {
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[Attack Spawn Tower] Tower %d is not active, cannot fire.", towerIndex);
        return; // Don't fire from inactive/destroyed towers
    }

    // --- 2. Get Start Position ---
    SDL_FPoint start_pos = firingTower->position;

    // --- 3. Calculate Velocity ---
    float dx = target_pos.x - start_pos.x;
    float dy = target_pos.y - start_pos.y;
    float len = sqrtf(dx * dx + dy * dy);
    SDL_FPoint velocity = {0.0f, 0.0f};
    float speed = 0.0f;

    switch (type)
    {
    case ATTACK_TYPE_FIREBALL:
        speed = FIREBALL_SPEED;
        break;
    default:
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Attack Spawn Tower] Unknown attack type %u for velocity calculation", (unsigned int)type);
        return;
    }

    // Avoid division by zero or near-zero if start and target positions are the same.
    if (len > 0.01f)
    {
        velocity.x = (dx / len) * speed;
        velocity.y = (dy / len) * speed;
    }
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[Attack Spawn Tower] Tower %d firing type %u. Velocity: (%.1f, %.1f)", towerIndex, (unsigned int)type, velocity.x, velocity.y);

    // --- 4. Generate Attack ID ---
    uint32_t new_attack_id = am->next_attack_id++;
    if (am->next_attack_id == 0)
    {
        am->next_attack_id = 1; // Prevent ID 0
    }
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[Attack Spawn Tower] Assigned attack ID: %u", new_attack_id);

    // --- 5. Prepare Spawn Message ---
    Msg_ServerSpawnAttackData spawn_msg;
    spawn_msg.message_type = MSG_TYPE_S_SPAWN_ATTACK;
    spawn_msg.attack_type = (uint8_t)type;
    spawn_msg.attack_id = new_attack_id;
    spawn_msg.owner_id = (uint8_t)towerIndex;
    spawn_msg.start_pos = start_pos;
    spawn_msg.target_pos = target_pos;
    spawn_msg.velocity = velocity;
    spawn_msg.attacker = OBJECT_TYPE_TOWER;

    // --- 6. Broadcast via NetServer ---
    NetServer_BroadcastMessage(state->net_server_state, &spawn_msg, sizeof(Msg_ServerSpawnAttackData), -1);

    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[Attack Spawn Tower] Broadcasting attack ID %u from tower %d", new_attack_id, towerIndex);

    // --- 7. Spawn Locally on Server Immediately ---
    AttackManager_HandleServerSpawn(am, &spawn_msg);
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[Attack Spawn Tower] Spawning attack ID %u locally on server.", new_attack_id);
}

/**
 * @brief Handles a destroy message received from the server for an existing attack instance.
 * Marks the specified attack as inactive for removal during the next update cycle.
 * Called by the NetClient when receiving MSG_TYPE_S_DESTROY_OBJECT.
 * @param am The AttackManager instance.
 * @param data Pointer to the received Msg_DestroyObjectData containing the object type and ID.
 */
void AttackManager_HandleDestroyObject(AttackManager am, const Msg_DestroyObjectData *data)
{
    if (!am || !data || data->object_type != OBJECT_TYPE_ATTACK)
        return;
    int index = find_attack_by_id(am, data->object_id);
    if (index != -1)
    {
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Marking attack ID %u at index %d for removal.", data->object_id, index);
        am->attacks[index].active = false; // Mark for removal by the update loop.
    }
    else
    {
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Received destroy request for already removed/unknown attack ID %u", data->object_id);
    }
}
