#ifndef ENTITY_H
#define ENTITY_H

#include <SDL3/SDL.h>

#define MAX_ENTITIES 100 

struct Entity
{
    void (*quit)(AppState*);
    void (*update)(AppState*);
    void (*update_player)(Player*);
    void (*render)(AppState*);
    void (*handle_events)(SDL_Event*);
};


typedef struct Entity Entity; 



#endif