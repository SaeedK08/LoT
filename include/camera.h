#pragma once

#include "../include/common.h"
#include "../include/player.h"

// --- Structures ---

/**
 * @brief Represents the game camera's state.
 */
typedef struct Camera
{
  float x; /**< World X coordinate of the camera's top-left corner. */
  float y; /**< World Y coordinate of the camera's top-left corner. */
  float w; /**< Width of the camera's view. */
  float h; /**< Height of the camera's view. */
} Camera;

// --- Global Variables ---

/**
 * @brief Global instance of the game camera.
 */
extern Camera camera;

// --- Public Function Declarations ---

/**
 * @brief Initializes the camera system and registers its entity.
 * @param renderer The main SDL renderer (currently unused in init but potentially useful later).
 * @return SDL_APP_SUCCESS on success, SDL_APP_FAILURE on error (e.g., entity creation failed).
 */
SDL_AppResult init_camera(SDL_Renderer *renderer);
