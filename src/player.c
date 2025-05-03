#include "../include/player.h"

struct Player
{
  SDL_FPoint position;
  SDL_Texture *texture;
  SDL_FRect sprite_portion;
  int movement_speed;
  SDL_FlipMode flip_mode;
  bool is_local_player;
  int client_index;
  // --- Animation state ---
  float anim_timer;
  int current_frame;
  bool is_moving;
};

// --- Static Variables ---
Player *players_array[MAX_CLIENTS];
static int local_player_index = -1;

// --- Forward Declarations ---
static void handle_local_player_input(Player *p, float delta_time);
static void update_local_player_position(Player *p);
static void update_local_player_animation(Player *p, float delta_time);
static void render_local_player(AppState *state, Player *p);
static void render_remote_players(AppState *state);
static void cleanup(void);
static void handle_events(void *appstate, SDL_Event *event);
static void update(AppState *state);
static void render(AppState *state);
bool get_local_player_state_for_network(PlayerStateData *out_data);

// --- Cleanup ---
/**
 * @brief Cleans up resources associated with the local player entity.
 * Destroys texture and frees memory for the local player struct.
 * @return void
 */
static void cleanup(void)
{
  if (local_player_index != -1 && players_array[local_player_index] != NULL)
  {
    if (players_array[local_player_index]->texture)
    {
      SDL_DestroyTexture(players_array[local_player_index]->texture);
      players_array[local_player_index]->texture = NULL;
    }
    SDL_free(players_array[local_player_index]);
    players_array[local_player_index] = NULL;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Player %d] Cleaned up.", local_player_index);
  }
  local_player_index = -1; // Reset local player index
}

// --- Event Handling (Local Player Only) ---
/**
 * @brief Handles input events relevant to the local player entity, primarily mouse clicks for attacks.
 * @param appstate Pointer to the global AppState.
 * @param event Pointer to the SDL_Event being processed.
 * @return void
 */
static void handle_events(void *appstate, SDL_Event *event)
{
  if (local_player_index == -1 || players_array[local_player_index] == NULL)
    return; // Ignore if local player not initialized

  AppState *pState = (AppState *)(appstate);
  int window_w, window_h;
  float scale_x, scale_y;

  SDL_GetWindowSize(pState->window, &window_w, &window_h);
  // Avoid division by zero if camera view dimensions are zero
  scale_x = (CAMERA_VIEW_WIDTH > 0) ? (float)window_w / CAMERA_VIEW_WIDTH : 1.0f;
  scale_y = (CAMERA_VIEW_HEIGHT > 0) ? (float)window_h / CAMERA_VIEW_HEIGHT : 1.0f;

  if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT)
  {
    float mouse_view_x = event->button.x / scale_x;
    float mouse_view_y = event->button.y / scale_y;
    float player_view_x = players_array[local_player_index]->position.x - camera.x;
    float player_view_y = players_array[local_player_index]->position.y - camera.y;
    float dist_x = mouse_view_x - player_view_x;
    float dist_y = mouse_view_y - player_view_y;
    float distance = sqrtf(dist_x * dist_x + dist_y * dist_y); // Using sqrtf as per original code

    // Check if click is within attack range
    if (distance <= ATTACK_RANGE)
    {
      // Activate fireball attack originating from player towards the click location
      activate_fireballs(players_array[local_player_index]->position.x, players_array[local_player_index]->position.y, camera.x, camera.y, mouse_view_x, mouse_view_y);
    }
  }
}

// --- Handle Local Player Input & Movement ---
/**
 * @brief Updates the local player's position based on keyboard input state.
 * Also sets the movement flag and sprite flip direction.
 * @param p Pointer to the local Player struct.
 * @param delta_time Time elapsed since the last frame in seconds.
 * @return void
 */
static void handle_local_player_input(Player *p, float delta_time)
{
  const bool *keyboard_state = SDL_GetKeyboardState(NULL);
  p->is_moving = false; // Assume not moving initially

  // --- Vertical Movement ---
  if (keyboard_state[SDL_SCANCODE_W])
  {
    p->position.y -= p->movement_speed * delta_time;
    p->is_moving = true;
  }
  if (keyboard_state[SDL_SCANCODE_S])
  {
    p->position.y += p->movement_speed * delta_time;
    p->is_moving = true;
  }
  // --- Horizontal Movement & Flipping ---
  if (keyboard_state[SDL_SCANCODE_A])
  {
    p->position.x -= p->movement_speed * delta_time;
    p->flip_mode = SDL_FLIP_HORIZONTAL; // Face left
    p->is_moving = true;
  }
  if (keyboard_state[SDL_SCANCODE_D])
  {
    p->position.x += p->movement_speed * delta_time;
    p->flip_mode = SDL_FLIP_NONE; // Face right
    p->is_moving = true;
  }
}

