#ifndef ATTACK_H
#define ATTACK_H

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdio.h>
#include <math.h>
#include "../include/player.h"

#define HIT_RANGE 3.0f
#define MAX_FIREBALLS 100
#define FIREBALL_NUM_FRAMES 11
#define FIREBALL_FRAME_WIDTH 40
#define FIREBALL_WIDTH 33
#define FIREBALL_HEIGHT 39
#define FIREBALL_SPEED 100.0f

typedef struct FireBall FireBall;

bool renderFireBall(SDL_Renderer *renderer);
void fireBallInit(SDL_Renderer *renderer, SDL_FPoint mousePos, SDL_FPoint player_position, SDL_FPoint scale_offset);

#endif