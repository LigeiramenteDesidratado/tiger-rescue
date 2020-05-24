#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define SDL_MAIN_HANDLEd
#include "SDL2/SDL.h"

// Game variables related
#include "defs.h"


// Declarations
void game_init(void);
void do_input(void);
void prepare_scene(void);
void present_scene(void);
void game_quit(void);

void init_stage(void);
static void init_player(void);

SDL_Texture* load_terxture(const char* filename);
void blit(SDL_Texture *texture, int x, int y);

void do_key_up(SDL_KeyboardEvent* event);
void do_key_down(SDL_KeyboardEvent* event);

static void logic(void);
static void do_player(void);
static void do_bullets(void);
static void do_enemies(void);
static void spawn_enemy(void);
static void fire_bullet(void);
static void draw(void);
static void draw_player(void);
static void draw_bullets(void);
static void draw_enemy(void);
static void capFrameRate(long *then, float *remainder);


// Temp
SDL_Texture* gBulletTexture;
SDL_Texture* gEnemyTexture;
int enemySpawnTimer;

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
    struct Entity* next;

    void (*do_entity)(void);

} Entity;

typedef struct {
        unsigned int w;
        unsigned int h;
        const char* name;
        SDL_Window* window;
        SDL_Renderer* renderer;

} Screen;

typedef struct {
    SDL_Texture* (*load_terxture)(const char* filename);
    void (*blit)(SDL_Texture *texture, int x, int y);

} Graphics;

typedef struct{
    int keyboard[MAX_KEYBOARD_KEYS];
    void (*do_input)(void);
    void (*do_key_up)(SDL_KeyboardEvent* event);
    void (*do_key_down)(SDL_KeyboardEvent* event);

} Input;

typedef struct {
    Entity playerHead, *playerTail;
    Entity bulletHead, *bulletTail;
    Entity enemyHead, *enemyTail;
    void (*init_stage)(void);

} Stage;

static struct {
    // Define attributes
    SDL_bool running;

    // All window related
    Screen* screen;

    // All graphics related
    Graphics* graphics;

    Delegate* delegate;

    // All input related
    Input* input;

    // All input related
    Stage* stage;

    //All entities related
    struct {
        Entity* player;
        Entity* bullet;
        Entity* enemy;

    } entities;

    // Define "methods"
    void (*init)(void);
    void (*prepare_scene)(void);
    void (*present_scene)(void);
    void (*quit)(void);

} Game =  {
    SDL_FALSE,

    // Screen
    .screen = &(Screen) {
        .w = SCREEN_SCALE*SCREEN_W,
        .h = SCREEN_SCALE*SCREEN_H,
        .name = SCREEN_NAME,
        .window = NULL,
        .renderer = NULL
    },

    // Graphics
    .graphics = &(Graphics) {
        load_terxture,
        blit
    },

    .delegate = &(Delegate) {
        logic,
        draw,
    },

    // Input
    .input = &(Input) {
        .keyboard = {0},
        do_input,
        do_key_up,
        do_key_down
    },

    // Stage
    .stage = &(Stage) {
        .playerHead =  {},
        .playerTail = NULL,
        .bulletHead =  {},
        .bulletTail = NULL,
        .enemyHead =  {},
        .enemyTail = NULL,

        init_stage,
    },

    .entities = {
        .player = &(Entity) {.do_entity = do_player},
        .bullet = &(Entity) {.do_entity = do_bullets},
        .enemy = &(Entity) {.do_entity = do_enemies}
    },

    game_init,
    prepare_scene,
    present_scene,
    game_quit

};



void game_init(void) {

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("Failed to initialize SDL! SDL Error %s\n", SDL_GetError());
        exit(1);
    };

    unsigned int w = Game.screen->w;
    unsigned int h = Game.screen->h;
    const char* name = Game.screen->name;

    Game.screen->window = SDL_CreateWindow(
        name,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        w, h, 0
    );
    if (!Game.screen->window) {
        printf("Failed to open %d x %d window: %s\n", SCREEN_W, SCREEN_H, SDL_GetError());
        exit(1);
    }

    Game.screen->renderer = SDL_CreateRenderer(
        Game.screen->window,
        -1,
        SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC
    );
    if (!Game.screen->renderer) {
        printf("Failed to create renerer: %s\n", SDL_GetError());
        exit(1);
    }

    Game.running = SDL_TRUE;
}

void game_quit(void) {


    SDL_DestroyTexture(gBulletTexture);
    gBulletTexture = NULL;

    SDL_DestroyTexture(gEnemyTexture);
    gEnemyTexture = NULL;

    SDL_DestroyRenderer(Game.screen->renderer);
    Game.screen->renderer = NULL;

    SDL_DestroyWindow(Game.screen->window);
    Game.screen->window = NULL;

    SDL_Quit();
    Game.running = SDL_FALSE;
}