// --- Update Local Player Position & Clamping ---
/**
 * @brief Ensures the local player's position stays within the defined map boundaries.
 * @param p Pointer to the local Player struct.
 * @return void
 */
static void update_local_player_position(Player *p)
{
  // Clamp X position
  p->position.x = fmaxf(PLAYER_WIDTH / 2.0f, fminf(p->position.x, MAP_WIDTH - PLAYER_WIDTH / 2.0f));
  // Clamp Y position
  p->position.y = fmaxf(PLAYER_HEIGHT / 2.0f, fminf(p->position.y, MAP_HEIGHT - PLAYER_HEIGHT / 2.0f));
}

// --- Update Local Player Animation ---
/**
 * @brief Updates the local player's current animation frame based on movement state and timer.
 * Switches between idle and walking animation rows and cycles through frames.
 * @param p Pointer to the local Player struct.
 * @param delta_time Time elapsed since the last frame in seconds.
 * @return void
 */
static void update_local_player_animation(Player *p, float delta_time)
{
  // Determine target animation row and number of frames based on movement
  float target_row_y = p->is_moving ? WALK_ROW_Y : IDLE_ROW_Y;
  int num_frames = p->is_moving ? NUM_WALK_FRAMES : NUM_IDLE_FRAMES;

  // Reset animation if movement state (and thus row) changes
  if (p->sprite_portion.y != target_row_y)
  {
    p->current_frame = 0;
    p->anim_timer = 0.0f;
    p->sprite_portion.y = target_row_y;
  }

  // Advance animation timer and frame
  p->anim_timer += delta_time;
  if (p->anim_timer >= TIME_PER_FRAME)
  {
    p->anim_timer -= TIME_PER_FRAME;                        // Subtract interval duration, don't just reset
    p->current_frame = (p->current_frame + 1) % num_frames; // Loop animation
  }

  // Update the source rectangle for rendering
  p->sprite_portion.x = p->current_frame * FRAME_WIDTH;
  p->sprite_portion.w = FRAME_WIDTH;
  p->sprite_portion.h = FRAME_HEIGHT;
}

// --- Update (Local Player Entity Update Function) ---
/**
 * @brief Updates the state of the local player entity based on input, physics, and animation logic.
 * Called once per frame for the player entity.
 * @param state Pointer to the global AppState, containing delta_time.
 * @return void
 */
static void update(AppState *state)
{
  if (local_player_index == -1 || players_array[local_player_index] == NULL)
    return; // Do nothing if local player isn't initialized

  Player *local_player = players_array[local_player_index];

  // --- Process Local Player Updates ---
  handle_local_player_input(local_player, state->delta_time);
  update_local_player_position(local_player);
  update_local_player_animation(local_player, state->delta_time);
}

// --- Render Local Player ---
/**
 * @brief Renders the sprite for the local player at its current position, adjusted by the camera.
 * @param state Pointer to the global AppState containing the renderer.
 * @param p Pointer to the local Player struct containing rendering information.
 * @return void
 */
static void render_local_player(AppState *state, Player *p)
{
  if (!p || !p->texture)
    return; // Cannot render without player data or texture

  // Calculate position on screen relative to camera
  float screen_x = p->position.x - camera.x - PLAYER_WIDTH / 2.0f;
  float screen_y = p->position.y - camera.y - PLAYER_HEIGHT / 2.0f;
  SDL_FRect player_dest_rect = {screen_x, screen_y, PLAYER_WIDTH, PLAYER_HEIGHT};

  // Render the player's current sprite frame
  SDL_RenderTextureRotated(state->renderer,
                           p->texture,
                           &p->sprite_portion, // Source rect from player state
                           &player_dest_rect,  // Destination rect on screen
                           0.0,                // Angle
                           NULL,               // Center of rotation (NULL for center)
                           p->flip_mode);      // Flip state
}

// --- Render Remote Players ---
/**
 * @brief Renders all active remote players based on their last known network state.
 * Uses the local player's texture asset but applies the remote player's position,
 * sprite portion, and flip mode.
 * @param state Pointer to the global AppState containing the renderer.
 * @return void
 */
static void render_remote_players(AppState *state)
{
  if (local_player_index == -1 || players_array[local_player_index] == NULL || players_array[local_player_index]->texture == NULL)
  {
    return; // Cannot render remote players if local texture is unavailable
  }
  SDL_Texture *player_texture = players_array[local_player_index]->texture;

  // --- Iterate Through Remote Player Slots ---
  for (int i = 0; i < MAX_CLIENTS; ++i)
  {
    // Skip local player and inactive slots
    if (i == local_player_index || !remotePlayers[i].active)
      continue;

    // Calculate screen position using network data and camera
    float screen_x = remotePlayers[i].position.x - camera.x - PLAYER_WIDTH / 2.0f;
    float screen_y = remotePlayers[i].position.y - camera.y - PLAYER_HEIGHT / 2.0f;
    SDL_FRect remote_player_dest_rect = {screen_x, screen_y, PLAYER_WIDTH, PLAYER_HEIGHT};

    // Use sprite portion and flip mode directly from network data
    SDL_FRect remote_sprite_portion = remotePlayers[i].sprite_portion;
    SDL_FlipMode remote_flip_mode = remotePlayers[i].flip_mode;

    // Render using local player's texture but remote player's state
    SDL_RenderTextureRotated(state->renderer,
                             player_texture, // Use local player texture
                             &remote_sprite_portion,
                             &remote_player_dest_rect,
                             0.0,  // Angle
                             NULL, // Center
                             remote_flip_mode);
  }
}

