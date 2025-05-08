#pragma once

// --- Includes ---
#include "../include/common.h"
#include "../include/attack.h"
#include "../include/entity.h"
#include "../include/tower.h"

// --- Opaque Pointer Type ---
/**
 * @brief Opaque handle to the NetServerState.
 * Manages the server-side network state, including listening for connections
 * and handling communication with connected clients.
 */
typedef struct NetServerState_s *NetServerState;

// --- Public API Function Declarations ---

/**
 * @brief Initializes the Network Server module and registers its entity functions.
 * Creates the server state and starts listening for client connections.
 * Only call this if running in server mode.
 * @param state Pointer to the main AppState.
 * @return A new NetServerState instance on success, NULL on failure.
 * @sa NetServer_Destroy
 */
NetServerState NetServer_Init(AppState *state);

/**
 * @brief Destroys the NetServerState instance, closes the listening socket,
 * and disconnects all clients.
 * Cleanup is primarily handled via the EntityManager callback. This frees the manager state itself.
 * @param ns_state The NetServerState instance to destroy.
 * @sa NetServer_Init
 */
void NetServer_Destroy(NetServerState ns_state);

/**
 * @brief Broadcasts a message buffer to connected clients.
 * Allows other modules (like AttackManager) to request broadcasts. Sends only to
 * clients in the WELCOMED state.
 * @param ns_state The NetServerState instance.
 * @param buffer Pointer to the data buffer.
 * @param length Number of bytes to send.
 * @param exclude_client_index Index of a client to skip sending to (-1 to broadcast to all).
 */
void NetServer_BroadcastMessage(NetServerState ns_state, const void *buffer, int length, int exclude_client_index);
