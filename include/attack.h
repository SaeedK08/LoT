#ifndef ATTACK_H
#define ATTACK_H

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdio.h>
#include <math.h>
#include "../include/player.h"

#define HIT_RANGE 3.0f
#define MAX_FIREBALLS 10
#define FIREBALL_FRAME_WIDTH 40
#define FIREBALL_FRAME_HEIGHT 40
#define FIREBALL_WIDTH 33
#define FIREBALL_HEIGHT 39
#define FIREBALL_SPEED 100.0f

typedef struct FireBall
{
    SDL_Texture *texture;
    SDL_FPoint src;
    SDL_FPoint dst;
    SDL_FPoint target;
    int hit;
    int active;
    float angle_deg;
    float velocity_x, velocity_y;
    int rotation_diff_x, rotation_diff_y;
} FireBall;

extern FireBall fireBalls[MAX_FIREBALLS];

SDL_AppResult init_fireball(SDL_Renderer *renderer);
void activate_fireballs(float player_pos_x, float player_pos_y, float cam_x, float cam_y, float mouse_view_x, float mouse_view_y);

#endif