#pragma once

#include "../include/common.h"

/**
 * @brief Initializes the server socket, prepares client slots, and creates the server entity.
 * @return SDL_APP_SUCCESS on successful initialization, SDL_APP_FAILURE otherwise.
 */
SDL_AppResult init_server(void);

/**
 * @brief Returns the current number of active clients connected to the server.
 * @return The count of clients marked as active.
 */
int get_active_client_count(void);

/**
 * @brief Sends a data buffer to all active clients, optionally excluding one.
 * @param buffer Pointer to the data buffer to send.
 * @param length The number of bytes to send from the buffer.
 * @param excludeClientIndex The index of a client to exclude from the broadcast, or -1 to send to all.
 * @return void
 */
void broadcast_buffer(const void *buffer, int length, int excludeClientIndex);