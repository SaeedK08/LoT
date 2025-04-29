#pragma once
#include <string.h>
#include <SDL3/SDL.h>
#include "../include/common.h"

SDL_AppResult create_entity(Entity entity);
SDL_AppResult delete_entity(int index);

void swap_entities(int index1, int index2);
int find_entity(const char *name);