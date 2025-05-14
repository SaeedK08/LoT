#pragma once

#include "../include/entity.h"
#include "../include/common.h"
#include "../include/net_client.h"
#include "../include/base.h"
#include "../include/player.h"

#define BLUE_MINION_PATH "./resources/Sprites/Blue_Team/Warrior_Blue.png"
#define RED_MINION_PATH "./resources/Sprites/Red_Team/Warrior_Red.png"
#define MINION_WIDTH 16.0f
#define MINION_HEIGHT 32.0f
#define MINION_HEALTH_MAX 100.0f
#define MINION_WAVE_AMOUNT 6
#define MINION_MAX_AMOUNT 24
#define MINION_ATTACK_COOLDOWN 1000

#define MINION_SPEED 150.0f
#define MINION_DAMAGE_VALUE 1.0f
// #define TARGETS 3

#define MINION_SPRITE_FRAME_WIDTH 107.0f
#define MINION_SPRITE_FRAME_HEIGHT 110.0f
#define MINION_SPRITE_MOVE (MINION_SPRITE_FRAME_HEIGHT)
#define MINION_SPRITE_ATTACK (MINION_SPRITE_FRAME_HEIGHT * 2)
#define MINION_SPRITE_NUM_FRAMES 6
#define MINION_SPRITE_TIME_PER_FRAME 0.1f /**< Duration each animation frame is displayed. */

typedef struct MinionData MinionData;
typedef struct MinionManager_s *MinionManager;

struct MinionData
{
    bool team;
    SDL_FPoint position;      /**< Current world position (center). */
    SDL_FRect sprite_portion; /**< The source rect defining the current animation frame. */
    SDL_FlipMode flip_mode;   /**< Rendering flip state (horizontal). */
    bool active;              /**< Whether this minion slot is currently in use. */
    bool is_attacking;
    SDL_Texture *texture;
    int current_health; /**< Current health points. */
    float anim_timer;
    int current_frame;
    float attack_cooldown_timer;
};

struct MinionManager_s
{
    MinionData minions[MINION_MAX_AMOUNT];
    SDL_Texture *red_texture;
    SDL_Texture *blue_texture;
    Uint64 minionWaveTimer;
    Uint64 recentMinionTimer;
    int activeMinionAmount;
    int currentMinionWaveAmount;
    bool spawnNextMinion;
};

MinionManager MinionManager_Init(AppState *state);
void MinionManager_Destroy(MinionManager mm);
void damageMinion(AppState state, int minionIndex, float damageValue, bool senToServer, float sentCurrentHealth);
