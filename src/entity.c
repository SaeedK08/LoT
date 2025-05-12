#include "../include/entity.h"

// --- Internal Structures ---

/**
 * @brief Internal state for the EntityManager module.
 */
struct EntityManager_s
{
    EntityFunctions *entities; /**< Dynamic array of registered entity functions. */
    int count;                 /**< Current number of registered entities. */
    int capacity;              /**< Max number of entities the array can hold. */
};

// --- Public API Function Implementations ---

EntityManager EntityManager_Create(int max_entities)
{
    if (max_entities <= 0)
    {
        SDL_SetError("Cannot create EntityManager with capacity <= 0");
        return NULL;
    }

    EntityManager manager = (EntityManager)SDL_malloc(sizeof(struct EntityManager_s));
    if (!manager)
    {
        SDL_OutOfMemory();
        return NULL;
    }

    manager->entities = (EntityFunctions *)SDL_calloc(max_entities, sizeof(EntityFunctions));
    if (!manager->entities)
    {
        SDL_OutOfMemory();
        SDL_free(manager);
        return NULL;
    }

    manager->count = 0;
    manager->capacity = max_entities;

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "EntityManager created with capacity %d.", max_entities);
    return manager;
}

void EntityManager_Destroy(EntityManager manager, AppState *state)
{
    if (!manager)
    {
        return;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Destroying EntityManager...");
    // Call cleanup for all registered entities in reverse order of addition.
    for (int i = manager->count - 1; i >= 0; --i)
    {
        if (manager->entities[i].cleanup)
        {
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Cleaning up entity: %s", manager->entities[i].name ? manager->entities[i].name : "[Unnamed]");
            manager->entities[i].cleanup(manager, state);
        }
    }

    SDL_free(manager->entities);
    SDL_free(manager);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "EntityManager destroyed.");
}

bool EntityManager_Add(EntityManager manager, const EntityFunctions *funcs)
{
    if (!manager)
    {
        SDL_SetError("EntityManager is NULL");
        return false;
    }
    if (!funcs)
    {
        SDL_SetError("EntityFunctions pointer is NULL");
        return false;
    }
    if (!funcs->name || funcs->name[0] == '\0')
    {
        SDL_SetError("Entity must have a name");
        return false;
    }

    if (manager->count >= manager->capacity)
    {
        SDL_SetError("EntityManager is full (capacity: %d)", manager->capacity);
        return false;
    }

    // Check for duplicate names before adding.
    for (int i = 0; i < manager->count; ++i)
    {
        if (manager->entities[i].name && strcmp(manager->entities[i].name, funcs->name) == 0)
        {
            SDL_SetError("Entity with name '%s' already exists", funcs->name);
            return false;
        }
    }

    manager->entities[manager->count] = *funcs;
    manager->count++;

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Added entity '%s' to EntityManager (count: %d).", funcs->name, manager->count);
    return true;
}

void EntityManager_HandleEventsAll(EntityManager manager, AppState *state, SDL_Event *event)
{
    if (!manager || !state || !event)
    {
        return;
    }

    for (int i = 0; i < manager->count; ++i)
    {
        if (manager->entities[i].handle_events)
        {
            manager->entities[i].handle_events(manager, state, event);
        }
    }
}

void EntityManager_UpdateAll(EntityManager manager, AppState *state)
{
    if (!manager || !state)
    {
        return;
    }

    for (int i = 0; i < manager->count; ++i)
    {
        if (manager->entities[i].update)
        {
            manager->entities[i].update(manager, state);
        }
    }
}

void EntityManager_RenderAll(EntityManager manager, AppState *state)
{
    if (!manager || !state)
    {
        return;
    }

    for (int i = 0; i < manager->count; ++i)
    {
        if (manager->entities[i].render)
        {
            if (state->currentGameState == GAME_STATE_PLAYING || !strcmp(manager->entities[i].name, "HUD_manager"))
            {
                manager->entities[i].render(manager, state);
            }
        }
    }
}

const EntityFunctions *EntityManager_FindDefinition(EntityManager manager, const char *name)
{
    if (!manager || !name)
    {
        return NULL;
    }
    for (int i = 0; i < manager->count; ++i)
    {
        if (manager->entities[i].name && strcmp(manager->entities[i].name, name) == 0)
        {
            return &manager->entities[i];
        }
    }
    return NULL; // Not found
}
