#ifndef PLAYER_H
#define PLAYER_H

#define PLAYER_WIDTH 35
#define PLAYER_HEIGHT 67

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdbool.h>

typedef struct Player Player; // Define a type alias for the player structure
Player *createPlayer(SDL_Renderer *pRenderer, int windowWidth, int windowHeight);

void render_player(Player *player, SDL_Renderer *renderer);
void update_player(Player *player);
SDL_FPoint funcGetPlayerPosition(Player *player);

#endif