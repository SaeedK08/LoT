#pragma once

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include "../include/common.h"

#define SERVER_PORT 8080

SDL_AppResult init_server(void);