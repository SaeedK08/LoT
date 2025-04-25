#ifndef ATTACK_H
#define ATTACK_H
#endif

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdio.h>
#include <math.h>
#include "../include/player.h"


typedef struct FireBall FireBall;
bool renderFireBall(SDL_Renderer *renderer);
void fireBallInit(SDL_Renderer *renderer, SDL_FPoint mousePos, SDL_FPoint player_position);
void directFireBall();