#pragma once

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include "../include/common.h"
#include "../include/entity.h"

#define SERVER_PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 512
#define HOSTNAME "130.229.135.149"

int funcGetAmountPlayers();

SDL_AppResult init_server(void);