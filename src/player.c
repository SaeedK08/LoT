#include "../include/player.h"


struct Player{
    SDL_Texture *player_texture, *player_walkTexture;
    SDL_Texture **player_animTextures, **player_walkAnimTextures;
    IMG_Animation *anim, *walk_anim;
    SDL_FPoint playerPos;
    // SDL_FPoint player2Pos = {600.0f, 200.0f};
    SDL_FPoint fireWizardSpritePos;
    Uint64 current_tick, last_tick;
    float delta_time;
    int current_frame, current_frame_walk;
    char anim_path[256]; 
    char walk_anim_path[256];
    char walk_texture[256];

    int windowWidth, windowHeight;

};

Player *createPlayer(SDL_Renderer *pRenderer, int windowWidth, int windowHeight)
{
    Player *player1 = SDL_malloc(sizeof(Player));
    player1->playerPos.x = 800.0f;
    player1->playerPos.y = 400.0f;
    player1->fireWizardSpritePos.x = 59.0f;
    player1->fireWizardSpritePos.y = 56.0f;
    player1->current_frame = 0;
    player1->current_frame_walk = 0;
    strncpy(player1->anim_path, "./resources/Sprites/Red_Team/Fire_Wizard/Idle_Animation.gif", sizeof(player1->anim_path) - 1);
    player1->anim_path[sizeof(player1->anim_path) - 1] = '\0'; // Ensure null termination

    strncpy(player1->walk_anim_path, "./resources/Sprites/Red_Team/Fire_Wizard/Walk_animation.gif", sizeof(player1->walk_anim_path) - 1);
    player1->walk_anim_path[sizeof(player1->walk_anim_path) - 1] = '\0'; // Ensure null termination

    strncpy(player1->walk_texture, "./resources/Sprites/Red_Team/Fire_Wizard/walk_modified.png", sizeof(player1->walk_texture) - 1);
    player1->walk_texture[sizeof(player1->walk_texture) - 1] = '\0'; // Ensure null termination
    // player1->anim_path[256] = "./resources/Sprites/Red_Team/Fire_Wizard/Idle_Animation.gif";
    // player1->walk_anim_path[256] = "./resources/Sprites/Red_Team/Fire_Wizard/Walk_animation.gif";
    // player1->walk_texture[256] = "./resources/Sprites/Red_Team/Fire_Wizard/walk_modified.png";
    player1->windowWidth = windowWidth;
    player1->windowHeight = windowHeight;

    player1->anim = IMG_LoadAnimation(player1->anim_path);
    player1->walk_anim = IMG_LoadAnimation(player1->walk_anim_path);
    player1->player_walkTexture = IMG_LoadTexture(pRenderer, player1->walk_texture);
    if (!player1->anim)
    {
        SDL_Log("Error loading player animation: %s\n", SDL_GetError());
        return NULL; // Signal initialization failure
    }
    if (!player1->walk_anim)
    {
        SDL_Log("Error loading walk animation: %s \n", SDL_GetError());
        return NULL; // Signal initialization failure
    }
    if (!player1->player_walkTexture)
    {
        SDL_Log("Error loading walk texture: %s\n", SDL_GetError());
        return NULL; // Signal initialization failure
    }
    player1->player_animTextures = (SDL_Texture **)SDL_calloc(player1->anim->count, sizeof(*player1->player_animTextures));
    player1->player_walkAnimTextures = (SDL_Texture **)SDL_calloc(player1->walk_anim->count, sizeof(player1->player_walkAnimTextures));
    for (int i = 0; i < player1->anim->count; i++)
    player1->player_animTextures[i] = SDL_CreateTextureFromSurface(pRenderer, player1->anim->frames[i]);
    for (int i = 0; i < player1->walk_anim->count; i++)
    player1->player_walkAnimTextures[i] = SDL_CreateTextureFromSurface(pRenderer, player1->walk_anim->frames[i]);
    return player1;
}



void quit_player()
{
}

