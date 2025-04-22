#include "../include/entity.h"

Entity entities[MAX_ENTITIES];
int entities_count = 0;

SDL_AppResult create_entity(Entity entity)
{
  if (entities_count < MAX_ENTITIES)
  {
    entities[entities_count] = entity;
    entities_count++;
    return SDL_APP_SUCCESS;
  }
  else
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Maximum number of entities (%d) reached. Cannot create entity '%s'.", __func__, MAX_ENTITIES, entity.name);
    return SDL_APP_FAILURE;
  }
}

SDL_AppResult delete_entity(int index)
{
  // Check for invalid index
  if (index < 0 || index >= entities_count) // Check against current count
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Invalid index %d provided (count: %d).", __func__, index, entities_count);
    return SDL_APP_FAILURE;
  }

  // Call the entity's specific cleanup function if it exists
  if (entities[index].cleanup)
  {
    entities[index].cleanup();
  }

  // Shift remaining entities down
  for (int i = index; i < entities_count - 1; i++)
  {
    entities[i] = entities[i + 1];
  }

  entities_count--;
  memset(&entities[entities_count], 0, sizeof(Entity));

  return SDL_APP_SUCCESS;
}

void swap_entities(int index1, int index2)
{
  // Check for invalid indices
  if (index1 < 0 || index1 >= entities_count || index2 < 0 || index2 >= entities_count)
  {
    // Use SDL_LogWarn as this might not be a fatal error depending on context
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[%s] Invalid indices %d and %d provided for swap (count: %d).", __func__, index1, index2, entities_count);
    return; // Don't return failure, just log and exit
  }
  if (index1 == index2)
    return; // No need to swap same index

  // Perform swap
  Entity temp = entities[index1];
  entities[index1] = entities[index2];
  entities[index2] = temp;
}

int find_entity(const char *name)
{
  // Check for NULL input name pointer
  if (!name)
  {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[%s] Attempted to find entity with NULL name.", __func__);
    return -1;
  }

  // Search for entity by name
  for (int i = 0; i < entities_count; i++)
  {
    if (strcmp(entities[i].name, name) == 0)
    {
      return i; // Return index if found
    }
  }
  return -1; // Return -1 if not found
}