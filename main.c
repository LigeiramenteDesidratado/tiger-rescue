#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_error.h>
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
#include "structs.h"


// Declarations
void game_init(void);
void do_input(void);
void prepare_scene(void);
void present_scene(void);
void game_quit(void);

static SDL_Texture* load_texture(const char*);
static int  bullet_hit_enemy(Entity*);
static int  bullet_hit_player(Entity*);
static int  detect_colision(Entity*, Entity*);
static void blit(SDL_Texture*, int, int);
static void blitRect(SDL_Texture*, SDL_Rect*, int, int);
static void calc_slope(int srcX, int srcY, int dstX, int dstY, float *refX, float * refY);
static void capFrameRate(long*, float*);
static void do_bullets(void);
static void do_enemies(void);
static void do_enemy_bullets(void);
static void do_key_down(SDL_KeyboardEvent*);
static void do_key_up(SDL_KeyboardEvent*);
static void do_player(void);
static void do_background(void);
static void do_starfield(void);
static void do_explosions(void);
static void do_debris(void);
static void add_explosions(int x, int y, int num);
static void add_debris(Entity* e);

static void draw(void);
static void draw_bullets(void);
static void draw_enemy(void);
static void draw_enemy_bullets(void);
static void draw_player(void);
static void draw_background(void);
static void draw_startfield(void);
static void draw_debris(void);
static void draw_explosions(void);

static void fire_bullet(void);
static void fire_enemy_bullet(Entity*);
static void init_player(void);
static void init_starfield(void);
static void init_stage(void);
static void logic(void);
static void reset_stage(void);
static void spawn_enemy(void);


// Temp
static SDL_Texture* gPlayerBulletTexture;
static SDL_Texture* gEnemyTexture;
static SDL_Texture* gEnemyBulletTexture;
static SDL_Texture* gPlayerTexture;
static SDL_Texture* gBackGround;
static SDL_Texture* gExplosion;