void update_player(Player *player)
{
    float player_speed = 40.0f;
    const bool *keyboard_state = SDL_GetKeyboardState(NULL);
    player->current_tick = SDL_GetPerformanceCounter();
    player->delta_time = (player->current_tick - player->last_tick) / (float)(SDL_GetPerformanceFrequency());
    player->last_tick = player->current_tick;
    if (keyboard_state[SDL_SCANCODE_W] && player->playerPos.y > 0)
    {
        player->playerPos.y -= player_speed * player->delta_time;
        // SDL_Delay(1000/60);
        // position.y --;
    }
    if (keyboard_state[SDL_SCANCODE_S] && player->playerPos.y <= player->windowHeight - PLAYER_HEIGHT)
    {
        player->playerPos.y += player_speed * player->delta_time;
        // SDL_Delay(1000/60);
        // position.y ++;
    }
    if (keyboard_state[SDL_SCANCODE_A] && player->playerPos.x > 0)
    {
        player->playerPos.x -= player_speed * player->delta_time;
        player->fireWizardSpritePos.y = 178.0f;
        player->fireWizardSpritePos.x += 128;
        if (player->fireWizardSpritePos.x > 700)
            player->fireWizardSpritePos.x = 56.0f;
        // SDL_Delay(1000/60);
        //  position.x --;
    }
    if (keyboard_state[SDL_SCANCODE_D] && player->playerPos.x < player->windowWidth - PLAYER_WIDTH)
    {
        player->playerPos.x += player_speed * player->delta_time;
        player->fireWizardSpritePos.y = 56.0f;
        player->fireWizardSpritePos.x += 128;
        if (player->fireWizardSpritePos.x > 700)
            player->fireWizardSpritePos.x = 56.0f;
        // SDL_Delay(1000/60);
        // player1Pos.x ++;
    }
    // if (keyboard_state[SDL_SCANCODE_UP] && player2Pos.y > 0)
    // {
    //     player2Pos.y -= player_speed * delta_time;
    //     // SDL_Delay(1000/60);
    //     // position.y --;
    // }
    // if (keyboard_state[SDL_SCANCODE_DOWN] && player2Pos.y <= windowHeight - PLAYER_HEIGHT)
    // {
    //     player2Pos.y += player_speed * delta_time;
    //     // SDL_Delay(1000/60);
    //     // position.y ++;
    // }
    // if (keyboard_state[SDL_SCANCODE_LEFT] && player2Pos.x > 0)
    // {
    //     player2Pos.x -= player_speed * delta_time;
    //     fireWizardSpritePos.y = 178.0f;
    //     fireWizardSpritePos.x += 128;
    //     if (fireWizardSpritePos.x > 700)
    //         fireWizardSpritePos.x = 56.0f;
    //     // SDL_Delay(1000/60);
    //     //  position.x --;
    // }
    // if (keyboard_state[SDL_SCANCODE_RIGHT] && player2Pos.x < windowWidth - PLAYER_WIDTH)
    // {
    //     player2Pos.x += player_speed * delta_time;
    //     fireWizardSpritePos.y = 56.0f;
    //     fireWizardSpritePos.x += 128;
    //     if (fireWizardSpritePos.x > 700)
    //         fireWizardSpritePos.x = 56.0f;
    //     // SDL_Delay(1000/60);
    //     // player1Pos.x ++;
    // }
}

void render_player(Player *player, SDL_Renderer *renderer)
{
    SDL_FRect srcrect = {player->fireWizardSpritePos.x, player->fireWizardSpritePos.y, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_FRect player1PosDest = {player->playerPos.x, player->playerPos.y, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_RenderTexture(renderer, player->player_walkTexture, &srcrect, &player1PosDest);

    // SDL_FRect player2PosDest = {player2Pos.x, player2Pos.y, PLAYER_WIDTH, PLAYER_HEIGHT};
    // SDL_RenderTexture(renderer, player_walkTexture, &srcrect, &player2PosDest);

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

// bool init_player(SDL_Renderer *renderer)
// {
//     char anim_path[] = "./resources/Sprites/Red_Team/Fire_Wizard/Idle_Animation.gif";
//     char walk_anim_path[] = "./resources/Sprites/Red_Team/Fire_Wizard/Walk_animation.gif";
//     char walk_texture[] = "./resources/Sprites/Red_Team/Fire_Wizard/walk_modified.png";
//     anim = IMG_LoadAnimation(anim_path);
//     walk_anim = IMG_LoadAnimation(walk_anim_path);
//     player_walkTexture = IMG_LoadTexture(renderer, walk_texture);
//     if (!anim)
//     {
//         SDL_Log("Error loading player animation: %s\n", SDL_GetError());
//         return SDL_APP_FAILURE; // Signal initialization failure
//     }
//     if (!walk_anim)
//     {
//         SDL_Log("Error loading walk animation: %s \n", SDL_GetError());
//         return SDL_APP_FAILURE; // Signal initialization failure
//     }
//     if (!player_walkTexture)
//     {
//         SDL_Log("Error loading walk texture: %s\n", SDL_GetError());
//         return SDL_APP_FAILURE; // Signal initialization failure
//     }
//     player_animTextures = (SDL_Texture **)SDL_calloc(anim->count, sizeof(*player_animTextures));
//     player_walkAnimTextures = (SDL_Texture **)SDL_calloc(walk_anim->count, sizeof(*player_walkAnimTextures));
//     for (int i = 0; i < anim->count; i++)
//         player_animTextures[i] = SDL_CreateTextureFromSurface(renderer, anim->frames[i]);
//     for (int i = 0; i < walk_anim->count; i++)
//         player_walkAnimTextures[i] = SDL_CreateTextureFromSurface(renderer, walk_anim->frames[i]);
//     return true;
// }

// Function to get the player's current position
SDL_FPoint funcGetPlayer1Position(Player *player)
{
    return player->playerPos; // Return the global position struct
}
// Function to get the player's current position
// SDL_FPoint funcGetPlayer2Position(void)
// {
//     return player2Pos; // Return the global position struct
// }

// walkTexture 1 : {47,62,23,66}
// walkTexture 2: {179,62,19,66}
// walkTexture 3: {308,61,26,67}
// walkTexture 4: {434,61,21,66}
// walkTexture 5: {562,61,22,67}
// walkTexture 6: {688,61,26,67}
