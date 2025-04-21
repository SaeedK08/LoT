#pragma once

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include <stdbool.h>
#include <string.h>

#include "../include/common.h"
#include "../include/net_server.h"
#include "../include/player.h"

typedef struct
{
    SDL_FPoint position;
    bool active;
} RemotePlayer;

extern RemotePlayer remotePlayers[MAX_CLIENTS];

SDL_AppResult init_client();