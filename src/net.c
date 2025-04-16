#include "../include/net.h"

static void cleanup()
{
    SDLNet_Quit();
    SDL_Log("SDLNet has been cleaned up.");
}

SDL_AppResult init_net()
{
    if (!SDLNet_Init())
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error initializing SDL_Net: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Log("SDLNet has been initialized.");

    Entity net_e = {
        .name = "net",
        .cleanup = cleanup};
    create_entity(net_e);

    return SDL_APP_SUCCESS;
}