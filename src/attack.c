#include "../include/attack.h"

typedef struct FireBall
{
    SDL_Texture *texture;
    SDL_FPoint src;
    SDL_FPoint dst;
    SDL_FRect target;
    int hit;
    float angle_deg;
    float velocity_x, velocity_y;
    int rotation_diff_x, rotation_diff_y;
} FireBall;

static FireBall fireBalls[MAX_FIREBALLS];
static int fireBallCount = 0;

bool renderFireBall(SDL_Renderer *renderer)
{
    // if (fireBall.src.x >= 360) fireBall.src.x = 0;                     // Render next sprite
    static Uint64 last_tick = 0, current_tick;
    static float delta_time;
    current_tick = SDL_GetTicks();
    delta_time = (current_tick - last_tick) / 1000.0f;
    last_tick = current_tick;
    int i = 0;

    while (i < fireBallCount)
    {
        if (fireBalls[i].hit)
        {
            SDL_DestroyTexture(fireBalls[i].texture);
            SDL_Log("HIT !!! \n");
            fireBalls[i] = fireBalls[fireBallCount - 1]; // Move the last ball to the removed one
            fireBallCount--;
            continue; //  Skip incrementing i and hanle the last ball
        }
        fireBalls[i].dst.x += fireBalls[i].velocity_x * delta_time;
        fireBalls[i].dst.y += fireBalls[i].velocity_y * delta_time;
        SDL_Log("dst.x: %f, dst.y: %f \n", fireBalls[i].dst.x, fireBalls[i].dst.y);
        if (fabsf(fireBalls[i].dst.x - fireBalls[i].target.x) < HIT_RANGE &&
            fabsf(fireBalls[i].dst.y - fireBalls[i].target.y) < HIT_RANGE)
        {
            fireBalls[i].hit = 1;
            continue; // Remove the ball
        }
        SDL_FRect srcrect = {fireBalls[i].src.x, fireBalls[i].src.y, FIREBALL_WIDTH, FIREBALL_HEIGHT};
        SDL_FRect dstrect = {fireBalls[i].dst.x + fireBalls[i].rotation_diff_x,
                             fireBalls[i].dst.y + fireBalls[i].rotation_diff_y,
                             FIREBALL_WIDTH,
                             FIREBALL_HEIGHT};
        SDL_RenderTextureRotated(renderer,
                                 fireBalls[i].texture,
                                 &srcrect,
                                 &dstrect,
                                 fireBalls[i].angle_deg,
                                 NULL,
                                 SDL_FLIP_NONE);

        i++;
    }
    return false;
}

void fireBallInit(SDL_Renderer *renderer, SDL_FPoint mousePos, SDL_FPoint player_position, SDL_FPoint scale_offset)
{
    if (fireBallCount >= MAX_FIREBALLS)
        return;
    FireBall *newFireBall = &fireBalls[fireBallCount++];
    newFireBall->src = (SDL_FPoint){0.0f, 0.0f};
    newFireBall->dst = (SDL_FPoint){player_position.x - camera.x - PLAYER_WIDTH / 4.0f,
                                    player_position.y - camera.y - PLAYER_HEIGHT / 2.0f};

    newFireBall->target = (SDL_FRect){mousePos.x / scale_offset.x,
                                      mousePos.y / scale_offset.y,
                                      100.0f,
                                      100.0f}; // Scaling down mouse coordinates to camera view size by 4

    // When shooting to left, adjust the diff which caused by >180° rotation
    if (newFireBall->dst.x > newFireBall->target.x)
    {
        newFireBall->rotation_diff_x = -20;
        newFireBall->rotation_diff_y = -25;
    }
    else
    {
        newFireBall->rotation_diff_x = 0;
        newFireBall->rotation_diff_y = 0;
    }

    float dx = (newFireBall->target.x) - (newFireBall->dst.x);
    float dy = (newFireBall->target.y) - (newFireBall->dst.y);

    // Normalize vector length to calculate velocity with right direction
    float length = sqrtf(dx * dx + dy * dy);
    newFireBall->velocity_x = (dx / length) * FIREBALL_SPEED;
    newFireBall->velocity_y = (dy / length) * FIREBALL_SPEED;

    // Count shooting angle in degrees
    float angle_rad = atan2f(dy, dx);
    newFireBall->angle_deg = angle_rad * (180.0f / M_PI);

    SDL_Log("TARGET x: %f, y: %f \n", newFireBall->target.x, newFireBall->target.y);
    newFireBall->hit = 0;
    char fireBallPath[] = "./resources/Sprites/Red_Team/Fire_Wizard/Charge.png";
    newFireBall->texture = IMG_LoadTexture(renderer, fireBallPath);

    if (!newFireBall->texture)
    {
        SDL_Log("Error loading texture for fire ball: %s\n", SDL_GetError());
        return;
    }
}