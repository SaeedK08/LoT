#include "../include/attack.h"

// --- Static Variables ---

static SDL_Texture *fireball_texture = NULL;
FireBall fireBalls[MAX_FIREBALLS];
static int fireBallCount = 0;

// --- Static Helper Functions ---

/**
 * @brief Cleans up resources used by the fireball system, specifically the texture.
 * @param void
 * @return void
 */
static void cleanup()
{
    if (fireball_texture)
    {
        SDL_DestroyTexture(fireball_texture);
        fireball_texture = NULL;
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Attack] Fireball texture cleaned up.");
    }
    fireBallCount = 0; // Reset count on cleanup
}

/**
 * @brief Updates the state of all active fireballs.
 * @param state Pointer to the global application state, containing delta_time.
 * @return void
 */
static void update(AppState *state)
{
    // Iterate backwards to allow safe removal of elements during iteration.
    for (int i = fireBallCount - 1; i >= 0; i--)
    {
        if (fireBalls[i].active)
        {
            // Check if fireball was marked for removal in the previous frame.
            if (fireBalls[i].hit)
            {
                fireBalls[i].active = 0;

                // Swap with the last active fireball for efficient removal (avoids shifting elements).
                if (i < fireBallCount - 1)
                {
                    fireBalls[i] = fireBalls[fireBallCount - 1];
                    // Ensure the swapped fireball still points to the correct texture.
                    fireBalls[i].texture = fireball_texture;
                }
                // Clear the now-unused last slot
                memset(&fireBalls[fireBallCount - 1], 0, sizeof(FireBall));

                fireBallCount--;
                continue; // Skip further processing for this slot.
            }

            // --- Position Update ---
            fireBalls[i].dst.x += fireBalls[i].velocity_x * state->delta_time;
            fireBalls[i].dst.y += fireBalls[i].velocity_y * state->delta_time;

            // --- Boundary Check ---
            // Mark for removal if the fireball goes too far off-screen.
            if (fireBalls[i].dst.x < -FIREBALL_WIDTH * 2 || fireBalls[i].dst.x > CAMERA_VIEW_WIDTH + FIREBALL_WIDTH ||
                fireBalls[i].dst.y < -FIREBALL_HEIGHT * 2 || fireBalls[i].dst.y > CAMERA_VIEW_HEIGHT + FIREBALL_HEIGHT)
            {
                fireBalls[i].hit = 1; // Mark for removal in the next frame.
                continue;
            }

            // --- Collision Check ---
            // Use distance squared for efficiency, avoiding sqrt calculation unless necessary.
            float dist_x = fireBalls[i].dst.x - fireBalls[i].target.x;
            float dist_y = fireBalls[i].dst.y - fireBalls[i].target.y;
            if (sqrtf(dist_x * dist_x + dist_y * dist_y) < HIT_RANGE)
            {
                fireBalls[i].hit = 1; // Mark for removal in the next frame.
                if (SDL_PointInRectFloat(&fireBalls[i].dst, &fireBalls[i].attackable_tower1))
                {
                    damageTower(fireBalls[i].attackable_tower1.x);
                }
                if (SDL_PointInRectFloat(&fireBalls[i].dst, &fireBalls[i].attackable_tower2))
                {
                    damageTower(fireBalls[i].attackable_tower2.x);
                }
                if (SDL_PointInRectFloat(&fireBalls[i].dst, &fireBalls[i].attackable_base))
                {
                    SDL_Log("Base is getting attack");
                    damageBase(fireBalls[i].attackable_base.x);
                }

                continue;
            }
        }
    }
}

/**
 * @brief Renders all active fireballs.
 * @param state Pointer to the global application state, containing the renderer.
 * @return void
 */
static void render(AppState *state)
{
    if (!fireball_texture)
    {
        return; // Don't try to render if the texture hasn't been loaded.
    }

    for (int i = 0; i < fireBallCount; i++) // Iterate only up to the active count.
    {
        if (fireBalls[i].active)
        {
            // Define source rect from spritesheet (currently using whole texture).
            SDL_FRect srcrect = {fireBalls[i].src.x, fireBalls[i].src.y, FIREBALL_FRAME_WIDTH, FIREBALL_FRAME_HEIGHT};

            // Define destination rect on screen, adjusted for rotation offset if needed.
            SDL_FRect dstrect = {fireBalls[i].dst.x + fireBalls[i].rotation_diff_x,
                                 fireBalls[i].dst.y + fireBalls[i].rotation_diff_y,
                                 FIREBALL_WIDTH,
                                 FIREBALL_HEIGHT};

            SDL_RenderTextureRotated(state->renderer,
                                     fireBalls[i].texture,
                                     &srcrect,
                                     &dstrect,
                                     fireBalls[i].angle_deg,
                                     NULL, // Rotation center (NULL uses dstrect center).
                                     SDL_FLIP_NONE);
        }
    }
}

// --- Public API Function Implementations ---

