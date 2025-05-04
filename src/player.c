#include "../include/player.h"

struct Player
{
  SDL_FPoint position;
  SDL_Texture *texture;
  SDL_Texture *blue_texture;
  SDL_Texture *red_texture;
  SDL_FRect sprite_portion;
  int movement_speed;
  SDL_FlipMode flip_mode;
  bool is_local_player;
  int client_index;
  // --- Animation state ---
  float anim_timer;
  int current_frame;
  bool is_moving;
  bool team;
};

// --- Static Variables ---
Player *players_array[MAX_CLIENTS]; // Global array holding pointers to player structs, indexed by client ID.
static int local_player_index = -1; // Index of the player controlled by this client instance, -1 if none.
// --- Static Helper Function Definitions ---

/**
 * @brief Cleans up resources associated with the local player entity.
 * @param void
 * @return void
 */
static void cleanup(void)
{
  // Only cleanup if the local player was actually initialized.
  if (local_player_index != -1 && players_array[local_player_index] != NULL)
  {
    // Destroy both team textures if they were loaded.
    if (players_array[local_player_index]->blue_texture)
    {
      SDL_DestroyTexture(players_array[local_player_index]->blue_texture);
      players_array[local_player_index]->blue_texture = NULL;
    }
    if (players_array[local_player_index]->red_texture)
    {
      SDL_DestroyTexture(players_array[local_player_index]->red_texture);
      players_array[local_player_index]->red_texture = NULL;
    }
    // The primary 'texture' pointer is just a reference to one of the above, no need to double-free.
    players_array[local_player_index]->texture = NULL;

    // Free the player struct itself.
    SDL_free(players_array[local_player_index]);
    players_array[local_player_index] = NULL; // Clear the pointer in the array.
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Player %d] Cleaned up.", local_player_index);
  }
  local_player_index = -1; // Reset the local player index.
}

/**
 * @brief Handles input events specifically for the local player.
 * @param appstate Pointer to the global application state.
 * @param event Pointer to the SDL_Event being processed.
 * @return void
 */
static void handle_events(void *appstate, SDL_Event *event)
{
  // Ignore events if the local player isn't initialized.
  if (local_player_index == -1 || players_array[local_player_index] == NULL)
    return;

  AppState *pState = (AppState *)(appstate);
  int window_w, window_h;
  float scale_x, scale_y;

  // Get window size and calculate scaling factors for mouse coordinates.
  SDL_GetWindowSize(pState->window, &window_w, &window_h);
  // Calculate scaling based on logical presentation size vs window size.
  scale_x = (CAMERA_VIEW_WIDTH > 0) ? (float)window_w / CAMERA_VIEW_WIDTH : 1.0f;
  scale_y = (CAMERA_VIEW_HEIGHT > 0) ? (float)window_h / CAMERA_VIEW_HEIGHT : 1.0f;

  // Check for left mouse button clicks.
  if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT)
  {
    // Convert window mouse coordinates to logical viewport coordinates.
    float mouse_view_x = event->button.x / scale_x;
    float mouse_view_y = event->button.y / scale_y;
    // Calculate player's position within the viewport.
    float player_view_x = players_array[local_player_index]->position.x - camera.x;
    float player_view_y = players_array[local_player_index]->position.y - camera.y;
    // Calculate distance between player and mouse click.
    float dist_x = mouse_view_x - player_view_x;
    float dist_y = mouse_view_y - player_view_y;
    float distance = sqrtf(dist_x * dist_x + dist_y * dist_y);

    // Trigger attack if click is within range.
    if (distance <= ATTACK_RANGE)
    {
      // Activate fireball originating from player world coords towards mouse viewport coords.
      activate_fireballs(players_array[local_player_index]->position.x, players_array[local_player_index]->position.y, camera.x, camera.y, mouse_view_x, mouse_view_y, players_array[local_player_index]->team);
    }
  }
}

/**
 * @brief Updates the local player's position based on current keyboard state.
 * @param p Pointer to the local Player struct.
 * @param delta_time Time elapsed since the last frame in seconds.
 * @return void
 */
