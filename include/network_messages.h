#pragma once

// --- Includes ---
#include <SDL3/SDL.h>
#include <stdint.h>

// --- Message Type Enum ---

/**
 * @brief Identifies the type of network message being sent or received.
 * The first byte of any network packet should correspond to one of these values.
 */
typedef enum MessageType
{
    MSG_TYPE_INVALID = 0, /**< Should not be used. */

    // --- Client-to-Server Messages ---
    MSG_TYPE_C_HELLO = 1,         /**< Client introduces itself upon connection. */
    MSG_TYPE_C_PLAYER_STATE = 2,  /**< Client sends its current state update. */
    MSG_TYPE_C_SPAWN_ATTACK = 3,  /**< Client requests to spawn an attack. */
    MSG_TYPE_C_DAMAGE_PLAYER = 4, /**< Client requests to damage a player. */
    MSG_TYPE_C_DAMAGE_TOWER = 5,  /**< Client requests to damage a tower. */
    MSG_TYPE_C_DAMAGE_BASE = 6,   /**< Client requests to damage a base. */
    MSG_TYPE_C_DAMAGE_MINION = 7,  /**< Client sends a request to damage a minion. */


    MSG_TYPE_C_MATCH_RESULT = 89, /**< Client sends the match result. */

    // --- Server-to-Client Messages ---
    MSG_TYPE_S_WELCOME = 101,       /**< Server acknowledges client, assigns ID. */
    MSG_TYPE_S_PLAYER_STATE = 102,  /**< Server broadcasts another player's state. */
    MSG_TYPE_S_SPAWN_ATTACK = 103,  /**< Server confirms/broadcasts an attack spawn. */
    MSG_TYPE_S_DAMAGE_PLAYER = 104, /**< Server confirms/broadcasts damage to a player. */
    MSG_TYPE_S_DAMAGE_TOWER = 105,  /**< Server confirms/broadcasts damage to a tower. */
    MSG_TYPE_S_DAMAGE_BASE = 106,   /**< Server confirms/broadcasts damage to a basea. */
    MSG_TYPE_S_DAMAGE_MINION = 107,  /**< Serever confirms/broadcast damage to minion. */
    

    MSG_TYPE_S_GAME_START = 188,
    MSG_TYPE_S_GAME_RESULT = 189,       /**< Server confirms/broadcasts the match result. */
    MSG_TYPE_S_DESTROY_OBJECT = 198,    /**< Server informs clients an object was destroyed. */
    MSG_TYPE_S_PLAYER_DISCONNECT = 199, /**< Server informs clients a player disconnected. */
} MessageType;

// --- Attack Type Enum ---

/**
 * @brief Enum defining different attack types.
 */
typedef enum AttackType
{
    PLAYER_ATTACK_TYPE_FIREBALL = 0,
    PLAYER_ATTACK_TYPE_LIGHTNING_ARROW = 1,
    TOWER_ATTACK_TYPE = 2,
} AttackType;

// --- Destroyable Object Type Enum ---

/**
 * @brief Enum defining different types of objects that can be destroyed over the network.
 */
typedef enum ObjectType
{
    OBJECT_TYPE_PLAYER = 0,
    OBJECT_TYPE_ATTACK = 1,
    OBJECT_TYPE_TOWER = 2,
    OBJECT_TYPE_BASE = 3,
} ObjectType;

// --- Message Data Structures ---

/**
 * @brief Data structure for MSG_TYPE_S_WELCOME.
 * Sent from server to a newly connected client.
 */
typedef struct Msg_WelcomeData
{
    uint8_t message_type;       /**< Should be MSG_TYPE_S_WELCOME. */
    uint8_t assigned_client_id; /**< The ID assigned to this client by the server. */
} Msg_WelcomeData;

/**
 * @brief Data structure for MSG_TYPE_C_PLAYER_STATE and MSG_TYPE_S_PLAYER_STATE.
 * Used for client updates to server and server broadcasts to clients.
 */
typedef struct Msg_PlayerStateData
{
    uint8_t message_type;     /**< MSG_TYPE_C_PLAYER_STATE or MSG_TYPE_S_PLAYER_STATE. */
    uint8_t client_id;        /**< The ID of the player this state belongs to. */
    SDL_FPoint position;      /**< Current world position (x, y). */
    SDL_FRect sprite_portion; /**< Current source rect for animation frame. */
    SDL_FlipMode flip_mode;   /**< Current horizontal flip state. */
    bool team;
    int current_health;
} Msg_PlayerStateData;


