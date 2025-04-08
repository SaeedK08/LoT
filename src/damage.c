#include "../include/damage.h"
#include "../include/player.h"

SDL_AppResult funcCheckHit(SDL_MouseButtonEvent mouse_event)
{
    // printf("\nfuncCheckHit received mouse event:\n");
    // printf(" - Button index: %u\n", mouse_event.button);
    // printf(" - State: %s\n", mouse_event.down ? "Pressed" : "Released");
    // printf(" - Clicks: %u\n", mouse_event.clicks);
    printf(" - Coordinates: (%.2f, %.2f)\n", mouse_event.x, mouse_event.y);
    // printf(" - Timestamp: %llu ns\n", (unsigned long long)mouse_event.timestamp); // Use %llu for Uint64

    float playerPosx = funcGetPlayerPosition().x;
    float playerPosy = funcGetPlayerPosition().y;

    printf("%f,", playerPosx);
    printf("%f\n", playerPosy);

    if ((mouse_event.x >= playerPosx && mouse_event.x <= playerPosx + PLAYER_WIDTH) && (mouse_event.y >= playerPosy && mouse_event.y <= playerPosy + PLAYER_HEIGHT))
    {
        printf("HIT\n");
    }

    return SDL_APP_SUCCESS;
}