static void handle_local_player_input(Player *p, float delta_time)
{
  const bool *keyboard_state = SDL_GetKeyboardState(NULL);
  p->is_moving = false; // Reset movement flag each frame.

  SDL_FPoint tempPos = (SDL_FPoint){p->position.x, p->position.y};

  // --- Vertical Movement ---
  if (keyboard_state[SDL_SCANCODE_W])
  {
    tempPos.y -= p->movement_speed * delta_time;
    p->is_moving = true;
  }
  if (keyboard_state[SDL_SCANCODE_S])
  {
    tempPos.y += p->movement_speed * delta_time;
    p->is_moving = true;
  }
  // --- Horizontal Movement & Flipping ---
  if (keyboard_state[SDL_SCANCODE_A])
  {
    tempPos.x -= p->movement_speed * delta_time;
    p->flip_mode = SDL_FLIP_HORIZONTAL; // Face left when moving left.
    p->is_moving = true;
  }
  if (keyboard_state[SDL_SCANCODE_D])
  {
    tempPos.x += p->movement_speed * delta_time;
    p->flip_mode = SDL_FLIP_NONE; // Face right when moving right.
    p->is_moving = true;
  }

  // Create Rect of the player
  SDL_FRect player_bounds = {
      tempPos.x - PLAYER_WIDTH / 2.0f,
      tempPos.y - PLAYER_HEIGHT / 2.0f,
      PLAYER_WIDTH,
      PLAYER_HEIGHT};

  bool collision = false;

  // Check if the player rect intersects base or tower rect
  for (int i = 0; i < MAX_TOWERS; i++)
  {
    SDL_FRect tower_bounds = getTowerPos(i);
    if (SDL_HasRectIntersectionFloat(&player_bounds, &tower_bounds))
    {
      collision = true;
    };
  }
  for (int i = 0; i < MAX_BASES; i++)
  {
    SDL_FRect tower_bounds = getBasePos(i);
    if (SDL_HasRectIntersectionFloat(&player_bounds, &tower_bounds))
    {
      collision = true;
    };
  }

  if (!collision) // If player doesn't intersect, update position
  {
    p->position.y = tempPos.y;
    p->position.x = tempPos.x;
  }
}

/**
 * @brief Clamps the local player's position within the defined map boundaries.
 * @param p Pointer to the local Player struct.
 * @return void
 */
static void update_local_player_position(Player *p)
{
  // Prevent player from moving outside the map horizontally.
  p->position.x = fmaxf(PLAYER_WIDTH / 2.0f, fminf(p->position.x, MAP_WIDTH - PLAYER_WIDTH / 2.0f));
  // Prevent player from moving outside the map vertically.
  p->position.y = fmaxf(CLIFF_BOUNDARY, fminf(p->position.y, WATER_BOUNDARY));
}

/**
 * @brief Updates the local player's animation frame based on movement state.
 * @param p Pointer to the local Player struct.
 * @param delta_time Time elapsed since the last frame in seconds.
 * @return void
 */
static void update_local_player_animation(Player *p, float delta_time)
{
  // Determine which row of the spritesheet to use based on movement.
  float target_row_y = p->is_moving ? WALK_ROW_Y : IDLE_ROW_Y;
  int num_frames = p->is_moving ? NUM_WALK_FRAMES : NUM_IDLE_FRAMES;

  // Reset animation if the movement state (and thus target row) changes.
  if (p->sprite_portion.y != target_row_y)
  {
    p->current_frame = 0;
    p->anim_timer = 0.0f;
    p->sprite_portion.y = target_row_y;
  }

  // Advance animation timer.
  p->anim_timer += delta_time;
  // If enough time has passed, move to the next frame.
  if (p->anim_timer >= TIME_PER_FRAME)
  {
    p->anim_timer -= TIME_PER_FRAME;                        // Subtract interval, don't just reset to 0.
    p->current_frame = (p->current_frame + 1) % num_frames; // Loop back to frame 0 after the last frame.
  }

  // Update the source rectangle for rendering based on the current frame.
  p->sprite_portion.x = p->current_frame * FRAME_WIDTH;
  p->sprite_portion.w = FRAME_WIDTH;
  p->sprite_portion.h = FRAME_HEIGHT;
}

/**
 * @brief Updates the state of the local player entity.
 * @param state Pointer to the global application state.
 * @return void
 */
static void update(AppState *state)
{
  // Only update if the local player is initialized.
  if (local_player_index == -1 || players_array[local_player_index] == NULL)
    return;

  Player *local_player = players_array[local_player_index];

  // --- Process Local Player Updates ---
  handle_local_player_input(local_player, state->delta_time);
  update_local_player_position(local_player);
  update_local_player_animation(local_player, state->delta_time);
}

