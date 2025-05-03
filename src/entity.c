#include "../include/entity.h"

// --- Global Variables ---
Entity entities[MAX_ENTITIES];
int entities_count = 0;

// --- Public API Function Implementations ---
SDL_AppResult create_entity(Entity entity)
{
  if (entities_count < MAX_ENTITIES)
  {
    // Check if an entity with the same name already exists.
    if (find_entity(entity.name) != -1)
    {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Entity] Entity with name '%s' already exists.", entity.name);
      return SDL_APP_FAILURE;
    }
    entities[entities_count] = entity;
    entities_count++;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Entity] Created entity: %s", entity.name);
    return SDL_APP_SUCCESS;
  }
  else
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Entity] Maximum number of entities (%d) reached. Cannot create entity '%s'.", MAX_ENTITIES, entity.name);
    return SDL_APP_FAILURE;
  }
}

SDL_AppResult delete_entity(int index)
{
  if (index < 0 || index >= entities_count)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Entity] Invalid index %d provided for deletion (count: %d).", index, entities_count);
    return SDL_APP_FAILURE;
  }

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Entity] Deleting entity: %s (index %d)", entities[index].name, index);

  // Call the entity's specific cleanup function, if provided.
  if (entities[index].cleanup)
  {
    entities[index].cleanup();
  }

  // Overwrite the entity at the given index with the last entity in the array.
  // This avoids shifting all subsequent elements.
  if (index < entities_count - 1)
  {
    entities[index] = entities[entities_count - 1];
  }

  entities_count--;
  // Clear the now-unused last slot.
  memset(&entities[entities_count], 0, sizeof(Entity));

  return SDL_APP_SUCCESS;
}

void swap_entities(int index1, int index2)
{
  // Basic validation for indices.
  if (index1 < 0 || index1 >= entities_count || index2 < 0 || index2 >= entities_count)
  {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Entity] Invalid indices (%d, %d) provided for swap (count: %d).", index1, index2, entities_count);
    return;
  }
  if (index1 == index2)
  {
    return; // No operation needed if indices are the same.
  }

  // Perform swap using a temporary variable.
  Entity temp = entities[index1];
  entities[index1] = entities[index2];
  entities[index2] = temp;
}

int find_entity(const char *name)
{
  if (!name)
  {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Entity] Attempted to find entity with NULL name.");
    return -1;
  }

  for (int i = 0; i < entities_count; i++)
  {
    // Compare the provided name with the name of the entity at the current index.
    if (strcmp(entities[i].name, name) == 0)
    {
      return i; // Return the index if a match is found.
    }
  }
  return -1; // Return -1 if no entity with the given name is found.
}
