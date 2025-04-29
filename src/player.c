#include "../include/player.h"

struct Player
{
  SDL_FPoint position;
  SDL_Texture *texture;
  SDL_FRect sprite_portion;
  int movement_speed;
  SDL_FlipMode flip_mode;
};

Player *players_array[MAX_CLIENTS];

int myIndex;
int amount_players = 0;

static float anim_timer = 0.0f;
static int current_frame = 0;
static _Bool is_moving = 0;

static void cleanup()
{
  if (players_array[myIndex]->texture)
  {
    SDL_DestroyTexture(players_array[myIndex]->texture);
    players_array[myIndex]->texture = NULL;
    SDL_Log("Player texture cleaned up.");
  }
}

static void handle_events(void *appstate, SDL_Event *event)
{
  AppState *pState = (AppState *)(appstate);
  int window_w, window_h;
  float scale_x, scale_y;
  SDL_GetWindowSize(pState->window, &window_w, &window_h);

  // Scaling window size to camera view size
  scale_x = window_w / CAMERA_VIEW_WIDTH;
  scale_y = window_h / CAMERA_VIEW_HEIGHT;

  // Check for left mouse button down event
  if (event->button.button == SDL_BUTTON_LEFT)
  {
    // Calculate mouse position relative to the camera view
    float mouse_view_x = event->button.x / scale_x;
    float mouse_view_y = event->button.y / scale_y;

    // Calculate player's center position in camera view
    float player_view_x = players_array[myIndex]->position.x - camera.x;
    float player_view_y = players_array[myIndex]->position.y - camera.y;

    // Calculate distance between player center and mouse click in view coordinates
    float dist_x = mouse_view_x - player_view_x;
    float dist_y = mouse_view_y - player_view_y;
    float distance = sqrtf(dist_x * dist_x + dist_y * dist_y);

    if (distance <= ATTACK_RANGE)
    {
      activate_fireballs(players_array[myIndex]->position.x, players_array[myIndex]->position.y, camera.x, camera.y, mouse_view_x, mouse_view_y);
    }
  }
}

// Updates LOCAL player state each frame
static void update(AppState *state)
{
  const bool *keyboard_state = SDL_GetKeyboardState(NULL); // Use Uint8
  is_moving = 0;

  if (keyboard_state[SDL_SCANCODE_W])
  {
    players_array[myIndex]->position.y -= players_array[myIndex]->movement_speed * state->delta_time;
    is_moving = 1;
  }
  if (keyboard_state[SDL_SCANCODE_S])
  {
    players_array[myIndex]->position.y += players_array[myIndex]->movement_speed * state->delta_time;
    is_moving = 1;
  }
  if (keyboard_state[SDL_SCANCODE_A])
  {
    players_array[myIndex]->position.x -= players_array[myIndex]->movement_speed * state->delta_time;
    players_array[myIndex]->flip_mode = SDL_FLIP_HORIZONTAL;
    is_moving = 1;
  }
  if (keyboard_state[SDL_SCANCODE_D])
  {
    players_array[myIndex]->position.x += players_array[myIndex]->movement_speed * state->delta_time;
    players_array[myIndex]->flip_mode = SDL_FLIP_NONE;
    is_moving = 1;
  }

  // Clamp player position to map boundaries
  if (players_array[myIndex]->position.x < PLAYER_WIDTH / 2.0f)
    players_array[myIndex]->position.x = PLAYER_WIDTH / 2.0f;
  if (players_array[myIndex]->position.y < PLAYER_HEIGHT / 2.0f)
    players_array[myIndex]->position.y = PLAYER_HEIGHT / 2.0f;
  if (players_array[myIndex]->position.x > MAP_WIDTH - PLAYER_WIDTH / 2.0f)
    players_array[myIndex]->position.x = MAP_WIDTH - PLAYER_WIDTH / 2.0f;
  if (players_array[myIndex]->position.y > MAP_HEIGHT - PLAYER_HEIGHT / 2.0f)
    players_array[myIndex]->position.y = MAP_HEIGHT - PLAYER_HEIGHT / 2.0f;

  // Animation logic
  if (is_moving)
  {
    if (players_array[myIndex]->sprite_portion.y != WALK_ROW_Y)
    {
      current_frame = 0;
      anim_timer = 0.0f;
      players_array[myIndex]->sprite_portion.y = WALK_ROW_Y;
    }
    anim_timer += state->delta_time;
    if (anim_timer >= TIME_PER_FRAME)
    {
      anim_timer -= TIME_PER_FRAME;
      current_frame = (current_frame + 1) % NUM_WALK_FRAMES;
    }
    players_array[myIndex]->sprite_portion.x = current_frame * FRAME_WIDTH;
  }
  else // Idle Animation
  {
    if (players_array[myIndex]->sprite_portion.y != IDLE_ROW_Y)
    {
      current_frame = 0;
      anim_timer = 0.0f;
      players_array[myIndex]->sprite_portion.y = IDLE_ROW_Y;
    }
    anim_timer += state->delta_time;
    if (anim_timer >= TIME_PER_FRAME)
    {
      anim_timer -= TIME_PER_FRAME;
      current_frame = (current_frame + 1) % NUM_IDLE_FRAMES;
    }
    players_array[myIndex]->sprite_portion.x = current_frame * FRAME_WIDTH;
  }
  players_array[myIndex]->sprite_portion.w = FRAME_WIDTH;
  players_array[myIndex]->sprite_portion.h = FRAME_HEIGHT;
}

