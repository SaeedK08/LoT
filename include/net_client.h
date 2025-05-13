#pragma once

// --- Includes ---
#include "../include/common.h"
#include "../include/player.h"
#include "../include/minion.h"
#include "../include/camera.h"
#include "../include/attack.h"
#include "../include/entity.h"
#include "../include/hud.h"

// --- Opaque Pointer Type ---
/**
 * @brief Opaque handle to the NetClientState.
 * Manages the client-side network connection and communication with the server.
 */
typedef struct NetClientState_s *NetClientState;

// --- Public API Function Declarations ---

/**
 * @brief Initializes the Network Client module and registers its entity functions.
 * Creates the client state but does not initiate connection yet.
 * @param state Pointer to the main AppState.
 * @return A new NetClientState instance on success, NULL on failure.
 * @sa NetClient_Destroy
 */
NetClientState NetClient_Init(AppState *state, const char *hostname);

/**
 * @brief Destroys the NetClientState instance and closes any active connection.
 * Cleanup is primarily handled via the EntityManager callback. This frees the manager state itself.
 * @param nc_state The NetClientState instance to destroy.
 * @sa NetClient_Init
 */
void NetClient_Destroy(NetClientState nc_state);

/**
 * @brief Gets the client ID assigned by the server.
 * @param nc_state The NetClientState instance.
 * @return The assigned client ID, or -1 if not connected or ID not received yet.
 */
int NetClient_GetClientID(NetClientState nc_state);

/**
 * @brief Checks if the client is currently connected to the server.
 * @param nc_state The NetClientState instance.
 * @return True if the client state is CONNECTED, false otherwise.
 */
bool NetClient_IsConnected(NetClientState nc_state);

/**
 * @brief Sends a request to the server to spawn an attack.
 * @param nc_state The NetClientState instance.
 * @param type The type of attack to spawn (from AttackType enum).
 * @param target_world_x The target world X coordinate.
 * @param target_world_y The target world Y coordinate.
 * @return True if the request was sent successfully, false otherwise (e.g., not connected).
 */
bool NetClient_SendSpawnAttackRequest(NetClientState nc_state, AttackType type, float target_world_x, float target_world_y, bool team);

bool NetClient_SendDamagePlayerRequest(NetClientState nc_state, int playerIndex, float damageValue);
/**
 * @brief Sends a request to the server to damage a tower.
 * @param nc_state The NetClientState instance.
 * @param towerIndex The index of the tower to apply damage to.
 * @param damageValue The amount of damage to apply to the tower.
 * @return True if the request was sent successfully, false otherwise (e.g., not connected).
 */
bool NetClient_SendDamageTowerRequest(NetClientState nc_state, int towerIndex, float damageValue, float current_health);

/**
 * @brief Sends a request to the server to damage a base.
 * @param nc_state The NetClientState instance.
 * @param baseIndex The index of the base to apply damage to.
 * @param damageValue The amount of damage to apply to the base.
 * @return True if the request was sent successfully, false otherwise (e.g., not connected).
 */
bool NetClient_SendDamageBaseRequest(NetClientState nc_state, int baseIndex, float damageValue);

/**
 * @brief Sends the match result to the server.
 * @param nc_state The NetClientState instance.
 * @param winningTeam The team that won the match.
 * @return True if the request was sent successfully, false otherwise (e.g., not connected).
 */
bool NetClient_SendMatchResult(NetClientState nc_state, bool winningTeam);
