#pragma once

// --- SDL/External Library Includes ---
#include <SDL3/SDL.h>
#include <stdbool.h>

/**
 * @brief Enum defining different game states.
 */
typedef enum GameState
{
    GAME_STATE_LOBBY = 1,
    GAME_STATE_PLAYING = 2,
    GAME_STATE_FINISHED = 3,
} GameState;

// --- Forward Declarations for ADT Opaque Pointer Types ---
typedef struct CameraState_s *CameraState;
typedef struct MapState_s *MapState;
typedef struct PlayerManager_s *PlayerManager;
typedef struct MinionManager_s *MinionManager;
typedef struct AttackManager_s *AttackManager;
typedef struct NetClientState_s *NetClientState;
typedef struct NetServerState_s *NetServerState;
typedef struct EntityManager_s *EntityManager;
typedef struct BaseManagerState_s *BaseManagerState;
typedef struct TowerManagerState_s *TowerManagerState;
typedef struct HUDManager_s *HUDManager;

// --- Main Application State Structure ---

/**
 * @brief Holds the overall state of the application.
 * Contains SDL resources and pointers to the state managed by various modules (ADTs).
 */
typedef struct AppState
{
    SDL_Window *window;
    SDL_Renderer *renderer;

    // --- Timing ---
    Uint64 last_tick;
    Uint64 current_tick;
    float delta_time;

    // --- Core State ---
    bool is_server;
    bool quit_requested;
    bool team;
    GameState currentGameState;

    // --- Module State Pointers (ADTs) ---
    EntityManager entity_manager;
    MapState map_state;
    CameraState camera_state;
    PlayerManager player_manager;
    MinionManager minion_manager;
    AttackManager attack_manager;
    NetClientState net_client_state;
    NetServerState net_server_state; /**< NULL if not running as server. */
    BaseManagerState base_manager;
    TowerManagerState tower_manager;
    HUDManager HUD_manager;
} AppState;