// Render the LOCAL player each frame
static void render(AppState *state)
{
  // SDL_Log("amountPlayers %d", amount_players);
  for (int i = 0; i <= amount_players; i++)
  {
    // SDL_Log("position %f", players_array[i]->position.x);

    // Calculate screen position based on world position and camera
    float screen_x = players_array[i]->position.x - camera.x - PLAYER_WIDTH / 2.0f;
    float screen_y = players_array[i]->position.y - camera.y - PLAYER_HEIGHT / 2.0f;

    SDL_FRect player_dest_rect = {
        screen_x,
        screen_y,
        PLAYER_WIDTH,
        PLAYER_HEIGHT};

    // Render the current sprite frame
    SDL_RenderTextureRotated(state->renderer, players_array[i]->texture, &players_array[i]->sprite_portion, &player_dest_rect, 0, NULL, players_array[i]->flip_mode);
  }
}

Player *init_player(SDL_Renderer *renderer, int playerIndex)
{
  myIndex = playerIndex;
  Player *current_player = SDL_malloc(sizeof(Player));

  const char path[] = "./resources/Sprites/Red_Team/Fire_Wizard/Fire_Wizard_Spiresheet.png";
  current_player->texture = IMG_LoadTexture(renderer, path);

  if (!current_player->texture)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Failed to load player texture '%s': %s", __func__, path, SDL_GetError());
    return NULL;
  }

  // Set texture scaling mode
  SDL_SetTextureScaleMode(current_player->texture, SDL_SCALEMODE_NEAREST);

  // Initialize sprite portion to the first idle frame
  current_player->sprite_portion.x = 0 * FRAME_WIDTH;
  current_player->sprite_portion.y = IDLE_ROW_Y;
  current_player->sprite_portion.w = FRAME_WIDTH;
  current_player->sprite_portion.h = FRAME_HEIGHT;
  current_player->position = (SDL_FPoint){40.0f, 40.0f};
  current_player->movement_speed = 150;
  current_player->flip_mode = SDL_FLIP_NONE;

  Entity player_entity = {
      .name = "player",
      .cleanup = cleanup,
      .handle_events = handle_events,
      .update = update,
      .render = render};

  if (create_entity(player_entity) == SDL_APP_FAILURE)
  {
    // Cleanup texture if entity creation failed after loading
    SDL_DestroyTexture(current_player->texture);
    current_player->texture = NULL;
    return NULL;
  }

  amount_players = myIndex;
  SDL_Log("myIndex %d", myIndex);

  players_array[myIndex] = current_player;

  SDL_Log("Player entity initialized.");
  return current_player;
}

SDL_FPoint funcGetPlayerPos()
{
  return players_array[myIndex]->position;
}