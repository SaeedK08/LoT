#include "../include/net.h"

SDL_AppResult funcNetInit()
{
    SDLNet_Address **addrs = NULL;
    int num_addrs = 0;
    int i;

    if (!SDLNet_Init())
    {
        SDL_Log("SDLNet_Init() failed: %s", SDL_GetError());
        return 1;
    }

    addrs = SDLNet_GetLocalAddresses(&num_addrs);
    if (addrs == NULL)
    {
        SDL_Log("Failed to determine local addresses: %s", SDL_GetError());
        SDLNet_Quit();
        return 1;
    }

    SDL_Log("We saw %d local addresses:", num_addrs);
    for (i = 0; i < num_addrs; i++)
    {
        SDL_Log("  - %s", SDLNet_GetAddressString(addrs[i]));
    }

    SDLNet_FreeLocalAddresses(addrs);
    SDLNet_Quit();

    return SDL_APP_SUCCESS;
}