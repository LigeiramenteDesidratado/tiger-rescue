#include <SDL2/SDL_events.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include "defs.h"


typedef struct Entity Entity;

typedef struct {
    void (*logic)(void);
    void (*draw)(void);
} Delegate;

typedef struct Entity {
    float x;
    float y;
    int w;
    int h;
    float dx;
    float dy;
    int heath;
    int reload;
    SDL_Texture* texture;
    Entity* next;

} Entity;

typedef struct {
        unsigned int w;
        unsigned int h;
        const char* name;
        SDL_Window* window;
        SDL_Renderer* renderer;

} Screen;

typedef struct {
    SDL_Texture* (*load_texture)(const char* filename);
    void (*blit)(SDL_Texture *texture, int x, int y);
    void (*blitRect)(SDL_Texture* texture, SDL_Rect* src, int x, int y);

} Graphics;

typedef struct{
    int keyboard[MAX_KEYBOARD_KEYS];
    void (*do_input)(void);
    void (*do_key_up)(SDL_KeyboardEvent* event);
    void (*do_key_down)(SDL_KeyboardEvent* event);

} Input;


typedef struct Explosion Explosion;
typedef struct Explosion {
    float x;
    float y;
    float dx;
    float dy;
    int r, g, b, a;
    Explosion *next;
} Explosion;

typedef struct Debris Debris;
typedef struct Debris {
    float x;
    float y;
    float dx;
    float dy;
    SDL_Rect rect;
    SDL_Texture *texture;
    int life;
    Debris *next;
} Debris;

typedef struct {
    Entity playerHead, *playerTail;
    Entity playerBulletHead, *playerBulletTail;
    Entity enemyHead, *enemyTail;
    Entity enemyBulletHead, *enemyBulletTail;

    Explosion explosionHead, *explosionTail;
    Debris debrisHead, *debrisTail;
    void (*init_stage)(void);
    void (*reset_stage)(void);

} Stage;

typedef struct {
    int x;
    int y;
    int speed;

} Star;

