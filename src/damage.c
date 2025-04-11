// #include "../include/damage.h"
// #include "../include/player.h"

// SDL_AppResult funcCheckHit(SDL_FPoint mousePos)
// {
//     SDL_FPoint player1Pos = funcGetPlayerPosition();
//     // SDL_FPoint player2Pos = funcGetPlayer2Position();
//     SDL_FRect player2Rect = {player2Pos.x, player2Pos.y, PLAYER_WIDTH, PLAYER_HEIGHT};

//     if (SDL_PointInRectFloat(&mousePos, &player2Rect))
//     {
//         printf("fake hit\n");

//         float deltaX = player1Pos.x - player2Pos.x;
//         float deltaY = player1Pos.y - player2Pos.y;
//         const float attackRange = 100.0f;

//         if (fabsf(deltaX) < attackRange && fabsf(deltaY) < attackRange)
//         {
//             printf("REAL HIT\n");
//         }
//     }

//     return SDL_APP_SUCCESS;
// }