void do_input(void) {

    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        switch (e.type) {
            case SDL_QUIT:
                Game.running = SDL_FALSE;
                break;
            case SDL_KEYUP:
                Game.input->do_key_up(&e.key);
                break;
            case SDL_KEYDOWN:
                Game.input->do_key_down(&e.key);
                break;
            default:
                break;
        }

    }
}

void init_stage(void) {

    Game.stage->playerTail = &Game.stage->playerHead;
    Game.stage->bulletTail = &Game.stage->bulletHead;
    Game.stage->enemyTail = &Game.stage->enemyHead;

    init_player();

    gBulletTexture = Game.graphics->load_terxture("gfx/playerBullet.png");
    if (gBulletTexture == NULL) {
        printf("Failed to load bullet texture! SDL Error %s\n", SDL_GetError());
        exit(1);
    }

    gEnemyTexture = Game.graphics->load_terxture("gfx/enemy.png");
    if (gEnemyTexture == NULL) {
        printf("Failed to load enemy texture! SDL Error %s\n", SDL_GetError());
        exit(1);
    }

    enemySpawnTimer = 0;

}

static void init_player(void) {
    Game.stage->playerTail->next = Game.entities.player;
    Game.stage->playerTail = Game.entities.player;

    Game.entities.player->x = 100;
    Game.entities.player->y = 100;

    Game.entities.player->texture = Game.graphics->load_terxture("gfx/player.png");
    if (Game.entities.player->texture == NULL) {
        printf("Failed to load player texture! SDL Error %s\n", SDL_GetError());
        exit(1);
    }

    // Query texture w and h and set it on player
    SDL_QueryTexture(
        Game.entities.player->texture,
        NULL,
        NULL,
        &Game.entities.player->w,
        &Game.entities.player->h
    );
}

static void logic(void) {
    do_player();

    do_enemies();

    do_bullets();

    spawn_enemy();
}

static void do_player(void) {
    // Alias
    Entity* player = Game.entities.player;

    player->dx = player->dy = 0;

    if (player->reload > 0) {
        player->reload--;
    }

    if (Game.input->keyboard[SDL_SCANCODE_K]) {
        if (player->y > 0)
            player->dy = -PLAYER_SPEED;
    }

    if (Game.input->keyboard[SDL_SCANCODE_J]) {
        if (player->y < SCREEN_H - player->h)
            player->dy = PLAYER_SPEED;
    }

    if (Game.input->keyboard[SDL_SCANCODE_H]) {
        if (player->x > 0)
            player->dx = -PLAYER_SPEED;
    }

    if (Game.input->keyboard[SDL_SCANCODE_L]) {
        if (player->x < SCREEN_W - player->w)
            player->dx = PLAYER_SPEED;
    }

    if (Game.input->keyboard[SDL_SCANCODE_F] && player->reload == 0) {
        fire_bullet();
    }

    player->x += player->dx;
    player->y += player->dy;
}

static void do_enemies(void) {

    Entity *e, *prev;

    prev = &Game.stage->enemyHead;

    for (e = Game.stage->enemyHead.next; e != NULL; e = e->next) {
        e->x += e->dx;
        e->y += e->dy;

        if (e->x < -e->w) {
            if (e == Game.stage->enemyTail) {
                Game.stage->enemyTail = prev;
            }
            prev->next = e->next;
            free(e);
            e = prev;
        }

        prev = e;
    }

}

static void spawn_enemy(void) {

    Game.entities.enemy = malloc(sizeof(Entity));
    memset(Game.entities.enemy, 0, sizeof(Entity));

    Entity* enemy = Game.entities.enemy;
    if (--enemySpawnTimer <= 0) {
        Game.stage->enemyTail->next = enemy;
        Game.stage->enemyTail = enemy;

        enemy->texture = gEnemyTexture;
        SDL_QueryTexture(enemy->texture, NULL, NULL, &enemy->w, &enemy->h);
        enemy->x = SCREEN_W;
        enemy->y = rand() % (SCREEN_H - enemy->h);

        enemy->dx = -(2 +(rand() % 4));
        enemySpawnTimer = 30 + (rand()%60);

    }
}

static void draw_enemy(void) {
    Entity *e;

    for (e = Game.stage->enemyHead.next; e != NULL; e = e->next) {
        Game.graphics->blit(e->texture, e->x, e->y);
    }
}