static int backgroundX;
static int enemySpawnTimer;
static int stageResetTimer;


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
        Entity* player_bullet;
        Entity* enemy;
        Entity* enemy_bullet;

        int (*detect_colision)(Entity*, Entity*);
        void(*calc_slope)(int srcX, int srcY, int dstX, int dstY, float *refX, float * refY);

    } entities;

    // All gfx related to scenary like stars and explosions
    struct {
        Star stars[MAX_STARS];
        void (*init_starfield)(void);
    } scenary;

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
        load_texture,
        blit,
        blitRect
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
        .playerBulletHead =  {},
        .playerBulletTail = NULL,
        .enemyHead =  {},
        .enemyTail = NULL,
        .enemyBulletHead = {},
        .enemyBulletTail = NULL,
        .explosionHead = {},
        .explosionTail = NULL,
        .debrisHead = {},
        .debrisTail = NULL,

        .reset_stage = reset_stage,
        .init_stage = init_stage,
    },

    .entities = {
        .player = &(Entity) {},
        .player_bullet = &(Entity) {},
        .enemy = &(Entity) {},
        .calc_slope = calc_slope,
        .detect_colision = detect_colision
    },

    .scenary = {
        .stars = {},
        init_starfield
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


    SDL_DestroyTexture(gPlayerTexture);
    gPlayerTexture = NULL;

    SDL_DestroyTexture(gPlayerBulletTexture);
    gPlayerBulletTexture = NULL;

    SDL_DestroyTexture(gEnemyBulletTexture);
    gEnemyBulletTexture = NULL;

    SDL_DestroyTexture(gEnemyTexture);
    gEnemyTexture = NULL;

    SDL_DestroyTexture(gBackGround);
    gEnemyTexture = NULL;

    SDL_DestroyTexture(gExplosion);
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

static void init_stage(void) {


    gPlayerTexture = Game.graphics->load_texture("gfx/player.png");
    if (gPlayerTexture == NULL) {
        printf("Failed to load player texture! SDL Error %s\n", SDL_GetError());
        exit(1);
    }

    gPlayerBulletTexture = Game.graphics->load_texture("gfx/playerBullet.png");
    if (gPlayerBulletTexture == NULL) {
        printf("Failed to load bullet texture! SDL Error %s\n", SDL_GetError());
        exit(1);
    }

    gEnemyTexture = Game.graphics->load_texture("gfx/enemy.png");
    if (gEnemyTexture == NULL) {
        printf("Failed to load enemy texture! SDL Error %s\n", SDL_GetError());
        exit(1);
    }

    gEnemyBulletTexture = Game.graphics->load_texture("gfx/enemyBullet.png");
    if (gEnemyBulletTexture == NULL) {
        printf("Failed to load enemy bullet texture! SDL Error %s\n", SDL_GetError());
        exit(1);
    }

    gBackGround = Game.graphics->load_texture("gfx/background.png");
    if (gBackGround == NULL) {
        printf("Failed to load background texture! SDL Error %s\n", SDL_GetError());
        exit(1);
    }

    gExplosion = Game.graphics->load_texture("gfx/explosion.png");
    if (gExplosion == NULL) {
        printf("Failed to load explosion texture! SDL Error %s\n", SDL_GetError());
        exit(1);
    }


    Game.stage->reset_stage();
    enemySpawnTimer = 0;

}

static void init_player(void) {

    Game.entities.player = malloc(sizeof(Entity));
    Game.entities.player = memset(Game.entities.player, 0, sizeof(Entity));

    Game.stage->playerTail->next = Game.entities.player;
    Game.stage->playerTail = Game.entities.player;

    Game.entities.player->x = 100;
    Game.entities.player->y = 100;
    Game.entities.player->heath = 1;
    Game.entities.player->texture = gPlayerTexture;

    // Query texture w and h and set it on player
    SDL_QueryTexture(
        Game.entities.player->texture,
        NULL,
        NULL,
        &Game.entities.player->w,
        &Game.entities.player->h
    );
}

static void init_starfield(void) {

    int i;
    for (i = 0; i < MAX_STARS; i++) {
        Game.scenary.stars[i].x  = rand() % SCREEN_W;
        Game.scenary.stars[i].y  = rand() % SCREEN_H;
        Game.scenary.stars[i].speed = 1 + rand() % 8;
    }
}

static void logic(void) {

        do_background();

        do_starfield();

        do_player();

        do_enemies();

        do_bullets();

        do_enemy_bullets();

        spawn_enemy();

        do_explosions();

        do_debris();
}

static void do_background(void) {

    if (--backgroundX < -SCREEN_W) {
        backgroundX = 0;
    }
}

static void do_starfield(void) {

    int i;

     for (i = 0; i < MAX_STARS; i++) {
         Game.scenary.stars[i].x -= Game.scenary.stars[i].speed;
         if (Game.scenary.stars[i].x < 0) {
             Game.scenary.stars[i].x = SCREEN_W + Game.scenary.stars[i].x;
         }
     }
}

static void do_explosions(void) {

    Explosion *e, *prev;
    prev = &Game.stage->explosionHead;

    for (e = Game.stage->explosionHead.next; e != NULL; e = e->next) {
        e->x += e->dx;
        e->y += e->dy;

        if (--e->a <= 0) {

            if (e == Game.stage->explosionTail) {
                Game.stage->explosionTail = prev;
            }

            prev->next = e->next;
            free(e);
            e = prev;
        }

        prev = e;
    }

}

static void do_debris(void) {

  Debris *d, *prev;
  prev = &Game.stage->debrisHead;

  for (d = Game.stage->debrisHead.next; d != NULL; d = d->next) {
    d->x += d->dx;
    d->y += d->dy;

    // accelerate down
    d->dy += 0.5;

    if (--d->life <= 0) {

      if (d == Game.stage->debrisTail) {
        Game.stage->debrisTail = prev;
      }

      prev->next = d->next;
      free(d);
      d = prev;
    }

    prev = d;
  }
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

        if (e->x < -e->w || e->heath == 0) {
            if (e == Game.stage->enemyTail) {
                Game.stage->enemyTail = prev;
            }
            prev->next = e->next;
            free(e);
            e = prev;
        } else if (Game.entities.player != NULL && --e->reload <= 0) {
            fire_enemy_bullet(e);
        }

        prev = e;
    }

}

