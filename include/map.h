#ifndef MAP_H
#define MAP_H

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "../include/cute_tiled.h"

void funcInitMap(SDL_Renderer *renderer);
void funcRenderMap(SDL_Renderer *renderer);

#endif