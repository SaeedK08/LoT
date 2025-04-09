#include "../include/player.h"

SDL_Texture *player_texture, *player_walkTexture;
SDL_Texture **player_animTextures, **player_walkAnimTextures;
IMG_Animation *anim, *walk_anim;
int current_frame = 0, current_frame_walk = 0;
SDL_FPoint player1Pos = {800.0f, 400.0f};
SDL_FPoint player2Pos = {600.0f, 200.0f};
SDL_FPoint fireWizardSpritePos = {59.0f, 56.0f};
Uint64 current_tick, last_tick;
float delta_time;

void quit_player()
{
}

void update_player()
{
    float player_speed = 40.0f;
    const bool *keyboard_state = SDL_GetKeyboardState(NULL);
    current_tick = SDL_GetPerformanceCounter();
    delta_time = (current_tick - last_tick) / (float)(SDL_GetPerformanceFrequency());
    last_tick = current_tick;
    if (keyboard_state[SDL_SCANCODE_W] && player1Pos.y > 0)
    {
        player1Pos.y -= player_speed * delta_time;
        // SDL_Delay(1000/60);
        // position.y --;
    }
    if (keyboard_state[SDL_SCANCODE_S] && player1Pos.y <= WINDOW_H - PLAYER_HEIGHT)
    {
        player1Pos.y += player_speed * delta_time;
        // SDL_Delay(1000/60);
        // position.y ++;
    }
    if (keyboard_state[SDL_SCANCODE_A] && player1Pos.x > 0)
    {
        player1Pos.x -= player_speed * delta_time;
        fireWizardSpritePos.y = 178.0f;
        fireWizardSpritePos.x += 128;
        if (fireWizardSpritePos.x > 700)
            fireWizardSpritePos.x = 56.0f;
        // SDL_Delay(1000/60);
        //  position.x --;
    }
    if (keyboard_state[SDL_SCANCODE_D] && player1Pos.x < WINDOW_W - PLAYER_WIDTH)
    {
        player1Pos.x += player_speed * delta_time;
        fireWizardSpritePos.y = 56.0f;
        fireWizardSpritePos.x += 128;
        if (fireWizardSpritePos.x > 700)
            fireWizardSpritePos.x = 56.0f;
        // SDL_Delay(1000/60);
        // player1Pos.x ++;
    }
    if (keyboard_state[SDL_SCANCODE_UP] && player2Pos.y > 0)
    {
        player2Pos.y -= player_speed * delta_time;
        // SDL_Delay(1000/60);
        // position.y --;
    }
    if (keyboard_state[SDL_SCANCODE_DOWN] && player2Pos.y <= WINDOW_H - PLAYER_HEIGHT)
    {
        player2Pos.y += player_speed * delta_time;
        // SDL_Delay(1000/60);
        // position.y ++;
    }
    if (keyboard_state[SDL_SCANCODE_LEFT] && player2Pos.x > 0)
    {
        player2Pos.x -= player_speed * delta_time;
        fireWizardSpritePos.y = 178.0f;
        fireWizardSpritePos.x += 128;
        if (fireWizardSpritePos.x > 700)
            fireWizardSpritePos.x = 56.0f;
        // SDL_Delay(1000/60);
        //  position.x --;
    }
    if (keyboard_state[SDL_SCANCODE_RIGHT] && player2Pos.x < WINDOW_W - PLAYER_WIDTH)
    {
        player2Pos.x += player_speed * delta_time;
        fireWizardSpritePos.y = 56.0f;
        fireWizardSpritePos.x += 128;
        if (fireWizardSpritePos.x > 700)
            fireWizardSpritePos.x = 56.0f;
        // SDL_Delay(1000/60);
        // player1Pos.x ++;
    }
}

void render_player(SDL_Renderer *renderer)
{
    SDL_FRect srcrect = {fireWizardSpritePos.x, fireWizardSpritePos.y, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_FRect player1PosDest = {player1Pos.x, player1Pos.y, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_RenderTexture(renderer, player_walkTexture, &srcrect, &player1PosDest);

    SDL_FRect player2PosDest = {player2Pos.x, player2Pos.y, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_RenderTexture(renderer, player_walkTexture, &srcrect, &player2PosDest);

    SDL_Delay(150);
    // static SDL_FRect srcrect = {0};
    // SDL_GetWindowSize(); // Try this one out!!
    // float player_width, player_height;
    // SDL_GetTextureSize(player_animTextures[0],&player_width,&player_height);
    // SDL_RenderTexture(renderer, player_animTextures[current_frame++ % anim->count], &srcrect,&player1PosDest);
    // SDL_RenderTexture(renderer, player_walkAnimTextures[current_frame_walk++ % walk_anim->count], NULL, &player1PosDest);
    // SDL_Delay(*(walk_anim->delays));
}

void handle_player_events()
{
}

bool init_player(SDL_Renderer *renderer)
{
    char anim_path[] = "./resources/Sprites/Red_Team/Fire_Wizard/Idle_Animation.gif";
    char walk_anim_path[] = "./resources/Sprites/Red_Team/Fire_Wizard/Walk_animation.gif";
    char walk_texture[] = "./resources/Sprites/Red_Team/Fire_Wizard/walk_modified.png";
    anim = IMG_LoadAnimation(anim_path);
    walk_anim = IMG_LoadAnimation(walk_anim_path);
    player_walkTexture = IMG_LoadTexture(renderer, walk_texture);
    if (!anim)
    {
        SDL_Log("Error loading player animation: %s\n", SDL_GetError());
        return SDL_APP_FAILURE; // Signal initialization failure
    }
    if (!walk_anim)
    {
        SDL_Log("Error loading walk animation: %s \n", SDL_GetError());
        return SDL_APP_FAILURE; // Signal initialization failure
    }
    if (!player_walkTexture)
    {
        SDL_Log("Error loading walk texture: %s\n", SDL_GetError());
        return SDL_APP_FAILURE; // Signal initialization failure
    }
    player_animTextures = (SDL_Texture **)SDL_calloc(anim->count, sizeof(*player_animTextures));
    player_walkAnimTextures = (SDL_Texture **)SDL_calloc(walk_anim->count, sizeof(*player_walkAnimTextures));
    for (int i = 0; i < anim->count; i++)
        player_animTextures[i] = SDL_CreateTextureFromSurface(renderer, anim->frames[i]);
    for (int i = 0; i < walk_anim->count; i++)
        player_walkAnimTextures[i] = SDL_CreateTextureFromSurface(renderer, walk_anim->frames[i]);
    return true;
}

// Function to get the player's current position
SDL_FPoint funcGetPlayer1Position(void)
{
    return player1Pos; // Return the global position struct
}
// Function to get the player's current position
SDL_FPoint funcGetPlayer2Position(void)
{
    return player2Pos; // Return the global position struct
}

// walkTexture 1 : {47,62,23,66}
// walkTexture 2: {179,62,19,66}
// walkTexture 3: {308,61,26,67}
// walkTexture 4: {434,61,21,66}
// walkTexture 5: {562,61,22,67}
// walkTexture 6: {688,61,26,67}
