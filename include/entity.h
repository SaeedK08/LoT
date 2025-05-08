#pragma once

// --- Includes ---
#include "../include/common.h"

// --- Constants ---
#define MAX_MANAGED_ENTITIES 100

// --- Opaque Pointer Type ---
/**
 * @brief Opaque handle to the EntityManager state.
 * Manages the lifecycle (update, render, events, cleanup) of registered game entities/modules.
 */
typedef struct EntityManager_s *EntityManager;

// --- Entity Definition Structure ---

/**
 * @brief Structure holding the function pointers that define an entity's behavior.
 * Each module (e.g., PlayerManager, Map) registers an instance of this struct
 * with the EntityManager to participate in the main game loop callbacks.
 */
typedef struct EntityFunctions
{
    const char *name; /**< Unique name for identifying this entity type. */

    /**
     * @brief Optional function called when the EntityManager is destroyed.
     * Used by the entity/module to free its specific resources.
     * @param manager The EntityManager instance managing this entity.
     * @param state Pointer to the main AppState.
     */
    void (*cleanup)(EntityManager manager, AppState *state);

    /**
     * @brief Optional function to handle SDL events dispatched by the main event loop.
     * @param manager The EntityManager instance managing this entity.
     * @param state Pointer to the main AppState.
     * @param event The SDL_Event to handle.
     */
    void (*handle_events)(EntityManager manager, AppState *state, SDL_Event *event);

    /**
     * @brief Optional function called once per frame for updates (e.g., physics, AI).
     * @param manager The EntityManager instance managing this entity.
     * @param state Pointer to the main AppState (contains delta_time, etc.).
     */
    void (*update)(EntityManager manager, AppState *state);

    /**
     * @brief Optional function called once per frame for rendering.
     * @param manager The EntityManager instance managing this entity.
     * @param state Pointer to the main AppState (contains renderer, etc.).
     */
    void (*render)(EntityManager manager, AppState *state);

} EntityFunctions;

// --- Public API Function Declarations ---

/**
 * @brief Creates a new EntityManager instance.
 * @param max_entities The maximum number of entity types this manager can hold.
 * @return A new EntityManager instance, or NULL on failure.
 * @sa EntityManager_Destroy
 */
EntityManager EntityManager_Create(int max_entities);

/**
 * @brief Destroys an EntityManager instance and cleans up its entities.
 * Calls the cleanup function for all registered entities before freeing memory.
 * @param manager The EntityManager instance to destroy.
 * @param state Pointer to the main AppState (passed to entity cleanup functions).
 * @sa EntityManager_Create
 */
void EntityManager_Destroy(EntityManager manager, AppState *state);

/**
 * @brief Adds a new entity definition (set of callbacks) to the manager.
 * The manager stores these definitions and calls their functions during the main loop.
 * @param manager The EntityManager instance.
 * @param funcs A pointer to an EntityFunctions struct defining the entity's behavior.
 * @return True on success, false on failure (e.g., manager is full or name conflict).
 * @sa EntityFunctions
 */
bool EntityManager_Add(EntityManager manager, const EntityFunctions *funcs);

/**
 * @brief Calls the handle_events function for all registered entities that have one.
 * @param manager The EntityManager instance.
 * @param state Pointer to the main AppState.
 * @param event The SDL_Event to process.
 */
void EntityManager_HandleEventsAll(EntityManager manager, AppState *state, SDL_Event *event);

/**
 * @brief Calls the update function for all registered entities that have one.
 * @param manager The EntityManager instance.
 * @param state Pointer to the main AppState.
 */
void EntityManager_UpdateAll(EntityManager manager, AppState *state);

/**
 * @brief Calls the render function for all registered entities that have one.
 * @param manager The EntityManager instance.
 * @param state Pointer to the main AppState.
 */
void EntityManager_RenderAll(EntityManager manager, AppState *state);

/**
 * @brief Finds an entity definition by its registered name.
 * Primarily for debugging or specific lookup needs.
 * @param manager The EntityManager instance.
 * @param name The name of the entity definition to find.
 * @return A pointer to the internal EntityFunctions if found, NULL otherwise.
 * @warning Do not modify the returned structure.
 */
const EntityFunctions *EntityManager_FindDefinition(EntityManager manager, const char *name);