void activate_fireballs(float player_pos_x, float player_pos_y, float cam_x, float cam_y, float mouse_view_x, float mouse_view_y, bool team)
{
    if (fireBallCount >= MAX_FIREBALLS)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[Attack] Max fireballs (%d) reached.", MAX_FIREBALLS);
        return;
    }

    int index = fireBallCount; // Use the current count as the index for the new fireball.
    FireBall *newFireBall = &fireBalls[index];

    // --- Initialize New Fireball ---
    newFireBall->active = 1;
    newFireBall->hit = 0;
    newFireBall->src = (SDL_FPoint){0.0f, 0.0f}; // Assuming sprite starts at 0,0 in the texture.

    // Start position relative to camera view (adjust offset from player center).
    newFireBall->dst.x = player_pos_x - cam_x - PLAYER_WIDTH / 4.0f;
    newFireBall->dst.y = player_pos_y - cam_y - PLAYER_HEIGHT / 2.0f;

    newFireBall->target = (SDL_FPoint){mouse_view_x, mouse_view_y};

    // --- Calculate Direction and Velocity ---
    float dx = newFireBall->target.x - newFireBall->dst.x;
    float dy = newFireBall->target.y - newFireBall->dst.y;
    float length = sqrtf(dx * dx + dy * dy);

    if (length > 0.001f) // Avoid division by zero or near-zero.
    {
        // Normalize direction vector and multiply by speed.
        newFireBall->velocity_x = (dx / length) * FIREBALL_SPEED;
        newFireBall->velocity_y = (dy / length) * FIREBALL_SPEED;
        // Calculate angle from direction vector.
        float angle_rad = atan2f(dy, dx);
        newFireBall->angle_deg = angle_rad * (180.0f / (float)M_PI);
    }
    else
    {
        // Default velocity (e.g., straight up) if click is exactly on the player spawn point.
        newFireBall->velocity_x = 0;
        newFireBall->velocity_y = -FIREBALL_SPEED;
        newFireBall->angle_deg = -90.0f;
    }

    newFireBall->texture = fireball_texture; // Assign the shared texture.

    // Reset rotation offset (not currently used).
    newFireBall->rotation_diff_x = 0;
    newFireBall->rotation_diff_y = 0;

    // --- Decide what towers a player can attack ---
    if (team)
    {
        newFireBall->attackable_tower1 = (SDL_FRect){2400.0f - camera.x - TOWER_WIDTH / 2.0f,
                                                     BUILDINGS_POS_Y - camera.y - TOWER_HEIGHT / 2.0f,
                                                     TOWER_WIDTH,
                                                     TOWER_HEIGHT};
        newFireBall->attackable_tower2 = (SDL_FRect){2700.0f - camera.x - TOWER_WIDTH / 2.0f,
                                                     BUILDINGS_POS_Y - camera.y - TOWER_HEIGHT / 2.0f,
                                                     TOWER_WIDTH,
                                                     TOWER_HEIGHT};
        newFireBall->attackable_base = (SDL_FRect){RED_BASE_POS_X - camera.x - BASE_WIDTH / 2.0f,
                                                   BUILDINGS_POS_Y - camera.y - BASE_HEIGHT / 2.0f,
                                                   BASE_WIDTH,
                                                   BASE_HEIGHT};
    }
    else
    {
        newFireBall->attackable_tower1 = (SDL_FRect){500.0f - camera.x - TOWER_WIDTH / 2.0f,
                                                     BUILDINGS_POS_Y - camera.y - TOWER_HEIGHT / 2.0f,
                                                     TOWER_WIDTH,
                                                     TOWER_HEIGHT};
        newFireBall->attackable_tower2 = (SDL_FRect){800.0f - camera.x - TOWER_WIDTH / 2.0f,
                                                     BUILDINGS_POS_Y - camera.y - TOWER_HEIGHT / 2.0f,
                                                     TOWER_WIDTH,
                                                     TOWER_HEIGHT};
        newFireBall->attackable_base = (SDL_FRect){BLUE_BASE_POS_X - camera.x - BASE_WIDTH / 2.0f,
                                                   BUILDINGS_POS_Y - camera.y - BASE_HEIGHT / 2.0f,
                                                   BASE_WIDTH,
                                                   BASE_HEIGHT};
    }

    fireBallCount++; // Increment active count *after* successful initialization.
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "[Attack] Fired fireball %d.", index);
}

SDL_AppResult init_fireball(SDL_Renderer *renderer, bool team_arg)
{
    // --- Load Texture ---
    // Load the texture only once if it hasn't been loaded yet.
    if (fireball_texture == NULL)
    {
        const char path[] = "./resources/Sprites/Red_Team/Fire_Wizard/Charge.png";
        fireball_texture = IMG_LoadTexture(renderer, path);
        if (!fireball_texture)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Attack] Failed load texture '%s': %s", path, SDL_GetError());
            return SDL_APP_FAILURE;
        }
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Attack] Fireball texture loaded.");
    }

    // --- Initialize Fireball Pool ---
    memset(fireBalls, 0, sizeof(fireBalls));
    fireBallCount = 0;

    for (int i = 0; i < MAX_FIREBALLS; ++i)
    {
        fireBalls[i].texture = fireball_texture; // Pre-assign shared texture pointer.
        fireBalls[i].active = 0;                 // Mark all as inactive initially.
    }

    // --- Register Entity ---
    // Register the fireball entity system if it hasn't been registered yet.
    if (find_entity("fireball") == -1)
    {
        Entity fireball_entity = {
            .name = "fireball",
            .update = update,
            .render = render,
            .cleanup = cleanup,
            .handle_events = NULL // Fireballs don't handle direct input events.
        };

        if (create_entity(fireball_entity) == SDL_APP_FAILURE)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Attack] Failed create entity.");
            cleanup(); // Clean up loaded texture if entity registration fails.
            return SDL_APP_FAILURE;
        }
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Attack] Fireball entity created.");
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "[Attack] Fireball system initialized.");
    return SDL_APP_SUCCESS;
}