/**
 * @brief Renders the local player's sprite.
 * @param state Pointer to the global application state.
 * @param p Pointer to the local Player struct.
 * @return void
 */
static void render_local_player(AppState *state, Player *p)
{
  // Ensure player and texture are valid before rendering.
  if (!p || !p->texture)
    return;

  // Calculate screen coordinates by subtracting camera position and offsetting by half player size for centering.
  float screen_x = p->position.x - camera.x - PLAYER_WIDTH / 2.0f;
  float screen_y = p->position.y - camera.y - PLAYER_HEIGHT / 2.0f;
  SDL_FRect player_dest_rect = {screen_x, screen_y, PLAYER_WIDTH, PLAYER_HEIGHT};

  // Render the current frame of the player sprite with appropriate flipping.
  SDL_RenderTextureRotated(state->renderer,
                           p->texture,
                           &p->sprite_portion, // Source rect from animation state.
                           &player_dest_rect,  // Destination rect on screen.
                           0.0,                // No rotation angle.
                           NULL,               // Use center point for rotation (if any).
                           p->flip_mode);      // Horizontal flip state.
}

/**
 * @brief Renders all active remote players based on received network state.
 * @param state Pointer to the global application state.
 * @return void
 */
static void render_remote_players(AppState *state)
{
  // Need the local player's struct to access the loaded textures.
  if (local_player_index == -1 || players_array[local_player_index] == NULL)
  {
    return;
  }
  // Get pointers to both team textures loaded by the local client.
  SDL_Texture *blue_texture = players_array[local_player_index]->blue_texture;
  SDL_Texture *red_texture = players_array[local_player_index]->red_texture;

  // --- Iterate Through Remote Player Slots ---
  for (int i = 0; i < MAX_CLIENTS; ++i)
  {
    // Skip rendering the local player and any inactive slots.
    if (i == local_player_index || !remotePlayers[i].active)
      continue;

    // --- Select Texture Based on Remote Player's Team ---
    SDL_Texture *texture_to_use = NULL;
    // Use the team ID received over the network to choose which locally loaded texture to use.
    if (remotePlayers[i].team == BLUE_TEAM && blue_texture)
    {
      texture_to_use = blue_texture;
    }
    else if (remotePlayers[i].team == RED_TEAM && red_texture)
    {
      texture_to_use = red_texture;
    }
    else
    {
      // Log a warning and skip if the required texture isn't available.
      SDL_LogWarn(SDL_LOG_CATEGORY_RENDER, "[Player] Missing texture for remote player %d (team %d)", i, remotePlayers[i].team);
      continue;
    }
    // --- End Select Texture ---

    // Calculate screen position based on network data and camera offset.
    float screen_x = remotePlayers[i].position.x - camera.x - PLAYER_WIDTH / 2.0f;
    float screen_y = remotePlayers[i].position.y - camera.y - PLAYER_HEIGHT / 2.0f;
    SDL_FRect remote_player_dest_rect = {screen_x, screen_y, PLAYER_WIDTH, PLAYER_HEIGHT};

    // Use sprite portion and flip mode received directly from network data.
    SDL_FRect remote_sprite_portion = remotePlayers[i].sprite_portion;
    SDL_FlipMode remote_flip_mode = remotePlayers[i].flip_mode;

    // Render the remote player using the selected texture and their state.
    SDL_RenderTextureRotated(state->renderer,
                             texture_to_use, // Use the texture corresponding to the remote player's team.
                             &remote_sprite_portion,
                             &remote_player_dest_rect,
                             0.0,  // No rotation angle.
                             NULL, // Use center point for rotation (if any).
                             remote_flip_mode);
  }
}

