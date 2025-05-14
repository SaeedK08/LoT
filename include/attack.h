#pragma once

// --- Standard Library Includes ---
#include <math.h>

// --- SDL/External Library Includes ---

// --- Project Includes ---
#include "../include/common.h"
#include "../include/entity.h"
#include "../include/net_server.h"
#include "../include/player.h"
#include "../include/minion.h"
#include "../include/map.h"
#include "../include/camera.h"
#include "../include/base.h"
#include "../include/tower.h"

// --- Constants ---
#define MAX_ATTACKS 100 /**< Maximum number of concurrent attacks allowed. */

#define PLAYER_ATTACK_SPRITE_FRAME_WIDTH 48
#define PLAYER_ATTACK_SPRITE_FRAME_HEIGHT 48
#define PLAYER_ATTACK_RENDER_WIDTH 33.0f
#define PLAYER_ATTACK_RENDER_HEIGHT 39.0f
#define ATTACK_SPEED 100.0f           /**< Speed in pixels/second for attacks. */
#define PLAYER_ATTACK_HIT_RANGE 10.0f /**< Collision radius for attacks. */

#define PLAYER_ATTACK_SPRITE_NUM_FRAMES 10
#define PLAYER_ATTACK_SPRITE_TIME_PER_FRAME 0.1f
#define PLAYER_ATTACK_SPRITE_ROW_Y 0.0f

#define PLAYER_ATTACK_DAMAGE_VALUE 25.0f
#define TOWER_ATTACK_DAMAGE_VALUE 50.0f

// --- Opaque Pointer Type ---

/**
 * @brief Opaque handle to the AttackManager state.
 * Manages all active attack instances in the game.
 */
typedef struct AttackManager_s *AttackManager;

// --- Public API Function Declarations ---

/**
 * @brief Initializes the AttackManager module and registers its entity functions.
 * Loads resources required for known attack types.
 * @param state Pointer to the main AppState.
 * @return A new AttackManager instance on success, NULL on failure.
 * @sa AttackManager_Destroy
 */
AttackManager AttackManager_Init(AppState *state);

/**
 * @brief Destroys the AttackManager instance and associated resources.
 * Cleanup is primarily handled via the EntityManager callback. This frees the manager state itself.
 * @param am The AttackManager instance to destroy.
 * @sa AttackManager_Init
 */
void AttackManager_Destroy(AttackManager am);

/**
 * @brief Handles a spawn message received from the server for a new attack instance.
 * Finds an available slot and initializes the attack based on the received data.
 * Called by the NetClient when receiving MSG_TYPE_S_SPAWN_ATTACK.
 * @param am The AttackManager instance.
 * @param data Pointer to the received Msg_ServerSpawnAttackData containing spawn details.
 */
void AttackManager_HandleServerSpawn(AttackManager am, const Msg_ServerSpawnAttackData *data);

/**
 * @brief Handles a request from a client (forwarded by NetServer) to spawn an attack.
 * Performs validation, calculates start position and velocity, assigns an ID,
 * and requests the NetServer to broadcast the spawn message. Also spawns locally if server.
 * @param am The AttackManager instance.
 * @param state The main AppState.
 * @param owner_id The ID of the client requesting the attack.
 * @param type The type of attack requested (from AttackType enum).
 * @param target_pos The world position the attack is aimed at.
 */
void AttackManager_HandleClientSpawnRequest(AttackManager am, AppState *state, uint8_t owner_id, Msg_ClientSpawnAttackData data);

/**
 * @brief Handles a destroy message received from the server for an existing attack instance.
 * Marks the specified attack as inactive for removal during the next update cycle.
 * Called by the NetClient when receiving MSG_TYPE_S_DESTROY_OBJECT if object_type is ATTACK.
 * @param am The AttackManager instance.
 * @param data Pointer to the received Msg_DestroyObjectData containing the object type and ID.
 */
void AttackManager_HandleDestroyObject(AttackManager am, const Msg_DestroyObjectData *data);

void AttackManager_ServerSpawnTowerAttack(AttackManager am, AppState *state, AttackType type, SDL_FPoint target_pos, int towerIndex);