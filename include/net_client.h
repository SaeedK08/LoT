#pragma once

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include "../include/common.h"
#include "../include/net_server.h"

#define HOSTNAME "localhost"

SDL_AppResult init_client();