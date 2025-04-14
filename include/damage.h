#ifndef DAMAGE_H
#define DAMAGE_H
#endif

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdio.h>
#include <math.h>
#include "../include/player.h"

SDL_AppResult funcCheckHit(SDL_FPoint mousePos, Player *player);
