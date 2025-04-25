#include "../include/attack.h"

typedef struct FireBall {
    SDL_Texture* fireBallTex;
    SDL_FPoint fireBallSrc;
    SDL_FPoint fireBallDst;
    SDL_FRect target;
    int hit;
    SDL_FlipMode filpMode;
} FireBall;

static FireBall fireBall;

void directFireBall() {
    if (fireBall.fireBallDst.x < fireBall.target.x){
        fireBall.fireBallDst.x += 10;
        fireBall.fireBallSrc.x += 40;
        if (fireBall.fireBallSrc.x >= 377) fireBall.fireBallSrc.x = 0;
    }
    if (fireBall.fireBallDst.y > fireBall.target.y) {
        fireBall.fireBallDst.y -= 10;
        fireBall.fireBallSrc.x += 40;
        if (fireBall.fireBallSrc.x >= 377) fireBall.fireBallSrc.x = 0;
    }
    if (fireBall.fireBallDst.x>=fireBall.target.x && fireBall.fireBallDst.y<=fireBall.target.y) {
        fireBall.hit = 1;
        SDL_DestroyTexture(fireBall.fireBallTex);
        SDL_Log("HIT!!!!\n");
        // SDL_Delay(10);
        return; 
    }
}

bool renderFireBall(SDL_Renderer *renderer) {
    if (fireBall.hit) return false;
    directFireBall();
    SDL_FRect srcrect = {fireBall.fireBallSrc.x, fireBall.fireBallSrc.y, 34.0f, 39.0f};
    SDL_FRect dstrect = {fireBall.fireBallDst.x, fireBall.fireBallDst.y, 34.0f, 39.0f};
    SDL_RenderTextureRotated(renderer, fireBall.fireBallTex, &srcrect, &dstrect, 0, NULL, SDL_FLIP_NONE);
    SDL_Delay(50);
    return true;
}

void fireBallInit(SDL_Renderer *renderer, SDL_FPoint mousePos, SDL_FPoint player_position)  {
    fireBall.fireBallSrc = (SDL_FPoint){64.0f, 26.0f};
    fireBall.fireBallDst = (SDL_FPoint) {player_position.x - camera.x - PLAYER_WIDTH/2, player_position.y - camera.y - PLAYER_HEIGHT/2};
    fireBall.target = (SDL_FRect){mousePos.x/4.26f,mousePos.y/4.26f,100.0f,100.0f};    // Scaling down mouse coordinates to camera view size
    SDL_Log("TARGET x: %f, y: %f", fireBall.target.x, fireBall.target.y);
    if (fireBall.target.x < player_position.x) fireBall.filpMode = SDL_FLIP_HORIZONTAL;
    else fireBall.filpMode = SDL_FLIP_NONE;
    fireBall.hit = 0;
    char fireBallPath[] = "./resources/Sprites/Red_Team/Fire_Wizard/Charge.png";
    fireBall.fireBallTex = IMG_LoadTexture(renderer, fireBallPath);
    if (!fireBall.fireBallTex) {
        SDL_Log("Error loading texture for fire ball: %s\n" ,SDL_GetError());
        return;
    }
}
