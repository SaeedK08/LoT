#include <stdbool.h>
#include <stdio.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> //should only be included from "main file" of the project.
#include <SDL3_net/SDL_net.h>

int main(int argv, char **args)
{
    SDLNet_Address **addrs = NULL;
    int num_addrs = 0;
    int i;

    (void)argv;
    (void)args;

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

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Hello SDL3", 800, 600, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

    bool isRunning = true;
    SDL_Event event;

    while (isRunning)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                isRunning = false;
            }
        }
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
