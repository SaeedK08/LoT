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

#define MINION_SPEED 150.0f
#define MINON_DAMAGE_VALUE 0.1f
// #define TARGETS 3

#define MINION_SPRITE_FRAME_WIDTH 107.0f
#define MINION_SPRITE_FRAME_HEIGHT 110.0f
#define MINION_SPRITE_MOVE (MINION_SPRITE_FRAME_HEIGHT)
#define MINION_SPRITE_ATTACK (MINION_SPRITE_FRAME_HEIGHT * 2)
#define MINION_SPRITE_NUM_FRAMES 6
#define MINION_SPRITE_TIME_PER_FRAME 0.1f /**< Duration each animation frame is displayed. */

typedef struct MinionData MinionData; 
typedef struct MinionManager_s *MinionManager; 

MinionManager MinionManager_Init(AppState *state);
void MinionManager_UpdateRemoteMinion(MinionManager mm, const Msg_MinionStateData *data);
void MinionManager_Destroy(MinionManager mm);
static void minion_manager_update_callback(EntityManager manager, AppState *state);
static void minion_manager_render_callback(EntityManager manager, AppState *state);
static void minion_manager_cleanup_callback(EntityManager manager, AppState *state);
bool Minion_Init(MinionManager mm, uint8_t client_id, TowerManagerState tm, BaseManagerState bm);
bool MinionManager_GetLocalMinionState(MinionManager mm, Msg_MinionStateData *out_data);
static void update_local_minion_movment(MinionData *m, AppState *state);
static void update_local_minion_animation(MinionData *m, float delta_time);
static void render_single_minion(MinionData *m, AppState *state);
