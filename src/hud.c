#include "../include/hud.h"

// --- Internal Structures ---

/**
 * @brief Represents a single active HUD instance on the screen, specialized for text.
 */
typedef struct HUDInstance
{
    bool visible;          /**< Whether this text element is currently visible. */
    char text_buffer[256]; /**< The actual string of text to display. */
    SDL_Texture *texture;  /**< The SDL_Texture holding the rendered text image. */
    SDL_FRect rect;        /**< The position (x,y) and dimensions (w,h) on screen. */
    SDL_Color color;       /**< The color of the text. */
    bool changeable;       /**< True if text_buffer changed and texture needs regeneration. */
} HUDInstance;

/**
 * @brief Internal state for the HUDManager module ADT.
 */
struct HUDManager_s
{
    HUDInstance elements[100]; /**< Pool of HUD instances. */
    int elementCount;
    TTF_Font *fontDefault;
};

/**
 * @brief Wrapper function conforming to EntityFunctions.update signature.
 * @param manager The EntityManager instance (unused).
 * @param state Pointer to the main AppState.
 */
static void HUD_manager_update_callback(EntityManager manager, AppState *state)
{
    (void)manager;
    if (!state || !state->HUD_manager || !state->renderer)
    {
        return;
    }
}

/**
 * @brief Wrapper function conforming to EntityFunctions.render signature.
 * @param manager The EntityManager instance (unused).
 * @param state Pointer to the main AppState.
 */
static void HUD_manager_render_callback(EntityManager manager, AppState *state)
{
    (void)manager;
    if (!state || !state->HUD_manager || !state->renderer)
    {
        return;
    }

    if (state->is_server && state->currentGameState == GAME_STATE_LOBBY)
    {
        SDL_Color color = (SDL_Color){255, 255, 255, 255};
        char text[] = "Host: Focus the game window and type 'start'";
        SDL_Surface *textSurface = TTF_RenderText_Blended(state->HUD_manager->fontDefault, text, sizeof(text), color);
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(state->renderer, textSurface);
        SDL_FRect destinationRect = {0.0f, 0.0f, (float)textSurface->w, (float)textSurface->h};
        SDL_RenderTexture(state->renderer, textTexture, NULL, &destinationRect);
    }

    if (!state->is_server && state->currentGameState == GAME_STATE_LOBBY)
    {
        SDL_Color color = (SDL_Color){255, 255, 255, 255};
        char text[] = "Client: Wating for host to start the game";
        SDL_Surface *textSurface = TTF_RenderText_Blended(state->HUD_manager->fontDefault, text, sizeof(text), color);
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(state->renderer, textSurface);
        SDL_FRect destinationRect = {0.0f, 0.0f, (float)textSurface->w, (float)textSurface->h};
        SDL_RenderTexture(state->renderer, textTexture, NULL, &destinationRect);
    }
}

/**
 * @brief Wrapper function conforming to EntityFunctions.cleanup signature.
 * @param manager The EntityManager instance (unused).
 * @param state Pointer to the main AppState.
 */
static void HUD_manager_cleanup_callback(EntityManager manager, AppState *state)
{
    (void)manager;
    if (!state || !state->HUD_manager || !state->renderer)
    {
        return;
    }
}

// --- Static Helper Functions ---
HUDManager HUDManager_Init(AppState *state)
{
    if (!state || !state->renderer || !state->entity_manager)
    {
        SDL_SetError("Invalid AppState or missing renderer/entity_manager for HUDManager_Init");
        return NULL;
    }

    if (!TTF_WasInit() && !TTF_Init())
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "HUDManager_Init: TTF_Init failed: %s", SDL_GetError());
        return NULL;
    }

    // --- Allocate HUDManager ---
    HUDManager HUD_manager = (HUDManager)SDL_calloc(1, sizeof(struct HUDManager_s));
    if (!HUD_manager)
    {
        SDL_OutOfMemory();
        TTF_Quit();
        return NULL;
    }

    HUD_manager->fontDefault = TTF_OpenFont("./resources/OpenSans-Regular.ttf", HUD_DEFAULT_FONT_SIZE);

    if (!HUD_manager->fontDefault)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "HUDManager_Init: Failed to load default font './resources/OpenSans-Regular.ttf': %s", SDL_GetError());
        SDL_free(HUD_manager->elements);
        SDL_free(HUD_manager);
        TTF_Quit();
        return NULL;
    }
    // --- Register with EntityManager ---
    EntityFunctions HUD_funcs = {
        .name = "HUD_manager",
        .update = HUD_manager_update_callback,
        .render = HUD_manager_render_callback,
        .cleanup = HUD_manager_cleanup_callback,
        .handle_events = NULL};

    if (!EntityManager_Add(state->entity_manager, &HUD_funcs))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[HUD Init] Failed to add entity to manager: %s", SDL_GetError());
        SDL_free(HUD_manager);
        return NULL;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "HUDManager initialized and entity registered.");
    return HUD_manager;
}
