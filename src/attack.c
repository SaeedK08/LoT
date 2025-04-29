#include "../include/attack.h"

static SDL_Texture *fireball_texture;

FireBall fireBalls[MAX_FIREBALLS];
static int fireBallCount = 0;

static void cleanup()
{
    if (fireball_texture)
    {
        SDL_DestroyTexture(fireball_texture);
        fireball_texture = NULL;
        SDL_Log("Fireball texture cleaned up.");
    }
}

static void update(AppState *state)
{
    for (int i = fireBallCount - 1; i >= 0; i--)
    {
        if (fireBalls[i].active)
        {
            if (fireBalls[i].hit)
            {
                SDL_Log("HIT !!! \n");
                fireBalls[i].active = 0;

                if (i < fireBallCount - 1)
                {
                    fireBalls[i] = fireBalls[fireBallCount - 1];
                }

                memset(&fireBalls[fireBallCount - 1], 0, sizeof(FireBall));

                if (i < fireBallCount - 1)
                {
                    fireBalls[i].texture = fireball_texture;
                }

                fireBallCount--;
                continue;
            }

            fireBalls[i].dst.x += fireBalls[i].velocity_x * state->delta_time;
            fireBalls[i].dst.y += fireBalls[i].velocity_y * state->delta_time;

            // SDL_Log("dst.x: %f, dst.y: %f \n", fireBalls[i].dst.x, fireBalls[i].dst.y);

            // Boundary check
            if (fireBalls[i].dst.x < -FIREBALL_WIDTH * 2 || fireBalls[i].dst.x > CAMERA_VIEW_WIDTH + FIREBALL_WIDTH ||
                fireBalls[i].dst.y < -FIREBALL_HEIGHT * 2 || fireBalls[i].dst.y > CAMERA_VIEW_HEIGHT + FIREBALL_HEIGHT)
            {
                fireBalls[i].hit = 1;
                continue;
            }

            // Check for collision with target point using distance
            float dist_x = fireBalls[i].dst.x - fireBalls[i].target.x;
            float dist_y = fireBalls[i].dst.y - fireBalls[i].target.y;
            if (sqrtf(dist_x * dist_x + dist_y * dist_y) < HIT_RANGE)
            {
                fireBalls[i].hit = 1;
                continue;
            }
        }
    }
}

static void render(AppState *state)
{
    for (int i = 0; i < fireBallCount; i++)
    {
        if (fireBalls[i].active)
        {
            SDL_FRect srcrect = {fireBalls[i].src.x, fireBalls[i].src.y, FIREBALL_FRAME_WIDTH, FIREBALL_FRAME_HEIGHT};

            SDL_FRect dstrect = {fireBalls[i].dst.x + fireBalls[i].rotation_diff_x,
                                 fireBalls[i].dst.y + fireBalls[i].rotation_diff_y,
                                 FIREBALL_WIDTH,
                                 FIREBALL_HEIGHT};

            SDL_RenderTextureRotated(state->renderer,
                                     fireBalls[i].texture,
                                     &srcrect,
                                     &dstrect,
                                     fireBalls[i].angle_deg,
                                     NULL,
                                     SDL_FLIP_NONE);
        }
    }
}

void activate_fireballs(float player_pos_x, float player_pos_y, float cam_x, float cam_y, float mouse_view_x, float mouse_view_y)
{
    if (fireBallCount >= MAX_FIREBALLS)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Max fireballs reached, cannot fire.");
        return;
    }

    int index = fireBallCount;
    FireBall *newFireBall = &fireBalls[index];

    newFireBall->active = 1;
    newFireBall->hit = 0;
    newFireBall->src = (SDL_FPoint){0.0f, 0.0f};

    // Start position relative to camera view, adjusted for player offset
    newFireBall->dst.x = player_pos_x - cam_x - PLAYER_WIDTH / 4.0f;
    newFireBall->dst.y = player_pos_y - cam_y - PLAYER_HEIGHT / 2.0f;

    newFireBall->target = (SDL_FPoint){mouse_view_x, mouse_view_y};

    // Calculate direction vector from fireball start (dst) to target
    float dx = newFireBall->target.x - newFireBall->dst.x;
    float dy = newFireBall->target.y - newFireBall->dst.y;

    // Normalize vector length to calculate velocity with right direction
    float length = sqrtf(dx * dx + dy * dy);
    if (length > 0)
    {
        newFireBall->velocity_x = (dx / length) * FIREBALL_SPEED;
        newFireBall->velocity_y = (dy / length) * FIREBALL_SPEED;
    }
    else
    {
        // Handle case where click is exactly on player center
        newFireBall->velocity_x = 0;
        newFireBall->velocity_y = -FIREBALL_SPEED;
    }

    // Count shooting angle in degrees
    float angle_rad = atan2f(dy, dx);
    newFireBall->angle_deg = angle_rad * (180.0f / M_PI);

    // Adjust rotation pivot offset based on angle
    if (newFireBall->dst.x > newFireBall->target.x) // Facing left
    {
        newFireBall->rotation_diff_x = -20;
        newFireBall->rotation_diff_y = -25;
    }
    else // Facing right
    {
        newFireBall->rotation_diff_x = 0;
        newFireBall->rotation_diff_y = 0;
    }

    SDL_Log("TARGET x: %f, y: %f \n", newFireBall->target.x, newFireBall->target.y);

    fireBallCount++;
}

// Load texture once and create entity
SDL_AppResult init_fireball(SDL_Renderer *renderer)
{
    char path[] = "./resources/Sprites/Red_Team/Fire_Wizard/Charge.png";
    fireball_texture = IMG_LoadTexture(renderer, path);

    if (!fireball_texture)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[%s] Failed to load fireball texture '%s': %s", __func__, path, SDL_GetError());
        return SDL_APP_FAILURE;
    }

    memset(fireBalls, 0, sizeof(fireBalls));
    for (int i = 0; i < MAX_FIREBALLS; i++)
    {
        fireBalls[i].texture = fireball_texture;
        fireBalls[i].active = 0;
    }

    Entity fireball_entity = {
        .name = "fireball",
        .update = update,
        .render = render,
        .cleanup = cleanup};

    if (create_entity(fireball_entity) == SDL_APP_FAILURE)
    {
        // Cleanup texture if entity creation failed after loading
        SDL_DestroyTexture(fireball_texture);
        fireball_texture = NULL;
        return SDL_APP_FAILURE;
    }

    SDL_Log("Fireball entity initialized.");
    return SDL_APP_SUCCESS;
}