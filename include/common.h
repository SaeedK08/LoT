#pragma once

// --- Standard Library Includes ---
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// --- SDL/External Library Includes ---
#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_ttf/SDL_textengine.h>

// --- Project Includes ---
#include "../include/app_state.h"
#include "../include/entity.h"
#include "../include/network_messages.h"

// --- Universal Constants ---
#define MAX_NAME_LENGTH 64
#define BUFFER_SIZE 512
#define SERVER_PORT 8080
#define DEFAULT_HOSTNAME "localhost"
#define MAX_CLIENTS 10
#define BLUE_TEAM 0
#define RED_TEAM 1

#define CLIFF_BOUNDARY 304.0f
#define WATER_BOUNDARY 1432.0f

#define BUILDINGS_POS_Y 850.0f // All buildings have the same Y position

#define WINDOW_W 1280
#define WINDOW_H 720

/**
 * @brief Enum defining different font sizes.
 */
typedef enum FontSize
{
    HUD_DEFAULT_FONT = 0,
    HUD_SMALL_FONT = 1,
} FontSize;

// --- Universal Macros ---
#define CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))
