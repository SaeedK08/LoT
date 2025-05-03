#pragma once

// --- Standard C Library Includes ---
#include <string.h>

// --- External Library Includes ---
#include <SDL3/SDL.h>

// --- Project Includes ---
#include "../include/common.h"

// --- Public Function Declarations ---

/**
 * @brief Registers a new entity/system in the global `entities` array.
 * @param entity The Entity struct containing the name and function pointers.
 * @return SDL_APP_SUCCESS on success, SDL_APP_FAILURE if max entities reached or name conflict.
 * @sa Entity in common.h
 */
SDL_AppResult create_entity(Entity entity);

/**
 * @brief Removes an entity/system from the registry by its index using a swap-and-pop method.
 * @param index The index of the entity to remove in the `entities` array.
 * @return SDL_APP_SUCCESS on success, SDL_APP_FAILURE if index is out of bounds.
 */
SDL_AppResult delete_entity(int index);

/**
 * @brief Swaps two entities within the global `entities` array. Primarily used internally by delete_entity.
 * @param index1 The index of the first entity.
 * @param index2 The index of the second entity.
 */
void swap_entities(int index1, int index2);

/**
 * @brief Finds the index of an entity/system within the global `entities` array by its name.
 * @param name The null-terminated string name of the entity to find.
 * @return The index of the entity if found (0 to entities_count - 1), or -1 if not found.
 */
int find_entity(const char *name);