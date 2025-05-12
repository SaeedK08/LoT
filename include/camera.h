#pragma once

// --- Includes ---
#include "../include/common.h"
#include "../include/player.h"
#include "../include/map.h"
#include "../include/entity.h"

// --- Constants ---
#define CAMERA_VIEW_WIDTH 600.0f
#define CAMERA_VIEW_HEIGHT 300.0f

// --- Opaque Pointer Type ---
/**
 * @brief Opaque handle to the CameraState.
 * Manages the game's camera position and view dimensions.
 */
typedef struct CameraState_s *CameraState;

// --- Public API Function Declarations ---

/**
 * @brief Initializes the Camera module and registers its entity functions.
 * Creates the camera state and sets initial view dimensions.
 * @param state Pointer to the main AppState.
 * @return A new CameraState instance on success, NULL on failure.
 * @sa Camera_Destroy
 */
CameraState Camera_Init(AppState *state);

/**
 * @brief Destroys the CameraState instance.
 * @param camera_state The CameraState instance to destroy.
 * @sa Camera_Init
 */
void Camera_Destroy(CameraState camera_state);

/**
 * @brief Updates the camera's position based on its target and map boundaries.
 * @param camera_state The CameraState instance to update.
 * @param state Pointer to the main AppState.
 */
void Camera_Render(CameraState camera_state, AppState *state);

/**
 * @brief Gets the current X coordinate of the camera's top-left corner.
 * @param camera_state The CameraState instance.
 * @return The camera's world X coordinate.
 */
float Camera_GetX(CameraState camera_state);

/**
 * @brief Gets the current Y coordinate of the camera's top-left corner.
 * @param camera_state The CameraState instance.
 * @return The camera's world Y coordinate.
 */
float Camera_GetY(CameraState camera_state);

/**
 * @brief Gets the current width of the camera's view.
 * @param camera_state The CameraState instance.
 * @return The camera's view width.
 */
float Camera_GetWidth(CameraState camera_state);

/**
 * @brief Gets the current height of the camera's view.
 * @param camera_state The CameraState instance.
 * @return The camera's view height.
 */
float Camera_GetHeight(CameraState camera_state);