static void add_explosions(int x, int y, int num) {
    Explosion *e;
    int i;

    for (i = 0; i < num; i++) {
        e = malloc(sizeof(Explosion));
        memset(e, 0, sizeof(Explosion));
        Game.stage->explosionTail->next = e;
        Game.stage->explosionTail = e;

        e->x = x + (rand() % 32) - (rand() % 32);
        e->y = y + (rand() % 32) - (rand() % 32);
        e->dx = (rand() % 10) - (rand() % 10);
        e->dy = (rand() % 10) - (rand() % 10);

        e->dx /= 10;
        e->dy /= 10;

        switch (rand() % 4) {
            case 0:
                e->r = 255;
                break;

            case 1:
                e->r = 255;
                e->g = 128;
                break;

            case 2:
                e->r = 255;
                e->g = 255;
                break;

            default:
                e->r = 255;
                e->g = 255;
                e->b = 255;
                break;

        }

        e->a = rand() % FPS * 3;
    }
}
static void add_debris(Entity *e) {
    Debris *d;
    int x, y, w, h;

    w = e->w /2;
    h = e->h /2;

    for(y = 0; y <= h; y += h) {
        for(x = 0; x <= w; x += w) {
            d = malloc(sizeof(Debris));
            memset(d, 0, sizeof(Debris));
            Game.stage->debrisTail->next = d;
            Game.stage->debrisTail = d;

            d->x = e->x + e->w / 2;
            d->y = e->y + e->h / 2;
            d->dx = (rand() % 5) - (rand() % 5);
            d->dy = -(5 + (rand() % 12));
            d->life = FPS * 2;
            d->texture = e->texture;

            d->rect.x = x;
            d->rect.y = y;
            d->rect.w = w;
            d->rect.h = h;
        }
    }
}