// --- Render (Player Entity Render Function) ---
/**
 * @brief Renders the player entity, drawing both the local player and all active remote players.
 * Called once per frame for the player entity.
 * @param state Pointer to the global AppState containing the renderer.
 * @return void
 */
static void render(AppState *state)
{
  // --- Render Local Player ---
  if (local_player_index != -1 && players_array[local_player_index] != NULL)
  {
    render_local_player(state, players_array[local_player_index]);
  }
  // --- Render Remote Players ---
  render_remote_players(state);
}

// --- Initialization ---
Player *init_player(SDL_Renderer *renderer, int assigned_player_index)
{
  // --- Input Validation ---
  if (assigned_player_index < 0 || assigned_player_index >= MAX_CLIENTS)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Player] Invalid index %d.", assigned_player_index);
    return NULL;
  }
  // Prevent re-initialization if already initialized for this index
  if (players_array[assigned_player_index] != NULL)
  {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Player] Already initialized at index %d.", assigned_player_index);
    return players_array[assigned_player_index];
  }

  local_player_index = assigned_player_index; // Store the index for this client instance

  // --- Allocate Player Struct ---
  Player *new_player = SDL_malloc(sizeof(Player));
  if (!new_player)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Player %d] Malloc failed.", local_player_index);
    local_player_index = -1; // Reset index on failure
    return NULL;
  }
  memset(new_player, 0, sizeof(Player)); // Zero out the struct

  // --- Load Texture ---
  const char path[] = "./resources/Sprites/Red_Team/Fire_Wizard/Fire_Wizard_Spiresheet.png";
  new_player->texture = IMG_LoadTexture(renderer, path);
  if (!new_player->texture)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Player %d] Failed load texture '%s': %s", local_player_index, path, SDL_GetError());
    SDL_free(new_player);
    local_player_index = -1; // Reset index on failure
    return NULL;
  }
  // Use nearest neighbor scaling for pixel art
  SDL_SetTextureScaleMode(new_player->texture, SDL_SCALEMODE_NEAREST);

  // --- Set Initial Player State ---
  new_player->sprite_portion = (SDL_FRect){0.0f, IDLE_ROW_Y, FRAME_WIDTH, FRAME_HEIGHT}; // Start in idle state
  new_player->position = (SDL_FPoint){100.0f, 100.0f};                                   // Default starting position
  new_player->movement_speed = 150;
  new_player->flip_mode = SDL_FLIP_NONE; // Default facing right
  new_player->is_local_player = true;
  new_player->client_index = local_player_index;
  new_player->anim_timer = 0.0f;
  new_player->current_frame = 0;
  new_player->is_moving = false;

  players_array[local_player_index] = new_player; // Store pointer in the global array

  // --- Create Player Entity (if first time initializing) ---
  int existing_entity_idx = find_entity("player");
  if (existing_entity_idx == -1)
  {
    Entity player_entity = {
        .name = "player",
        .cleanup = cleanup,
        .handle_events = handle_events,
        .update = update,
        .render = render};
    if (create_entity(player_entity) == SDL_APP_FAILURE)
    {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Player %d] Failed create entity.", local_player_index);
      cleanup(); // Use cleanup function to handle partial initialization
      return NULL;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Player %d] Player entity created.", local_player_index);
  }

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Player %d] Local player initialized.", local_player_index);
  return new_player;
}

// --- Get Local Player Position ---
SDL_FPoint funcGetPlayerPos()
{
  // Return position only if local player is initialized
  if (local_player_index != -1 && players_array[local_player_index] != NULL)
  {
    return players_array[local_player_index]->position;
  }
  return (SDL_FPoint){-1.0f, -1.0f};
}

// --- Get Local Player State for Network ---
bool get_local_player_state_for_network(PlayerStateData *out_data)
{
  // Check if local player exists and output pointer is valid
  if (local_player_index == -1 || players_array[local_player_index] == NULL || out_data == NULL)
  {
    return false;
  }

  // Populate the output struct with current state
  Player *local_player = players_array[local_player_index];
  out_data->client_id = (uint8_t)local_player->client_index; // Ensure correct ID is set
  out_data->position = local_player->position;
  out_data->sprite_portion = local_player->sprite_portion;
  out_data->flip_mode = local_player->flip_mode;
  return true;
}