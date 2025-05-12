#include "../include/common.h"
#include "../include/net_server.h"

// --- Public Functions ---

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  AppState *state = (AppState *)appstate;

  if (event->type == SDL_EVENT_QUIT)
  {
    state->quit_requested = true;
    return SDL_APP_SUCCESS;
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
      }
    }
  }

  if (state->entity_manager)
  {
    EntityManager_HandleEventsAll(state->entity_manager, state, event);
  }
  else
  {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "EntityManager not initialized in SDL_AppEvent.");
  }

  return state->quit_requested ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}