static void spawn_enemy(void) {

    Game.entities.enemy = malloc(sizeof(Entity));
    memset(Game.entities.enemy, 0, sizeof(Entity));

    Entity* enemy = Game.entities.enemy;
    if (--enemySpawnTimer <= 0) {
        Game.stage->enemyTail->next = enemy;
        Game.stage->enemyTail = enemy;
        enemy->heath = 1;

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

    prev = &Game.stage->playerBulletHead;

    for (b = Game.stage->playerBulletHead.next; b != NULL; b = b->next) {
        b->x += b->dx;
        b->y += b->dy;

        if (bullet_hit_enemy(b) || b->x > SCREEN_W) {
            if (b == Game.stage->playerBulletTail) {
                Game.stage->playerBulletTail = prev;
            }
            prev->next = b->next;
            free(b);
            b = prev;
        }

        prev = b;
    }
}

static void do_enemy_bullets(void) {
    Entity *b, *prev;

    prev = &Game.stage->enemyBulletHead;
    int bul = 0;

    for (b = Game.stage->enemyBulletHead.next; b != NULL; b = b->next) {
        b->x += b->dx;
        b->y += b->dy;
        bul++;

        if (bullet_hit_player(b) || b->x < -b->w || b->y < -b->h || b->x > SCREEN_W || b->y > SCREEN_H) {
            if (b == Game.stage->enemyBulletTail) {
                Game.stage->enemyBulletTail = prev;
            }
            prev->next = b->next;
            free(b);
            bul--;
            b = prev;
        }

        /* printf("bullets on screen: %d\r", bul); */
        prev = b;
    }
}

static int bullet_hit_enemy(Entity* b) {

    Entity* e;

    for(e = Game.stage->enemyHead.next; e != NULL; e = e->next) {
        if (detect_colision(b, e)) {
            b->heath = 0;
            e->heath = 0;

            add_explosions(e->x, e->y, 32);
            add_debris(e);

            return 1;
        }
    }

    return 0;
}

static int bullet_hit_player(Entity* b) {
    Entity* e;

    for(e = Game.stage->playerHead.next; e != NULL; e = e->next) {
        if (detect_colision(b, e)) {
            b->heath = 0;
            e->heath = 0;

			add_explosions(e->x, e->y, 32);
			add_debris(e);
            return 1;
        }
    }

    return 0;
}


static int detect_colision(Entity* ent1, Entity* ent2) {
    return (MAX(ent1->x, ent2->x) < MIN(ent1->x + ent1->w, ent2->x + ent2->w))
        && (MAX(ent1->y, ent2->y) < MIN(ent1->y + ent1->h, ent2->y + ent2->h));
}

static void fire_bullet(void) {

    Game.entities.player_bullet = malloc(sizeof(Entity));
    memset(Game.entities.player_bullet, 0, sizeof(Entity));

    Entity* bullet = Game.entities.player_bullet;
    Entity* player = Game.entities.player;

    Game.stage->playerBulletTail->next = bullet;
    Game.stage->playerBulletTail = bullet;

    // set initial position of the bullet
    bullet->x = player->x;
    bullet->y = player->y;

    bullet->dx = PLAYER_BULLET_SPEED;
    bullet->heath = 1;
    bullet->texture = gPlayerBulletTexture;
    SDL_QueryTexture(bullet->texture, NULL, NULL, &bullet->w, &bullet->h);

    bullet->y += (player->h / 2) - (bullet->h / 2);

    player->reload = 8;

}

static void fire_enemy_bullet(Entity* e) {

    Game.entities.enemy_bullet = malloc(sizeof(Entity));
    memset(Game.entities.enemy_bullet, 0, sizeof(Entity));

    Entity* bullet = Game.entities.enemy_bullet;
    /* Entity* player = Game.entities.player; */

    Game.stage->enemyBulletTail->next = bullet;
    Game.stage->enemyBulletTail = bullet;

    bullet->x = e->x;
    bullet->y = e->y;
    bullet->heath = 1;
    bullet->texture = gEnemyBulletTexture;
    SDL_QueryTexture(bullet->texture, NULL, NULL, &bullet->w, &bullet->h);

    bullet->x += (e->w / 2) - (bullet->w / 2);
    bullet->y += (e->h / 2) - (bullet->h / 2);

    Entity* player = Game.entities.player;
    Game.entities.calc_slope(
        player->x + (player->w / 2),
        player->y + (player->h / 2),
        e->x,
        e->y,
        &bullet->dx,
        &bullet->dy
    );

    bullet->dx *= ENEMY_BULLET_SPPED;
    bullet->dy *= ENEMY_BULLET_SPPED;

    e->reload = (rand() % FPS * 2);
}

// present_scene will clear the screen and set the background color
void prepare_scene(void) {
    SDL_SetRenderDrawColor(Game.screen->renderer, 0x12, 0x12, 0x12, 0xFF);
    SDL_RenderClear(Game.screen->renderer);
}

void present_scene(void) {
    SDL_RenderPresent(Game.screen->renderer);
}

static SDL_Texture* load_texture(const char* filename) {
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
static void blit(SDL_Texture *texture, int x, int y) {
    SDL_Rect dest;

    dest.x = x;
    dest.y = y;
    SDL_QueryTexture(texture, NULL, NULL, &dest.w, &dest.h);

    SDL_RenderCopy(Game.screen->renderer, texture, NULL, &dest);
}
static void blitRect(SDL_Texture* texture, SDL_Rect* src, int x, int y) {

    SDL_Rect dest;

    dest.x = x;
    dest.y = y;
    dest.w = src->w;
    dest.h = src->h;

    SDL_RenderCopy(Game.screen->renderer, texture, src, &dest);
}


static void do_key_up(SDL_KeyboardEvent* event) {
    // check if the keyboard event was a result of  Keyboard repeat event
    if (event->repeat == 0 && event->keysym.scancode < MAX_KEYBOARD_KEYS) {
        Game.input->keyboard[event->keysym.scancode] = 0;
    }
}

static void do_key_down(SDL_KeyboardEvent* event) {
    // check if the keyboard event was a result of  Keyboard repeat event
    if (event->repeat == 0 && event->keysym.scancode < MAX_KEYBOARD_KEYS) {
        Game.input->keyboard[event->keysym.scancode] = 1;
    }
}

static void draw(void) {
    draw_background();
    draw_startfield();
    draw_player();
    draw_bullets();
    draw_enemy_bullets();
    draw_enemy();
    draw_debris();
    draw_explosions();
}

static void draw_background(void) {
    SDL_Rect dest;
    int x;

    for (x = backgroundX; x < SCREEN_W; x += SCREEN_W) {
        dest.x = x;
        dest.y = 0;
        dest.w = SCREEN_W;
        dest.h = SCREEN_H;

        SDL_RenderCopy(Game.screen->renderer, gBackGround, NULL, &dest);
    }
}

static void draw_startfield(void) {
    int i, c;

    for (i = 0; i < MAX_STARS; i++) {
        c = 32 * Game.scenary.stars[i].speed;
        SDL_SetRenderDrawColor(Game.screen->renderer, c, c, c, c);
        SDL_RenderDrawLine(Game.screen->renderer,
                Game.scenary.stars[i].x,
                Game.scenary.stars[i].y,
                Game.scenary.stars[i].x+3,
                Game.scenary.stars[i].y);
    }
}

static void draw_debris(void) {
    Debris *d;

    for (d = Game.stage->debrisHead.next; d != NULL; d = d->next) {
        blitRect(d->texture, &d->rect, d->x, d->y);
    }
}

static void draw_explosions(void) {
    Explosion *e;

    SDL_SetRenderDrawBlendMode(Game.screen->renderer, SDL_BLENDMODE_ADD);
    SDL_SetTextureBlendMode(gExplosion, SDL_BLENDMODE_ADD);

    for (e = Game.stage->explosionHead.next; e != NULL; e = e->next) {
        SDL_SetTextureColorMod(gExplosion, e->r, e->g, e->b);
        SDL_SetTextureAlphaMod(gExplosion, e->a);
        blit(gExplosion, e->x, e->y);
    }

    SDL_SetRenderDrawBlendMode(Game.screen->renderer, SDL_BLENDMODE_NONE);

}

static void draw_player(void) {
    Entity* player = Game.entities.player;
    Game.graphics->blit(player->texture, player->x, player->y);
}

static void draw_bullets(void) {
    Entity *b;

    for (b = Game.stage->playerBulletHead.next; b != NULL; b = b->next) {
        Game.graphics->blit(b->texture, b->x, b->y);
    }
}

static void draw_enemy_bullets(void) {
    Entity *b;

    for (b = Game.stage->enemyBulletHead.next; b != NULL; b = b->next) {
        Game.graphics->blit(b->texture, b->x, b->y);
    }
}

static void reset_stage(void) {

    Entity* e;
    Explosion* Exp;
    Debris* Deb;

    while (Game.stage->enemyBulletHead.next) {
        e = Game.stage->enemyBulletHead.next;
        Game.stage->enemyBulletHead.next = e->next;
        free(e);
    }

    while (Game.stage->enemyHead.next) {
        e = Game.stage->enemyHead.next;
        Game.stage->enemyHead.next = e->next;
        free(e);
    }

    while (Game.stage->playerBulletHead.next) {
        e = Game.stage->playerBulletHead.next;
        Game.stage->playerBulletHead.next = e->next;
        free(e);
    }

    while (Game.stage->playerHead.next) {
        e = Game.stage->playerHead.next;
        Game.stage->playerHead.next = e->next;
        free(e);
    }

    while (Game.stage->explosionHead.next) {
        Exp = Game.stage->explosionHead.next;
        Game.stage->explosionHead.next = Exp->next;
        free(Exp);
    }

    while (Game.stage->debrisHead.next) {
        Deb = Game.stage->debrisHead.next;
        Game.stage->debrisHead.next = Deb->next;
        free(Deb);
    }

    memset(Game.stage, 0, sizeof(Stage));

    Game.stage->playerTail = &Game.stage->playerHead;
    Game.stage->playerBulletTail = &Game.stage->playerBulletHead;
    Game.stage->enemyBulletTail = &Game.stage->enemyBulletHead;
    Game.stage->enemyTail = &Game.stage->enemyHead;
    Game.stage->explosionTail = &Game.stage->explosionHead;
    Game.stage->debrisTail = &Game.stage->debrisHead;

    Game.stage->init_stage = init_stage;
    Game.stage->reset_stage = reset_stage;

    init_player();
    init_starfield();

    enemySpawnTimer = 0;
    stageResetTimer = FPS*3;
}

static void calc_slope(int srcX, int srcY, int dstX, int dstY, float *refX, float * refY) {
    int steps = MAX(abs(srcX-dstX), abs(srcY-dstY));
    if (steps == 0) {
        *refX = *refY = 0;
        return;
    }

    *refX = (srcX-dstX);
    *refX /= steps;
    *refY = (srcY-dstY);
    *refY /= steps;
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

        Uint64 start = SDL_GetPerformanceCounter();

        // Handle inputs from the SDL's queue
        Game.prepare_scene();
        Game.input->do_input();


        if (Game.entities.player != NULL && Game.entities.player->heath <= 0) {
            Game.entities.player = NULL;
        }

        if (Game.entities.player == NULL && --stageResetTimer <= 0) {
            Game.stage->reset_stage();
        };

        if (Game.entities.player != NULL){
            Game.delegate->logic();
            Game.delegate->draw();
        }

        Game.present_scene();
        capFrameRate(&then, &remainder);
        Uint64 end = SDL_GetPerformanceCounter();
        float elapsed = (end - start) / (float)SDL_GetPerformanceFrequency();
        printf("FPS: %.2f\r", (1.0f / elapsed));
    };

    return 0;

}
