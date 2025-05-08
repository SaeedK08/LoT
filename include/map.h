#pragma once

// --- Standard Library Includes ---
#include <stdbool.h>

// --- SDL/External Library Includes ---
#include <SDL3/SDL_render.h>

// --- Project Includes ---
#include "../include/common.h"
#include "../include/camera.h"
#include "../include/entity.h"

// --- Constants ---
#define MAP_TILE_WIDTH 16  /**< Width of a single tile in pixels. */
#define MAP_TILE_HEIGHT 16 /**< Height of a single tile in pixels. */

// --- Opaque Pointer Type ---
/**
 * @brief Opaque handle to the MapState.
 * Manages loading and rendering of the tilemap.
 */
typedef struct MapState_s *MapState;

// --- Public API Function Declarations ---

/**
 * @brief Initializes the Map module and registers its entity functions.
 * Loads the map data from the specified file and prepares textures.
 * @param state Pointer to the main AppState.
 * @return A new MapState instance on success, NULL on failure.
 * @sa Map_Destroy
 */
MapState Map_Init(AppState *state);

/**
 * @brief Destroys the MapState instance and associated resources.
 * Cleanup is primarily handled via the EntityManager callback. This frees the manager state itself.
 * @param ms The MapState instance to destroy.
 * @sa Map_Init
 */
void Map_Destroy(MapState ms);

/**
 * @brief Gets the total width of the map in pixels.
 * @param ms The MapState instance.
 * @return The map width in pixels, or 0 if map state is invalid.
 */
int Map_GetWidthPixels(MapState ms);

/**
 * @brief Gets the total height of the map in pixels.
 * @param ms The MapState instance.
 * @return The map height in pixels, or 0 if map state is invalid.
 */
int Map_GetHeightPixels(MapState ms);
