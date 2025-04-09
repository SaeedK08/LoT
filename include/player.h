#ifndef PLAYER_H
#define PLAYER_H
#endif

#define WINDOW_W 1920
#define WINDOW_H 1080
#define PLAYER_WIDTH 35
#define PLAYER_HEIGHT 67

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdbool.h>

bool init_player(SDL_Renderer *renderer);
void render_player(SDL_Renderer *renderer);
void update_player(void);
SDL_FPoint funcGetPlayer1Position(void);
SDL_FPoint funcGetPlayer2Position(void);