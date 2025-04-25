// #include "init.h" // Include the header file for initialization

// #define WINDOW_W 1600
// #define WINDOW_H 900


// struct AppState{
//     SDL_Window *pWindow;     // SDL window structure
//     SDL_Renderer *pRenderer; // SDL renderer structure
//     Player *player; // Pointer to the player structure
//     int windowWidth;       // Window width
//     int windowHeight;      // Window height

// };


// // Callback for application initialization
// SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
// {
//     AppState *pState = SDL_malloc(sizeof(AppState)); // Allocate memory for the game structure
//     if (!pState)
//     {
//         SDL_Log("Failed to allocate memory for game structure"); // Log error
//         return SDL_APP_FAILURE;                                   // Signal initialization failure
//     }
//     // Initialize SDL 
//     if (!SDL_Init(SDL_INIT_VIDEO))
//     {
//         SDL_Log("SDL_Init() failed: %s", SDL_GetError()); // Log SDL error
//         return SDL_APP_FAILURE;                           // Signal initialization failure
//     }

//     pState->windowWidth = WINDOW_W;   // Set window width
//     pState->windowHeight = WINDOW_H;  // Set window height
//     // Create the main window
//     pState->pWindow = SDL_CreateWindow(
//         "League Of Tigers",         // Window title
//         WINDOW_W,                   // Window width
//         WINDOW_H,                   // Window height
//         SDL_WINDOW_RESIZABLE        // Window flags
//     );

//     if (!pState->pWindow)
//     {
//         SDL_Log("SDL_CreateWindow() failed: %s", SDL_GetError()); // Log SDL error
//         return SDL_APP_FAILURE;                                   // Signal initialization failure
//     }

//     // Create a renderer for the window
//     pState->pRenderer = SDL_CreateRenderer(pState->pWindow, NULL); // Use default rendering driver
//     if (!pState->pRenderer)
//     {
//         SDL_Log("SDL_CreateRenderer() failed: %s", SDL_GetError()); // Log SDL error
//         return SDL_APP_FAILURE;                                     // Signal initialization failure
//     }
//     pState->player = createPlayer(pState->pRenderer, pState->windowWidth, pState->windowHeight); // Create the player
//     if (!pState->player)
//     {
//         SDL_Log("Error creating player: %s \n", SDL_GetError()); // Log error
//         return SDL_APP_FAILURE;                                   // Signal initialization failure
//     }

//      *appstate = pState; // Assign the allocated memory to the appstate pointer
    
//     return SDL_APP_CONTINUE; // Signal successful initialization
// }




// void update(AppState *pState)
// {
//     update_player(pState->player);
// }
