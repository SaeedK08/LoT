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

static void handle_events()
{
}

// Updates LOCAL player state each frame
static void update(float delta_time)
{
  const bool *keyboard_state = SDL_GetKeyboardState(NULL);
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

  if (player_position.x < PLAYER_WIDTH / 2.0f)
    player_position.x = PLAYER_WIDTH / 2.0f;
  if (player_position.y < PLAYER_HEIGHT / 2.0f)
    player_position.y = PLAYER_HEIGHT / 2.0f;
  if (player_position.x > MAP_WIDTH - PLAYER_WIDTH / 2.0f)
    player_position.x = MAP_WIDTH - PLAYER_WIDTH / 2.0f;
  if (player_position.y > MAP_HEIGHT - PLAYER_HEIGHT / 2.0f)
    player_position.y = MAP_HEIGHT - PLAYER_HEIGHT / 2.0f;

  if (is_moving)
  {
    // If we weren't moving before, or switched animation row, reset frame
    if (sprite_portion.y != WALK_ROW_Y)
    {
      current_frame = 0;
      anim_timer = 0.0f;
      sprite_portion.y = WALK_ROW_Y;
    }

    // Update animation timer
    anim_timer += delta_time;

    // Advance frame if timer exceeds threshold
    if (anim_timer >= TIME_PER_FRAME)
    {
      anim_timer -= TIME_PER_FRAME; // Reset timer, keeping remainder
      current_frame++;
      if (current_frame >= NUM_WALK_FRAMES) // Wrap frame index
      {
        current_frame = 0;
      }
    }
    // Calculate current frame's X position in the spritesheet
    sprite_portion.x = current_frame * FRAME_WIDTH;
  }
  else // Not moving - Handle Idle Animation FOR LOCAL PLAYER
  {
    // If we were moving before, or switched to idle row, reset frame
    if (sprite_portion.y != IDLE_ROW_Y)
    {
      current_frame = 0;
      anim_timer = 0.0f;
      sprite_portion.y = IDLE_ROW_Y; // Set Y coordinate for idle animation
    }

    // Update animation timer for idle
    anim_timer += delta_time;

    // Advance idle frame if timer exceeds threshold
    if (anim_timer >= TIME_PER_FRAME)
    {
      anim_timer -= TIME_PER_FRAME;
      current_frame++;
      if (current_frame >= NUM_IDLE_FRAMES) // Wrap idle frame index
      {
        current_frame = 0;
      }
    }
    // Calculate current idle frame's X position
    sprite_portion.x = current_frame * FRAME_WIDTH;
  }
  sprite_portion.w = FRAME_WIDTH;
  sprite_portion.h = FRAME_HEIGHT;
}

// Render the LOCAL player each frame
static void render(SDL_Renderer *renderer)
{
  float screen_x = player_position.x - camera.x - PLAYER_WIDTH / 2.0f;
  float screen_y = player_position.y - camera.y - PLAYER_HEIGHT / 2.0f;

  SDL_FRect player_dest_rect = {
      screen_x,
      screen_y,
      PLAYER_WIDTH,
      PLAYER_HEIGHT};

  SDL_RenderTextureRotated(renderer, player_texture, &sprite_portion, &player_dest_rect, 0, NULL, flip_mode);
}

void render_remote_players(SDL_Renderer *renderer)
{
  if (!player_texture)
    return; // Don't render if texture isn't loaded

  SDL_FRect remote_sprite_portion = {
      0.0f,
      IDLE_ROW_Y,
      FRAME_WIDTH,
      FRAME_HEIGHT};

  for (int i = 0; i < MAX_CLIENTS; ++i)
  {
    if (remotePlayers[i].active)
    {
      // Calculate remote player's screen position based on their world position relative to camera
      float screen_x = remotePlayers[i].position.x - camera.x - PLAYER_WIDTH / 2.0f;
      float screen_y = remotePlayers[i].position.y - camera.y - PLAYER_HEIGHT / 2.0f;

      SDL_FRect remote_player_dest_rect = {
          screen_x,
          screen_y,
          PLAYER_WIDTH,
          PLAYER_HEIGHT};

      SDL_RenderTexture(renderer, player_texture, &remote_sprite_portion, &remote_player_dest_rect);
    }
  }
}

void init_player(SDL_Renderer *renderer)
{
  const char path[] = "./resources/Sprites/Red_Team/Fire_Wizard/Fire_Wizard_Spiresheet.png";
  player_texture = IMG_LoadTexture(renderer, path);

  if (!player_texture)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load player texture at %s: %s", path, SDL_GetError());
    return;
  }

  SDL_SetTextureScaleMode(player_texture, SDL_SCALEMODE_NEAREST);

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

  create_entity(player_entity);
  SDL_Log("Player entity initialized.");
}