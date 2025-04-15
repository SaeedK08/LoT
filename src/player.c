#include "../include/player.h"

// Global variable storing the player's current world position
SDL_FPoint player_position = {400.0f, 400.0f};

// --- Static variables local to this file ---
static SDL_Texture *player_texture;            // Texture containing the player sprite sheet
static SDL_FRect sprite_portion;               // Source rect from the texture (x, y, w, h) - defines current frame
static int movement_speed = 150;               // Player speed in pixels per second
static SDL_FlipMode flip_mode = SDL_FLIP_NONE; // Current sprite flipping state

// --- Animation Constants ---
static const float FRAME_WIDTH = 48.0f;   // Width of a single frame
static const float FRAME_HEIGHT = 80.0f;  // Height of a single frame
static const float IDLE_ROW_Y = 0.0f;     // Y coordinate on spritesheet for idle frames
static const float WALK_ROW_Y = 80.0f;    // Y coordinate for walking frames
static const int NUM_IDLE_FRAMES = 6;     // Number of frames in idle animation (e.g., 6 frames: 0-5)
static const int NUM_WALK_FRAMES = 6;     // Number of frames in walk cycle (e.g., 6 frames: 0-5)
static const float TIME_PER_FRAME = 0.1f; // Time each animation frame is displayed (in seconds)

// --- Animation State Variables ---
static float anim_timer = 0.0f; // Timer to track frame duration
static int current_frame = 0;   // Index of the current frame (0 to NUM_FRAMES-1)
static _Bool is_moving = 0;     // Flag to track if player is currently moving

// Function to clean up player resources
static void cleanup()
{
  if (player_texture)
  {
    SDL_DestroyTexture(player_texture);
    player_texture = NULL;
  }
}

// Function to handle player-specific events
static void handle_events()
{
}

// Function to update player state each frame
static void update(float delta_time)
{
  const bool *keyboard_state = SDL_GetKeyboardState(NULL);
  is_moving = 0; // Assume not moving until input detected

  // --- Handle Movement Input & Flip ---
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
    flip_mode = SDL_FLIP_HORIZONTAL; // Set flip left
    is_moving = 1;
  }
  if (keyboard_state[SDL_SCANCODE_D])
  {
    player_position.x += movement_speed * delta_time;
    flip_mode = SDL_FLIP_NONE; // Set flip right (no flip)
    is_moving = 1;
  }

  // --- Handle Animation State & Timing ---
  if (is_moving)
  {
    // If we weren't moving before, or switched animation row, reset frame
    if (sprite_portion.y != WALK_ROW_Y)
    {
      current_frame = 0;
      anim_timer = 0.0f;
      sprite_portion.y = WALK_ROW_Y; // Set Y coordinate for walking animation
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
  else // Not moving - Handle Idle Animation
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
  // Ensure sprite portion uses defined width/height
  sprite_portion.w = PLAYER_WIDTH;
  sprite_portion.h = PLAYER_HEIGHT;

  // --- Clamp player position to map boundaries ---
  // Check against left edge
  if (player_position.x <= PLAYER_WIDTH)
  {
    player_position.x = PLAYER_WIDTH;
  }
  // Check against right edge
  if (player_position.x >= MAP_WIDTH)
  {
    player_position.x = MAP_WIDTH;
  }
  // Check against CliffWall layer
  if (player_position.y <= 320)
  {
    player_position.y = 320;
  }
  // Check against Water layer
  if (player_position.y >= 1440 - 40)
  {
    player_position.y = 1440 - 40;
  }
}

// Function to render the player each frame
static void render(SDL_Renderer *renderer)
{
  // Calculate the player's screen X/Y relative to the camera's view center
  // This is the *target* position if the camera isn't at an edge.
  float centered_x = camera.w / 2.0f - PLAYER_WIDTH / 2.0f;
  float centered_y = camera.h / 2.0f - PLAYER_HEIGHT / 2.0f;

  // Start with the default centered position
  float final_x = centered_x;
  float final_y = centered_y;

  // --- Adjust screen position based on camera hitting map edges ---

  // If camera hits the LEFT edge, the player's screen X is just their world X
  if (camera.x <= 0)
  {
    final_x = player_position.x;
  }
  // If camera hits the RIGHT edge, calculate position relative to the right side of the camera view
  else if (camera.x + camera.w >= MAP_WIDTH)
  {
    // Player's world X relative to the camera's left edge + offset from map edge
    final_x = player_position.x - (MAP_WIDTH - camera.w);
  }

  // If camera hits the TOP edge, the player's screen Y is just their world Y
  if (camera.y <= 0)
  {
    final_y = player_position.y;
  }
  // If camera hits the BOTTOM edge, calculate position relative to the bottom side of the camera view
  else if (camera.y + camera.h >= MAP_HEIGHT)
  {
    // Player's world Y relative to the camera's top edge + offset from map edge
    final_y = player_position.y - (MAP_HEIGHT - camera.h);
  }

  // Define the destination rectangle on the screen
  SDL_FRect player_rect = {
      final_x,
      final_y,
      PLAYER_WIDTH,
      PLAYER_HEIGHT};

  // Render the current sprite portion with calculated flip state
  SDL_RenderTextureRotated(renderer, player_texture, &sprite_portion, &player_rect, 0, NULL, flip_mode);
}

// Initializes the player entity
void init_player(SDL_Renderer *renderer)
{
  const char path[] = "./resources/Sprites/Red_Team/Fire_Wizard/Fire_Wizard_Spiresheet.png";
  player_texture = IMG_LoadTexture(renderer, path);

  // Error check texture loading
  if (!player_texture)
  {
    SDL_Log("Failed to load player texture at %s: %s", path, SDL_GetError());
    return;
  }

  // Set scaling mode for pixel art
  SDL_SetTextureScaleMode(player_texture, SDL_SCALEMODE_NEAREST);

  // Initialize sprite portion to the first idle frame
  sprite_portion.x = 0 * FRAME_WIDTH; // Start at frame 0
  sprite_portion.y = IDLE_ROW_Y;      // Start on idle row
  sprite_portion.w = PLAYER_WIDTH;    // Use defined width
  sprite_portion.h = PLAYER_HEIGHT;   // Use defined height

  // Define the player entity and assign its functions
  Entity player = {
      .name = "player",
      .cleanup = cleanup,
      .handle_events = handle_events,
      .update = update,
      .render = render};

  // Register the player entity
  create_entity(player);
}