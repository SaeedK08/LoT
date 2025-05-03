#pragma once

// --- Standard C Library Includes ---
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// --- External Library Includes ---
#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include <SDL3_image/SDL_image.h>

// --- Constants ---
#define BLUE_TEAM 1
#define RED_TEAM 0

#define MAX_NAME_LENGTH 64
#define MAX_ENTITIES 100
#define MAX_CLIENTS 10
#define BUFFER_SIZE 512
#define SERVER_PORT 8080
#define HOSTNAME "localhost"

#define MAP_WIDTH 3200
#define MAP_HEIGHT 1760
#define CLIFF_BOUNDARY 304.0f
#define WATER_BOUNDARY 1432.0f

#define CAMERA_VIEW_WIDTH 600.0f
#define CAMERA_VIEW_HEIGHT 300.0f

#define PLAYER_WIDTH 32
#define PLAYER_HEIGHT 64
#define ATTACK_RANGE 100.0f
#define FRAME_WIDTH 64.0f
#define FRAME_HEIGHT 80.0f
#define IDLE_ROW_Y 0.0f
#define WALK_ROW_Y 80.0f
#define NUM_IDLE_FRAMES 6
#define NUM_WALK_FRAMES 6
#define TIME_PER_FRAME 0.1f
#define BLUE_WIZARD_PATH "./resources/Sprites/Blue_Team/Blue_Wizard/Blue_Wizard_Spritesheet.png"
#define RED_WIZARD_PATH "./resources/Sprites/Red_Team/Fire_Wizard/Fire_Wizard_Spiresheet.png"

#define MAX_BASES 2
#define BASE_WIDTH 296.0f
#define BASE_HEIGHT 205.0f

#define MAX_TOWERS 4
#define TOWER_WIDTH 114.0f
#define TOWER_HEIGHT 183.0f

// --- Global Variables ---
/**
 * @brief Count of currently active entities in the `entities` array.
 */
extern int entities_count;

// --- Enums ---

/**
 * @brief Defines the types of messages used in network communication.
 */
typedef enum
{
  MSG_TYPE_INVALID = 0,     /**< Invalid or uninitialized message type. */
  MSG_TYPE_C_HELLO = 1,     /**< Client initiates connection. */
  MSG_TYPE_S_WELCOME = 2,   /**< Server acknowledges connection, assigns ID. */
  MSG_TYPE_PLAYER_STATE = 3 /**< Update of a player's position and appearance. */
} MessageType;

// --- Structures ---

/**
 * @brief Data structure for sending player state information over the network.
 */
typedef struct
{
  uint8_t client_id;        /**< Unique identifier for the client/player. */
  SDL_FPoint position;      /**< Player's world coordinates. */
  SDL_FRect sprite_portion; /**< Source rectangle for the player's current sprite frame. */
  SDL_FlipMode flip_mode;   /**< Horizontal flip state for the player sprite. */
  bool team;
} PlayerStateData;

/**
 * @brief Holds the core application state needed by various systems.
 */
typedef struct AppState
{
  SDL_Window *window;     /**< The main application window. */
  SDL_Renderer *renderer; /**< The main application renderer. */
  float last_tick;        /**< Timestamp of the previous frame. */
  float current_tick;     /**< Timestamp of the current frame. */
  float delta_time;       /**< Time elapsed since the last frame in seconds. */
  bool is_server;         /**< Flag indicating if this instance is running as the server. */
} AppState;

/**
 * @brief Represents a game entity or system with optional callback functions.
 */
typedef struct Entity
{
  char name[MAX_NAME_LENGTH];                              /**< Unique name identifier for the entity/system. */
  void (*cleanup)(void);                                   /**< Function pointer for resource cleanup on shutdown. */
  void (*handle_events)(void *appstate, SDL_Event *event); /**< Function pointer for handling SDL events. */
  void (*update)(AppState *state);                         /**< Function pointer for game logic updates each frame. */
  void (*render)(AppState *state);                         /**< Function pointer for rendering each frame. */
} Entity;

/**
 * @brief Global array holding all registered entities/systems.
 */
extern Entity entities[MAX_ENTITIES];

// --- Entity System Function Declarations ---

/**
 * @brief Registers a new entity/system.
 * @param entity The Entity struct containing the name and function pointers.
 * @return SDL_APP_SUCCESS on success, SDL_APP_FAILURE if max entities reached or name conflict.
 */
SDL_AppResult create_entity(Entity entity);

/**
 * @brief Removes an entity/system from the registry by its index.
 * @param index The index of the entity to remove in the `entities` array.
 * @return SDL_APP_SUCCESS on success, SDL_APP_FAILURE if index is out of bounds.
 */
SDL_AppResult delete_entity(int index);

/**
 * @brief Swaps two entities in the registry array (used internally for removal).
 * @param index1 The index of the first entity.
 * @param index2 The index of the second entity.
 */
void swap_entities(int index1, int index2);

/**
 * @brief Finds the index of an entity/system by its name.
 * @param name The name of the entity to find.
 * @return The index of the entity if found, -1 otherwise.
 */
int find_entity(const char *name);

// --- Forward Declarations for Opaque Structs ---
// Allows pointers to these types without needing their full definition here.
typedef struct Player Player;
typedef struct Camera Camera;
typedef struct FireBall FireBall;

// --- System Initialization Function Declarations ---

/**
 * @brief Initializes the camera system.
 * @param renderer The main SDL renderer.
 * @return SDL_APP_SUCCESS or SDL_APP_FAILURE.
 * @sa init_camera in camera.h
 */
SDL_AppResult init_camera(SDL_Renderer *renderer);

/**
 * @brief Initializes the map system.
 * @param renderer The main SDL renderer.
 * @return SDL_APP_SUCCESS or SDL_APP_FAILURE.
 */
SDL_AppResult init_map(SDL_Renderer *renderer);

/**
 * @brief Initializes the base system.
 * @param renderer The main SDL renderer.
 * @return SDL_APP_SUCCESS or SDL_APP_FAILURE.
 * @sa init_base in base.h
 */
SDL_AppResult init_base(SDL_Renderer *renderer);

/**
 * @brief Initializes the tower system.
 * @param renderer The main SDL renderer.
 * @return SDL_APP_SUCCESS or SDL_APP_FAILURE.
 */
SDL_AppResult init_tower(SDL_Renderer *renderer);

/**
 * @brief Initializes the fireball attack system.
 * @param renderer The main SDL renderer.
 * @return SDL_APP_SUCCESS or SDL_APP_FAILURE.
 * @sa init_fireball in attack.h
 */
SDL_AppResult init_fireball(SDL_Renderer *renderer);

/**
 * @brief Activates a new fireball projectile.
 * @param player_pos_x Player's world X coordinate.
 * @param player_pos_y Player's world Y coordinate.
 * @param cam_x Camera's world X coordinate.
 * @param cam_y Camera's world Y coordinate.
 * @param mouse_view_x Mouse X coordinate relative to the viewport.
 * @param mouse_view_y Mouse Y coordinate relative to the viewport.
 * @sa activate_fireballs in attack.h
 */
void activate_fireballs(float player_pos_x, float player_pos_y, float cam_x, float cam_y, float mouse_view_x, float mouse_view_y);