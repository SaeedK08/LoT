#include "../include/player.h"

struct Player
{
    SDL_Texture *player_texture;
    SDL_FPoint playerPos, player_srcrect;
    Uint64 current_tick, last_tick;
    float delta_time, player_speed;
    char sprite_sheet_path[256];
    SDL_FlipMode flipMode;
    int windowWidth, windowHeight;
};

Player *createPlayer(SDL_Renderer *pRenderer, int windowWidth, int windowHeight)
{
    Player *player = SDL_malloc(sizeof(Player));
    player->playerPos.x = 400.0f;
    player->playerPos.y = 400.0f;
    player->player_srcrect.x = 0;
    player->player_srcrect.y = 0;
    player->flipMode = SDL_FLIP_NONE;
    player->player_speed = 20.0f;

    strncpy(player->sprite_sheet_path, "./resources/Sprites/Red_Team/Fire_Wizard/Fire_Wizard_Spiresheet.png", sizeof(player->sprite_sheet_path) - 1);
    player->sprite_sheet_path[sizeof(player->sprite_sheet_path) - 1] = '\0'; // Ensure null termination
    player->windowWidth = windowWidth;
    player->windowHeight = windowHeight;

    player->player_texture = IMG_LoadTexture(pRenderer, player->sprite_sheet_path);
    if (!player->player_texture)
    {
        SDL_Log("Error loading sprite sheet for player: %s\n", SDL_GetError());
        return NULL;
    }

    return player;
}

void quit_player()
{
}

void update_player(Player *player)
{
    const bool *keyboard_state = SDL_GetKeyboardState(NULL);
    player->current_tick = SDL_GetPerformanceCounter();
    player->delta_time = (player->current_tick - player->last_tick) / (float)(SDL_GetPerformanceFrequency());
    player->last_tick = player->current_tick;

    if (keyboard_state[SDL_SCANCODE_W] && player->playerPos.y > 0)
    {
        player->playerPos.y -= player->player_speed * player->delta_time;
    }

    if (keyboard_state[SDL_SCANCODE_S] && player->playerPos.y <= player->windowHeight - PLAYER_HEIGHT)
    {
        player->playerPos.y += player->player_speed * player->delta_time;
    }

    if (keyboard_state[SDL_SCANCODE_A] && player->playerPos.x > 0)
    {
        player->flipMode = SDL_FLIP_HORIZONTAL;
        player->playerPos.x -= player->player_speed * player->delta_time;
        player->player_srcrect.y = 80.0f;
        player->player_srcrect.x += 48.0f;
        if (player->player_srcrect.x > 270)
            player->player_srcrect.x = 0;
    }

    if (keyboard_state[SDL_SCANCODE_D] && player->playerPos.x < player->windowWidth - PLAYER_WIDTH)
    {
        player->playerPos.x += player->player_speed * player->delta_time;
        player->player_srcrect.y = 80.0f;
        player->player_srcrect.x += 48.0f;
        if (player->player_srcrect.x > 270)
            player->player_srcrect.x = 0;
    }
    if (keyboard_state[SDL_SCANCODE_D] && keyboard_state[SDL_SCANCODE_TAB])
    {
        player->playerPos.x += player->player_speed * 2.5 * player->delta_time;
        player->player_srcrect.y = 80 * 2;
        player->player_srcrect.x += 48;
        if (player->player_srcrect.x > 370)
            player->player_srcrect.x = 0;
    }
    if (keyboard_state[SDL_SCANCODE_A] && keyboard_state[SDL_SCANCODE_TAB])
    {
        player->flipMode = SDL_FLIP_HORIZONTAL;
        player->playerPos.x -= player->player_speed * 2.5 * player->delta_time;
        player->player_srcrect.y = 80 * 2;
        player->player_srcrect.x += 48;
        if (player->player_srcrect.x > 370)
            player->player_srcrect.x = 0;
    }
}

void render_player(Player *player, SDL_Renderer *renderer)
{
    SDL_FRect srcrect = {player->player_srcrect.x, player->player_srcrect.y, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_FRect player1PosDest = {player->playerPos.x, player->playerPos.y, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_RenderTextureRotated(renderer, player->player_texture, &srcrect, &player1PosDest, 0, NULL, player->flipMode);
    player->flipMode = SDL_FLIP_NONE;
    SDL_Delay(150);
}

void handle_player_events()
{
}

SDL_FPoint funcGetPlayerPosition(Player *player)
{
    return player->playerPos; // Return the global position struct
}