static void do_bullets(void) {
    Entity *b, *prev;

    prev = &Game.stage->bulletHead;

    for (b = Game.stage->bulletHead.next; b != NULL; b = b->next) {
        b->x += b->dx;
        b->y += b->dy;

        if (b->x > SCREEN_W) {
            if (b == Game.stage->bulletTail) {
                Game.stage->bulletTail = prev;
            }
            prev->next = b->next;
            free(b);
            b = prev;
        }

        prev = b;
    }
}

static void fire_bullet(void) {

    Game.entities.bullet = malloc(sizeof(Entity));
    memset(Game.entities.bullet, 0, sizeof(Entity));

    Entity* bullet = Game.entities.bullet;
    Entity* player = Game.entities.player;

    Game.stage->bulletTail->next = bullet;
    Game.stage->bulletTail = bullet;

    // set initial position of the bullet
    bullet->x = player->x;
    bullet->y = player->y;

    bullet->dx = PLAYER_BULLET_SPEED;
    bullet->heath = 1;
    bullet->texture = gBulletTexture;
    SDL_QueryTexture(bullet->texture, NULL, NULL, &bullet->w, &bullet->h);

    bullet->y += (player->h / 2) - (bullet->h / 2);

    player->reload = 8;

}

// present_scene will clear the screen and set the background color
void prepare_scene(void) {
    SDL_SetRenderDrawColor(Game.screen->renderer, 0x12, 0x12, 0x12, 0xFF);
    SDL_RenderClear(Game.screen->renderer);
}

void present_scene(void) {
    SDL_RenderPresent(Game.screen->renderer);
}

SDL_Texture* load_terxture(const char* filename) {
    SDL_Texture* texture;
    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "Loading %s", filename);

    texture = IMG_LoadTexture(Game.screen->renderer, filename);
    if (texture == NULL) {
        printf("Failed to load texture %s! SDL Error: %s\n", filename, SDL_GetError());
        exit(1);
    }

    return texture;
}

// The blit function simply draws the specified texture on screen
// at specified x and y coordinates
void blit(SDL_Texture *texture, int x, int y) {
    SDL_Rect dest;

    dest.x = x;
    dest.y = y;
    SDL_QueryTexture(texture, NULL, NULL, &dest.w, &dest.h);

    SDL_RenderCopy(Game.screen->renderer, texture, NULL, &dest);
}

void do_key_up(SDL_KeyboardEvent* event) {
    // check if the keyboard event was a result of  Keyboard repeat event
    if (event->repeat == 0 && event->keysym.scancode < MAX_KEYBOARD_KEYS) {
        Game.input->keyboard[event->keysym.scancode] = 0;
    }
}

void do_key_down(SDL_KeyboardEvent* event) {
    // check if the keyboard event was a result of  Keyboard repeat event
    if (event->repeat == 0 && event->keysym.scancode < MAX_KEYBOARD_KEYS) {
        Game.input->keyboard[event->keysym.scancode] = 1;
    }
}

static void draw(void) {
    draw_player();
    draw_bullets();
    draw_enemy();
}

static void draw_player(void) {
    Entity* player = Game.entities.player;
    Game.graphics->blit(player->texture, player->x, player->y);
}

static void draw_bullets(void) {
    Entity *b;

    for (b = Game.stage->bulletHead.next; b != NULL; b = b->next) {
        Game.graphics->blit(b->texture, b->x, b->y);
    }
}

static void capFrameRate(long *then, float *remainder) {
    long wait, frameTime;
    wait = 16 + *remainder;

    *remainder -= (int)*remainder;

    frameTime = SDL_GetTicks() - *then;

    wait -= frameTime;
    if (wait < 1) {
        wait = 1;
    }
    SDL_Delay(wait);

    *remainder += 0.667;

    *then = SDL_GetTicks();
}

int main(int argc, char* argv[]) {

    long then;
    float remainder;

    Game.init();
    // Make sure to clean up all resources before exit
    atexit(Game.quit);

    Game.stage->init_stage();

    then = SDL_GetTicks();
    remainder = 0;

    while (Game.running) {

        // Handle inputs from the SDL's queue
        Game.prepare_scene();
        Game.input->do_input();

        Game.delegate->logic();
        Game.delegate->draw();

        SDL_Rect whiteRec;
        whiteRec.h = 16;
        whiteRec.w = 16;
        whiteRec.x = SCREEN_SCALE*SCREEN_W / 2;
        whiteRec.y = SCREEN_SCALE*SCREEN_H / 2;

        SDL_SetRenderDrawColor(Game.screen->renderer, 0xff, 0xff, 0xff, 0xff);
        SDL_RenderFillRect(Game.screen->renderer, &whiteRec);

        Game.present_scene();
        capFrameRate(&then, &remainder);
    };

    return 0;

}
