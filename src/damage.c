#include "../include/damage.h"
#include "../include/player.h"

SDL_AppResult funcCheckHit(SDL_FPoint mousePos, Player *player)
{
    SDL_FPoint player1Pos = funcGetPlayerPosition(player);

    return SDL_APP_SUCCESS;
}