/**
 * @brief Data structure for MSG_TYPE_S_PLAYER_DISCONNECT.
 * Sent from server to clients when a player leaves.
 */
typedef struct Msg_PlayerDisconnectData
{
    uint8_t message_type; /**< Should be MSG_TYPE_S_PLAYER_DISCONNECT. */
    uint8_t client_id;    /**< The ID of the player who disconnected. */
} Msg_PlayerDisconnectData;

/**
 * @brief Data structure for MSG_TYPE_C_SPAWN_ATTACK.
 * Sent from client to server requesting an attack spawn.
 */
typedef struct Msg_ClientSpawnAttackData
{
    uint8_t message_type;  /**< Should be MSG_TYPE_C_SPAWN_ATTACK. */
    uint8_t attack_type;   /**< The type of attack (AttackType enum). */
    SDL_FPoint target_pos; /**< Target world position for the attack. */
    bool team;
} Msg_ClientSpawnAttackData;

/**
 * @brief Data structure for MSG_TYPE_S_SPAWN_ATTACK.
 * Sent from server to clients to confirm/broadcast an attack spawn.
 */
typedef struct Msg_ServerSpawnAttackData
{
    uint8_t message_type; /**< Should be MSG_TYPE_S_SPAWN_ATTACK. */
    uint8_t attack_type;  /**< The type of attack (AttackType enum). */
    uint32_t attack_id;   /**< Unique ID assigned by the server for this attack instance. */
    uint8_t owner_id;     /**< Client ID of the player who owns this attack. */
    SDL_FPoint start_pos; /**< Initial world position of the attack. */
    SDL_FPoint target_pos;
    SDL_FPoint velocity; /**< Initial velocity vector (dx, dy) per second. */
    ObjectType attacker;
    bool team;
} Msg_ServerSpawnAttackData;

/**
 * @brief Data structure for MSG_TYPE_S_DESTROY_OBJECT.
 * Sent from server to clients when a networked object is destroyed.
 */
typedef struct Msg_DestroyObjectData
{
    uint8_t message_type; /**< Should be MSG_TYPE_S_DESTROY_OBJECT. */
    uint8_t object_type;  /**< Type of the object being destroyed (ObjectType enum). */
    uint32_t object_id;   /**< Unique ID of the object. */
} Msg_DestroyObjectData;

/**
 * @brief Data structure for Msg_DamagePlayer.
 * Sent from when a player is damaged.
 */
typedef struct Msg_DamagePlayer
{
    uint8_t message_type; /**< Should be MSG_TYPE_S_DAMAGE_PLAYER. */
    int playerIndex;      /**< The index of the player that got damaged. */
    float damageValue;    /**< The amount of damage that the player got. */
} Msg_DamagePlayer;

/**
 * @brief Data structure for Msg_DamageMinion.
 * Sent from when a minion is damaged.
 */
typedef struct Msg_DamageMinion
{
    uint8_t message_type; /**< Should be MSG_TYPE_S_DAMAGE_MINION. */
    int minionIndex;       /**< The index of the minion that got damaged. */
    float current_health;  /**<  The current health of tower after receviving the message from server*/
} Msg_DamageMinion;

/**
 * @brief Data structure for Msg_DamageTower.
 * Sent from when a tower is damaged.
 */
typedef struct Msg_DamageTower
{
    uint8_t message_type; /**< Should be MSG_TYPE_S_DAMAGE_TOWER. */
    int towerIndex;       /**< The index of the tower that got damaged. */
    float damageValue;    /**< The amount of damage that the tower got. */
    float current_health;  /**<  The current health of tower after receviving the message from server*/
} Msg_DamageTower;

/**
 * @brief Data structure for Msg_DamageBase.
 * Sent when a base is damaged.
 */
typedef struct Msg_DamageBase
{
    uint8_t message_type; /**< Should be MSG_TYPE_S_DAMAGE_BASE. */
    int baseIndex;        /**< The index of the base that got damaged. */
    float damageValue;    /**< The amount of damage that the base got. */
} Msg_DamageBase;

/**
 * @brief Data structure for Msg_MatchResult.
 * Sent when a match result has been decided.
 */
typedef struct Msg_MatchResult
{
    uint8_t message_type; /**< Should be MSG_TYPE_S_. */
    bool winningTeam;
} Msg_MatchResult;