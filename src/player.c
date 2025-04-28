#include "../include/player.h"

SDL_FPoint player_position = {40.0f, 40.0f};

static SDL_Texture *player_texture;
static SDL_FRect sprite_portion;
static int movement_speed = 150;
static SDL_FlipMode flip_mode = SDL_FLIP_NONE;

static const float FRAME_WIDTH = 48.0f;
static const float FRAME_HEIGHT = 80.0f;
static const float IDLE_ROW_Y = 0.0f;
static const float WALK_ROW_Y = 80.0f;
static const int NUM_IDLE_FRAMES = 6;
static const int NUM_WALK_FRAMES = 6;
static const float TIME_PER_FRAME = 0.1f;

static float anim_timer = 0.0f;
static int current_frame = 0;
static _Bool is_moving = 0;

static void cleanup()
{
  if (player_texture)
  {
    SDL_DestroyTexture(player_texture);
    player_texture = NULL;
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
  if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT)
  {
    // Calculate mouse position relative to the camera view
    float mouse_view_x = event->button.x / scale_x;
    float mouse_view_y = event->button.y / scale_y;

    // Calculate player's center position in camera view
    float player_view_x = player_position.x - camera.x;
    float player_view_y = player_position.y - camera.y;

    // Calculate distance between player center and mouse click in view coordinates
    float dist_x = mouse_view_x - player_view_x;
    float dist_y = mouse_view_y - player_view_y;
    float distance = sqrtf(dist_x * dist_x + dist_y * dist_y);

    if (distance <= ATTACK_RANGE)
    {
      activate_fireballs(player_position.x, player_position.y, camera.x, camera.y, mouse_view_x, mouse_view_y);
    }
  }
}

// Updates LOCAL player state each frame
static void update(float delta_time)
{
  const bool *keyboard_state = SDL_GetKeyboardState(NULL); // Use Uint8
  is_moving = 0;

  if (keyboard_state[SDL_SCANCODE_W])
  {
    player_position.y -= movement_speed * delta_time;
    is_moving = 1;
  }
  if (keyboard_state[SDL_SCANCODE_S])
  {
    player_position.y += movement_speed * delta_time;
    is_moving = 1;
  }
  if (keyboard_state[SDL_SCANCODE_A])
  {
    player_position.x -= movement_speed * delta_time;
    flip_mode = SDL_FLIP_HORIZONTAL;
    is_moving = 1;
  }
  if (keyboard_state[SDL_SCANCODE_D])
  {
    player_position.x += movement_speed * delta_time;
    flip_mode = SDL_FLIP_NONE;
    is_moving = 1;
  }

  // Clamp player position to map boundaries
  if (player_position.x < PLAYER_WIDTH / 2.0f)
    player_position.x = PLAYER_WIDTH / 2.0f;
  if (player_position.y < PLAYER_HEIGHT / 2.0f)
    player_position.y = PLAYER_HEIGHT / 2.0f;
  if (player_position.x > MAP_WIDTH - PLAYER_WIDTH / 2.0f)
    player_position.x = MAP_WIDTH - PLAYER_WIDTH / 2.0f;
  if (player_position.y > MAP_HEIGHT - PLAYER_HEIGHT / 2.0f)
    player_position.y = MAP_HEIGHT - PLAYER_HEIGHT / 2.0f;

  // Animation logic
  if (is_moving)
  {
    if (sprite_portion.y != WALK_ROW_Y)
    {
      current_frame = 0;
      anim_timer = 0.0f;
      sprite_portion.y = WALK_ROW_Y;
    }
    anim_timer += delta_time;
    if (anim_timer >= TIME_PER_FRAME)
    {
      anim_timer -= TIME_PER_FRAME;
      current_frame = (current_frame + 1) % NUM_WALK_FRAMES;
    }
    sprite_portion.x = current_frame * FRAME_WIDTH;
  }
  else // Idle Animation
  {
    if (sprite_portion.y != IDLE_ROW_Y)
    {
      current_frame = 0;
      anim_timer = 0.0f;
      sprite_portion.y = IDLE_ROW_Y;
    }
    anim_timer += delta_time;
    if (anim_timer >= TIME_PER_FRAME)
    {
      anim_timer -= TIME_PER_FRAME;
      current_frame = (current_frame + 1) % NUM_IDLE_FRAMES;
    }
    sprite_portion.x = current_frame * FRAME_WIDTH;
  }
  sprite_portion.w = FRAME_WIDTH;
  sprite_portion.h = FRAME_HEIGHT;
}

// Render the LOCAL player each frame
static void render(SDL_Renderer *renderer)
{
  // Calculate screen position based on world position and camera
  float screen_x = player_position.x - camera.x - PLAYER_WIDTH / 2.0f;
  float screen_y = player_position.y - camera.y - PLAYER_HEIGHT / 2.0f;

  SDL_FRect player_dest_rect = {
      screen_x,
      screen_y,
      PLAYER_WIDTH,
      PLAYER_HEIGHT};

  // Render the current sprite frame
  SDL_RenderTextureRotated(renderer, player_texture, &sprite_portion, &player_dest_rect, 0, NULL, flip_mode);
}

void render_remote_players(SDL_Renderer *renderer)
{
  if (!player_texture)
    return; // Don't render if texture isn't loaded

  // Use a fixed portion of the sprite sheet for remote players for now
  SDL_FRect remote_sprite_portion = {
      0.0f,       // First frame
      IDLE_ROW_Y, // Idle animation row
      FRAME_WIDTH,
      FRAME_HEIGHT};

  for (int i = 0; i < MAX_CLIENTS; ++i)
  {
    if (remotePlayers[i].active)
    {
      // Calculate remote player's screen position
      float screen_x = remotePlayers[i].position.x - camera.x - PLAYER_WIDTH / 2.0f;
      float screen_y = remotePlayers[i].position.y - camera.y - PLAYER_HEIGHT / 2.0f;

      SDL_FRect remote_player_dest_rect = {
          screen_x,
          screen_y,
          PLAYER_WIDTH,
          PLAYER_HEIGHT};

      // Render the remote player
      SDL_RenderTexture(renderer, player_texture, &remote_sprite_portion, &remote_player_dest_rect);
    }
  }
}

SDL_AppResult init_player(SDL_Renderer *renderer)
{
  const char path[] = "./resources/Sprites/Red_Team/Fire_Wizard/Fire_Wizard_Spiresheet.png";
  player_texture = IMG_LoadTexture(renderer, path);

  if (!player_texture)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Failed to load player texture '%s': %s", __func__, path, SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // Set texture scaling mode
  SDL_SetTextureScaleMode(player_texture, SDL_SCALEMODE_NEAREST);

  // Initialize sprite portion to the first idle frame
  sprite_portion.x = 0 * FRAME_WIDTH;
  sprite_portion.y = IDLE_ROW_Y;
  sprite_portion.w = FRAME_WIDTH;
  sprite_portion.h = FRAME_HEIGHT;

  Entity player_entity = {
      .name = "player",
      .cleanup = cleanup,
      .handle_events = handle_events,
      .update = update,
      .render = render};

  if (create_entity(player_entity) == SDL_APP_FAILURE)
  {
    // Cleanup texture if entity creation failed after loading
    SDL_DestroyTexture(player_texture);
    player_texture = NULL;
    return SDL_APP_FAILURE;
  }

  SDL_Log("Player entity initialized.");
  return SDL_APP_SUCCESS;
}