/**
 * @brief Renders the player entity, including both local and remote players.
 * @param state Pointer to the global application state.
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

Player *init_player(SDL_Renderer *renderer, int assigned_player_index, bool team_arg)
{
  // --- Input Validation ---
  if (assigned_player_index < 0 || assigned_player_index >= MAX_CLIENTS)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Player] Invalid index %d.", assigned_player_index);
    return NULL;
  }
  // Prevent re-initialization unless it's the local player getting the correct team ID assigned.
  if (players_array[assigned_player_index] != NULL)
  {
    if (assigned_player_index != local_player_index || players_array[assigned_player_index]->team != team_arg)
    {
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Player] Attempt to re-initialize player index %d.", assigned_player_index);
    }
    // Ensure the local player struct reflects the assigned team ID.
    if (assigned_player_index == local_player_index)
    {
      players_array[assigned_player_index]->team = team_arg;
      // Update the primary texture pointer based on the finalized team ID.
      players_array[assigned_player_index]->texture = (team_arg == BLUE_TEAM) ? players_array[assigned_player_index]->blue_texture : players_array[assigned_player_index]->red_texture;
    }
    return players_array[assigned_player_index];
  }

  local_player_index = assigned_player_index; // Store the index for this client instance.

  // --- Allocate Player Struct ---
  Player *new_player = SDL_malloc(sizeof(Player));
  if (!new_player)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Player %d] Malloc failed.", local_player_index);
    local_player_index = -1;
    return NULL;
  }
  memset(new_player, 0, sizeof(Player)); // Initialize struct members to zero/NULL.

  // --- Load BOTH Team Textures ---
  // Load blue team texture.
  new_player->blue_texture = IMG_LoadTexture(renderer, BLUE_WIZARD_PATH);
  if (!new_player->blue_texture)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Player %d] Failed load texture '%s': %s", local_player_index, BLUE_WIZARD_PATH, SDL_GetError());
    SDL_free(new_player);
    local_player_index = -1;
    return NULL;
  }
  SDL_SetTextureScaleMode(new_player->blue_texture, SDL_SCALEMODE_NEAREST); // Use nearest neighbor scaling for pixel art.

  // Load red team texture.
  new_player->red_texture = IMG_LoadTexture(renderer, RED_WIZARD_PATH);
  if (!new_player->red_texture)
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Player %d] Failed load texture '%s': %s", local_player_index, RED_WIZARD_PATH, SDL_GetError());
    SDL_DestroyTexture(new_player->blue_texture); // Clean up the already loaded blue texture.
    SDL_free(new_player);
    local_player_index = -1;
    return NULL;
  }
  SDL_SetTextureScaleMode(new_player->red_texture, SDL_SCALEMODE_NEAREST);

  // --- Set Initial Player State ---
  new_player->team = team_arg;
  // Set the primary 'texture' pointer to the correct texture based on the assigned team.
  new_player->texture = (team_arg == BLUE_TEAM) ? new_player->blue_texture : new_player->red_texture;
  new_player->sprite_portion = (SDL_FRect){0.0f, IDLE_ROW_Y, FRAME_WIDTH, FRAME_HEIGHT}; // Start with the first idle frame.

  // --- Set Team-Based Starting Position ---
  if (team_arg == BLUE_TEAM)
  {
    new_player->position = (SDL_FPoint){350.0f, 850.0f}; // Position near blue base.
  }
  else
  {
    new_player->position = (SDL_FPoint){2850.0f, 850.0f}; // Position near red base.
  }

  new_player->movement_speed = 500;
  // Set initial facing direction based on team (towards center of map).
  new_player->flip_mode = (team_arg == BLUE_TEAM) ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
  new_player->is_local_player = true;
  new_player->client_index = local_player_index;
  new_player->anim_timer = 0.0f;
  new_player->current_frame = 0;
  new_player->is_moving = false;

  players_array[local_player_index] = new_player; // Store the pointer in the global array.

  // --- Create Player Entity (if it doesn't exist yet) ---
  // This ensures the entity system (update, render, etc.) is only registered once.
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
      // Cleanup allocated player resources if entity creation fails.
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Player %d] Failed create entity.", local_player_index);
      cleanup();
      return NULL;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Player %d] Player entity created.", local_player_index);
  }

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Player %d] Local player initialized for team %d.", local_player_index, team_arg);
  return new_player;
}

SDL_FPoint funcGetPlayerPos()
{
  // Return a valid position only if the local player is initialized.
  if (local_player_index != -1 && players_array[local_player_index] != NULL)
  {
    return players_array[local_player_index]->position;
  }
  // Return an invalid position marker if not initialized.
  return (SDL_FPoint){-1.0f, -1.0f};
}

bool get_local_player_state_for_network(PlayerStateData *out_data)
{
  // Check for invalid pointers or uninitialized local player.
  if (local_player_index == -1 || players_array[local_player_index] == NULL || out_data == NULL)
  {
    return false;
  }

  // Copy current state from the local player struct to the output struct.
  Player *local_player = players_array[local_player_index];
  out_data->client_id = (uint8_t)local_player->client_index;
  out_data->team = local_player->team;
  out_data->position = local_player->position;
  out_data->sprite_portion = local_player->sprite_portion;
  out_data->flip_mode = local_player->flip_mode;

  return true;
}