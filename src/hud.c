#include "../include/hud.h"

// --- Internal Structures ---

/**
 * @brief Represents a single active HUD instance on the screen, specialized for text.
 */
typedef struct HUDInstance
{
    char name[64];
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
    HUDInstance elements[HUD_MAX_ELEMENTS_AMOUNT]; /**< Pool of HUD instances. */
    int elementCount;
    TTF_Font *fontDefault;
    TTF_Font *fontSmall;
};

int get_hud_element_count(HUDManager hm)
{
    return hm->elementCount;
}

int get_hud_index_by_name(AppState *state, char name[])
{
    for (int i = 0; i < state->HUD_manager->elementCount; i++)
    {
        if (!strcmp(state->HUD_manager->elements[i].name, name))
        {
            return i;
        }
    }
    return -1;
}

void create_hud_instace(AppState *state, int index, char name[], bool visible, char text_buffer[], SDL_Color color, bool changeable, SDL_FPoint dest_point, FontSize fontSize)
{
    HUDManager hm = state ? state->HUD_manager : NULL;
    // Ensure local HUD exists and required managers are available.
    if (!hm || !state)
    {
        return;
    }

    bool existingElement = false;
    for (int i = 0; i < HUD_MAX_ELEMENTS_AMOUNT; i++)
    {
        if (!strcmp(hm->elements[i].name, name))
        {
            existingElement = true;
            // SDL_Log("elementCount:%d", hm->elementCount);
        }
    }
    if (!existingElement)
    {
        hm->elementCount++;
    }

    HUDInstance *currentElement = &state->HUD_manager->elements[index];
    strcpy(currentElement->name, name);
    strcpy(currentElement->text_buffer, text_buffer);
    currentElement->visible = visible;
    currentElement->color = color;
    currentElement->changeable = changeable;

    if (strlen(text_buffer) >= 1)
    {
        TTF_Font *currentFont = fontSize ? state->HUD_manager->fontSmall : state->HUD_manager->fontDefault;
        SDL_Surface *textSurface = TTF_RenderText_Blended(currentFont, text_buffer, strlen(text_buffer), color);
        currentElement->texture = SDL_CreateTextureFromSurface(state->renderer, textSurface);
        currentElement->rect = (SDL_FRect){dest_point.x, dest_point.y, (float)textSurface->w, (float)textSurface->h};
    }
    else
    {
        currentElement->rect = (SDL_FRect){dest_point.x, dest_point.y, 0.0f, 0.0f};
    }
}

static void HUD_manager_event_callback(EntityManager manager, AppState *state, SDL_Event *event)
{
    (void)manager;
    HUDManager hm = state ? state->HUD_manager : NULL;
    // Ensure local HUD exists and required managers are available.
    if (!hm || !state)
    {
        return;
    }

    // Host-specific lobby command input
    if (state->is_server && state->currentGameState == GAME_STATE_LOBBY)
    {
        static char command_input_buffer[128] = {0}; // Static to retain input across events
        static int command_input_len = 0;

        if (event->type == SDL_EVENT_TEXT_INPUT)
        {
            // Append new text to buffer, ensuring no overflow
            if (command_input_len + strlen(event->text.text) < sizeof(command_input_buffer) - 1)
            {
                strcat(command_input_buffer, event->text.text);
                command_input_len += strlen(event->text.text);
                create_hud_instace(state, get_hud_index_by_name(state, "lobby_host_input"), "lobby_host_input", true, command_input_buffer, (SDL_Color){255, 255, 255, 255}, true, (SDL_FPoint){0.0f, 50.0f}, 0);
            }
        }
        else if (event->type == SDL_EVENT_KEY_DOWN)
        {
            if (event->key.scancode == SDL_SCANCODE_RETURN || event->key.scancode == SDL_SCANCODE_KP_ENTER)
            {
                // Process the command when Enter is pressed
                if (stricmp(command_input_buffer, "start") == 0)
                {
                    SDL_Log("Host selected 'start'. Transitioning to GAME_STATE_PLAYING.");
                    state->currentGameState = GAME_STATE_PLAYING;
                    SDL_StopTextInput(state->window);

                    // Broadcast MSG_TYPE_S_GAME_START to all clients
                    if (state->net_server_state)
                    {
                        uint8_t msg_type = MSG_TYPE_S_GAME_START;
                        NetServer_BroadcastMessage(state->net_server_state, &msg_type, sizeof(msg_type), -1);
                    }
                    hm->elements[get_hud_index_by_name(state, "lobby_host_msg")].visible = false;
                    hm->elements[get_hud_index_by_name(state, "lobby_host_input")].visible = false;
                }
                else
                {
                    SDL_Log("Host: Unknown command '%s'. Type 'start' and press Enter.", command_input_buffer);
                }
                // Reset buffer for next command
                command_input_len = 0;
                command_input_buffer[0] = '\0';
            }
            else if (event->key.scancode == SDL_SCANCODE_BACKSPACE && command_input_len > 0)
            {
                // Handle backspace
                command_input_len--;
                command_input_buffer[command_input_len] = '\0';
                create_hud_instace(state, get_hud_index_by_name(state, "lobby_host_input"), "lobby_host_input", true, command_input_buffer, (SDL_Color){255, 255, 255, 255}, true, (SDL_FPoint){0.0f, 50.0f}, 0);
            }
        }
    }
}

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
    HUDManager hm = state ? state->HUD_manager : NULL;
    if (!state || !hm || !state->renderer)
    {
        return;
    }

    for (int i = 0; i < HUD_MAX_ELEMENTS_AMOUNT; i++)
    {
        if (hm->elements[i].visible)
        {
            SDL_RenderTexture(state->renderer, hm->elements[i].texture, NULL, &hm->elements[i].rect);
        }
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
    HUDManager hm = (HUDManager)SDL_calloc(1, sizeof(struct HUDManager_s));
    if (!hm)
    {
        SDL_OutOfMemory();
        TTF_Quit();
        return NULL;
    }

    hm->elementCount = 0;
    hm->fontDefault = TTF_OpenFont("./resources/OpenSans-Regular.ttf", HUD_DEFAULT_FONT_SIZE);
    if (!hm->fontDefault)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "HUDManager_Init: Failed to load default font './resources/OpenSans-Regular.ttf': %s", SDL_GetError());
        SDL_free(hm->elements);
        SDL_free(hm);
        TTF_Quit();
        return NULL;
    }

    hm->fontSmall = TTF_OpenFont("./resources/OpenSans-Regular.ttf", HUD_SMALL_FONT_SIZE);
    if (!hm->fontSmall)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "HUDManager_Init: Failed to load small font './resources/OpenSans-Regular.ttf': %s", SDL_GetError());
        SDL_free(hm->elements);
        SDL_free(hm);
        TTF_Quit();
        return NULL;
    }

    for (int i = 0; i < HUD_MAX_ELEMENTS_AMOUNT; i++)
    {
        hm->elements[i].visible = false;
    }

    // --- Register with EntityManager ---
    EntityFunctions HUD_funcs = {
        .name = "HUD_manager",
        .update = HUD_manager_update_callback,
        .render = HUD_manager_render_callback,
        .cleanup = HUD_manager_cleanup_callback,
        .handle_events = HUD_manager_event_callback};

    if (!EntityManager_Add(state->entity_manager, &HUD_funcs))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[HUD Init] Failed to add entity to manager: %s", SDL_GetError());
        SDL_free(hm);
        return NULL;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "HUDManager initialized and entity registered.");
    return hm;
}
