/* Effects for Frostline Hold: everything that makes a hit feel like a hit.
 *
 * The model raises events without positions, so this module reads the
 * battle instead: each new tick it compares the model with a snapshot of
 * the last one. An enemy that lost health was struck; one that vanished
 * died (or, at the gate, got through); a shot that vanished landed; a
 * tower whose shot timer went back to zero just fired. Each of those
 * starts particles from a small pool, and the big ones shake the camera.
 *
 * Everything stays in the two tones: ink and paper, with tone from dither.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#include "private/frostline_view_internal.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PI_F 3.14159265f

/* ------------------------------------------------------------------------
 * Particles
 * --------------------------------------------------------------------- */

typedef enum {
    P_SPARK = 0,   /* a streak along its velocity */
    P_DEBRIS,      /* a chip that falls, bounces and settles */
    P_DUST,        /* a dithered puff that swells and thins */
    P_SMOKE,       /* a puff that rises */
    P_RING,        /* a shock ring */
    P_FEATHER,     /* sways down to the ground */
    P_SHARD,       /* an ice diamond flung out */
    P_EMBER,       /* a spark that drifts up */
    P_DROP,        /* a slime droplet */
    P_TWINKLE,     /* a four-point star that swells and fades */
    P_FLASH,       /* a starburst of rays */
    P_COIN,        /* the bounty: hops up and fades */
    P_WISP,        /* a soul wavering upwards */
    P_CLOD,        /* a lump of earth */
} particle_kind_t;

typedef struct {
    float x, y;
    float vx, vy;
    float floor;      /* the ground under it, where debris lands */
    uint8_t kind;
    uint8_t age;
    uint8_t life;
    uint8_t size;
    uint8_t delay;    /* ticks before it shows */
    uint8_t landed;
    int8_t spin;
} particle_t;

/* Marks left on the ground: scorch, splat, rime. */
typedef enum { D_SCORCH = 0, D_SPLAT, D_RIME, D_CRATER } decal_kind_t;

typedef struct {
    int16_t x, y;
    uint8_t rx, ry;
    uint8_t kind;
    uint16_t age;
    uint16_t life;
} decal_t;

#define FX_PARTICLES 220
#define FX_DECALS 24

/* What the last tick looked like. */
typedef struct {
    float x, y, hp, dist;
    uint16_t ward;
    uint8_t alive, kind, burrowed, stage, flies;
} enemy_snap_t;

typedef struct {
    float x, y;       /* last seen */
    float sx, sy;     /* where it was first seen: the arc starts here */
    float tx, ty;
    float total;      /* first-seen distance to the target */
    int16_t aoe;
    uint8_t active, kind, flags;
} shot_snap_t;

typedef struct {
    uint8_t active, kind, level, shot_tick;
    int16_t x, y;
} tower_snap_t;

static struct {
    bool primed;
    uint32_t tick;
    uint16_t run_serial;
    uint32_t rng;
    enemy_snap_t enemies[FL_MAX_ENEMIES];
    shot_snap_t shots[FL_MAX_PROJECTILES];
    tower_snap_t towers[FL_MAX_TOWERS];
    uint8_t arc_ttl[FL_MAX_ARCS];
    uint16_t hero_skill[FL_MAX_HEROES];
    uint16_t hero_cooldown[FL_MAX_HEROES];
    uint8_t hero_alive[FL_MAX_HEROES];
    /* A struck enemy is knocked back for a few ticks. */
    uint8_t jolt[FL_MAX_ENEMIES];
    int8_t jolt_dx[FL_MAX_ENEMIES];
    int8_t jolt_dy[FL_MAX_ENEMIES];
    particle_t particles[FX_PARTICLES];
    decal_t decals[FX_DECALS];
    int next_particle;
    int next_decal;
    int shake;          /* amplitude in px */
    int shake_ticks;
} S;

static uint32_t fx_rand(void)
{
    S.rng = S.rng * 1664525u + 1013904223u;
    return S.rng >> 8;
}

/* A float in [lo, hi). */
static float frand(float lo, float hi)
{
    return lo + (hi - lo) * (float)(fx_rand() & 0xFFFF) / 65536.0f;
}

static particle_t *emit(particle_kind_t kind, float x, float y, float vx,
                        float vy, int life, int size)
{
    /* The pool is a ring: when it is full the oldest gives way. */
    particle_t *p = &S.particles[S.next_particle];
    S.next_particle = (S.next_particle + 1) % FX_PARTICLES;
    memset(p, 0, sizeof(*p));
    p->kind = (uint8_t)kind;
    p->x = x;
    p->y = y;
    p->vx = vx;
    p->vy = vy;
    p->floor = y + 4.0f;
    p->life = (uint8_t)(life > 255 ? 255 : life);
    p->size = (uint8_t)size;
    p->spin = (int8_t)((int)(fx_rand() % 7u) - 3);
    return p;
}

/* `count` particles flung out round a point at speeds in [lo, hi). */
static void burst(particle_kind_t kind, float x, float y, int count, float lo,
                  float hi, int life, int size, float lift)
{
    for (int i = 0; i < count; ++i) {
        float a = frand(0.0f, 2.0f * PI_F);
        float v = frand(lo, hi);
        particle_t *p = emit(kind, x, y, cosf(a) * v, sinf(a) * v * 0.6f - lift,
                             life + (int)(fx_rand() % 6u), size);
        p->floor = y + frand(2.0f, 8.0f);
    }
}

static void ring(float x, float y, int start, float speed, int life)
{
    particle_t *p = emit(P_RING, x, y, speed, 0.0f, life, start);
    p->floor = y;
}

static void decal(decal_kind_t kind, int x, int y, int rx, int ry, int life)
{
    decal_t *d = &S.decals[S.next_decal];
    S.next_decal = (S.next_decal + 1) % FX_DECALS;
    d->x = (int16_t)x;
    d->y = (int16_t)y;
    d->rx = (uint8_t)(rx > 255 ? 255 : rx);
    d->ry = (uint8_t)(ry > 255 ? 255 : ry);
    d->kind = (uint8_t)kind;
    d->age = 0;
    d->life = (uint16_t)life;
}

void fx_shake(int strength, int ticks)
{
    if (strength > S.shake) {
        S.shake = strength;
    }
    if (ticks > S.shake_ticks) {
        S.shake_ticks = ticks;
    }
}

void fx_shake_offset(int *dx, int *dy)
{
    *dx = 0;
    *dy = 0;
    if (S.shake_ticks <= 0 || S.shake <= 0) {
        return;
    }
    /* Strong at first, easing off as it runs out. */
    int amp = S.shake;
    if (S.shake_ticks < 6) {
        amp = (amp * S.shake_ticks + 5) / 6;
    }
    if (amp <= 0) {
        return;
    }
    uint32_t h = jag_hash(S.tick * 2654435761u);
    *dx = (int)(h % (uint32_t)(amp * 2 + 1)) - amp;
    *dy = (int)((h >> 8) % (uint32_t)(amp * 2 + 1)) - amp;
}

void fx_enemy_jolt(int index, int *dx, int *dy)
{
    *dx = 0;
    *dy = 0;
    if (index < 0 || index >= FL_MAX_ENEMIES || S.jolt[index] == 0) {
        return;
    }
    *dx = S.jolt_dx[index];
    *dy = S.jolt_dy[index];
}

void fx_reset(void)
{
    uint32_t rng = S.rng ? S.rng : 0x5EED1234u;
    memset(&S, 0, sizeof(S));
    S.rng = rng;
}

/* ------------------------------------------------------------------------
 * Where things are
 * --------------------------------------------------------------------- */

static float enemy_size(int kind)
{
    float size;
    (void)enemy_sheet(kind, &size);
    return size;
}

static bool is_boss(int kind)
{
    return kind == FL_ENEMY_WYRM || kind == FL_ENEMY_KING;
}

/* The middle of an enemy's body, where hits land, and the top of its
 * sprite. The same sums as draw_enemy. */
static void enemy_body(float ex, float ey, int kind, bool flies, float *bx,
                       float *by, float *top)
{
    float size = enemy_size(kind);
    float y = ey - size + SPRITE_FOOT;
    if (flies) {
        y = ey - (float)FL_FLY_HEIGHT - (float)CROW_BODY_Y;
    }
    *bx = ex;
    *by = y + size * 0.55f;
    *top = y + (flies ? 10.0f : size * 0.2f);
}

/* ------------------------------------------------------------------------
 * What starts the effects
 * --------------------------------------------------------------------- */

static void on_muzzle(const fl_tower_t *tower)
{
    float mx = (float)tower->x;
    float my = (float)tower->y - fl_muzzle_height(tower->kind);
    switch (tower->kind) {
    case FL_TOWER_GUARD:
        /* The string's snap throws a puff of frost off the deck. */
        burst(P_SPARK, mx, my, 3, 0.8f, 1.6f, 4, 1, 0.3f);
        emit(P_TWINKLE, mx, my - 2.0f, 0.0f, 0.0f, 5, 3);
        break;
    case FL_TOWER_FROST:
        emit(P_TWINKLE, mx, my, 0.0f, 0.0f, 7, 5);
        burst(P_SHARD, mx, my, 3, 0.4f, 1.0f, 8, 1, 0.4f);
        break;
    case FL_TOWER_SIEGE: {
        /* The arm slams its stop: dust kicks out from the base. */
        float bx = (float)tower->x;
        float by = (float)tower->y;
        for (int k = 0; k < 4; ++k) {
            float side = k % 2 ? 1.0f : -1.0f;
            particle_t *p = emit(P_DUST, bx + side * 10.0f, by - 1.0f,
                                 side * frand(0.6f, 1.2f), -frand(0.0f, 0.3f),
                                 14, 3);
            p->floor = by;
        }
        burst(P_DEBRIS, bx, by - 6.0f, 2, 0.5f, 1.2f, 16, 1, 1.4f);
        break;
    }
    case FL_TOWER_STORM:
        burst(P_SPARK, mx, my, 5, 1.2f, 2.4f, 5, 1, 0.0f);
        break;
    case FL_TOWER_RUNE:
        ring(mx, my, 3, 1.4f, 7);
        break;
    case FL_TOWER_EMBER:
        for (int k = 0; k < 3; ++k) {
            emit(P_SMOKE, mx + frand(-3.0f, 3.0f), my, frand(-0.2f, 0.2f),
                 -frand(0.4f, 0.8f), 18 + k * 3, 2 + k);
        }
        burst(P_EMBER, mx, my, 3, 0.3f, 0.8f, 14, 1, 0.8f);
        break;
    default:
        break;
    }
}

static void on_impact(const shot_snap_t *shot)
{
    float x = shot->tx;
    float y = shot->ty;
    switch (shot->kind) {
    case FL_TOWER_GUARD:
        if (shot->flags & FL_SHOT_PIERCE) {
            /* A ballista bolt bites hard. */
            emit(P_FLASH, x, y, 0.0f, 0.0f, 5, 7);
            burst(P_SPARK, x, y, 6, 1.2f, 2.6f, 6, 1, 0.4f);
        } else {
            burst(P_SPARK, x, y, 4, 0.8f, 2.0f, 5, 1, 0.3f);
        }
        burst(P_DEBRIS, x, y, 2, 0.4f, 1.0f, 12, 1, 1.0f);
        break;
    case FL_TOWER_FROST:
        burst(P_SHARD, x, y, 7, 0.8f, 2.0f, 12, 2, 0.4f);
        ring(x, y, 3, 1.3f, 8);
        emit(P_TWINKLE, x, y, 0.0f, 0.0f, 6, 4);
        if (shot->aoe > 0) {
            ring(x, y, 4, (float)shot->aoe / 8.0f, 9);
            decal(D_RIME, (int)x, (int)y + 2, shot->aoe / 2 + 4, shot->aoe / 5 + 2, 90);
        }
        break;
    case FL_TOWER_SIEGE: {
        int r = shot->aoe > 0 ? shot->aoe : 18;
        ring(x, y, 4, (float)r / 7.0f, 8);
        ring(x, y, 2, (float)r / 11.0f, 10);
        emit(P_FLASH, x, y - 2.0f, 0.0f, 0.0f, 4, 9);
        for (int k = 0; k < 7; ++k) {
            particle_t *p = emit(P_DEBRIS, x, y - 2.0f, frand(-2.2f, 2.2f),
                                 -frand(1.6f, 3.4f), 28 + (int)(fx_rand() % 10u),
                                 1 + (int)(fx_rand() % 2u));
            p->floor = y + frand(-4.0f, 8.0f);
        }
        for (int k = 0; k < 5; ++k) {
            particle_t *p = emit(P_DUST, x + frand(-8.0f, 8.0f),
                                 y + frand(-3.0f, 3.0f), frand(-0.6f, 0.6f),
                                 -frand(0.0f, 0.3f), 20 + k * 2, 4 + k % 3);
            p->floor = y;
        }
        if (shot->flags & FL_SHOT_BURN) {
            burst(P_EMBER, x, y, 6, 0.4f, 1.2f, 16, 1, 1.2f);
        }
        decal(D_CRATER, (int)x, (int)y + 1, r * 2 / 3, r / 4 + 2, 150);
        fx_shake(2, 5);
        break;
    }
    case FL_TOWER_EMBER: {
        int r = shot->aoe > 0 ? shot->aoe : 16;
        emit(P_FLASH, x, y - 3.0f, 0.0f, 0.0f, 5, 8);
        ring(x, y, 3, (float)r / 8.0f, 8);
        burst(P_EMBER, x, y - 2.0f, 10, 0.5f, 1.6f, 18, 1, 1.4f);
        for (int k = 0; k < 3; ++k) {
            emit(P_SMOKE, x + frand(-6.0f, 6.0f), y - 4.0f, frand(-0.3f, 0.3f),
                 -frand(0.5f, 0.9f), 22 + k * 4, 3 + k);
        }
        decal(D_SCORCH, (int)x, (int)y + 1, r * 2 / 3, r / 4 + 2, 120);
        break;
    }
    default:
        burst(P_SPARK, x, y, 3, 0.8f, 1.8f, 5, 1, 0.3f);
        break;
    }
}

static void on_hit(int index, const fl_enemy_t *enemy, float lost)
{
    float bx, by, top;
    enemy_body(enemy->x, enemy->y, enemy->kind, fl_enemy_flies(enemy), &bx, &by,
               &top);
    (void)top;
    if (S.jolt[index] == 0) {
        S.jolt_dx[index] = (int8_t)(fx_rand() % 2u ? 1 : -1);
        S.jolt_dy[index] = -1;
        burst(P_SPARK, bx, by, 2, 0.8f, 1.6f, 4, 1, 0.2f);
    }
    S.jolt[index] = 3;
    /* A blow worth a fifth of its health knocks chips off. */
    if (enemy->max_hp > 0.0f && lost > enemy->max_hp * 0.2f) {
        burst(P_DEBRIS, bx, by, 2, 0.6f, 1.4f, 14, 1, 1.2f);
        emit(P_FLASH, bx, by, 0.0f, 0.0f, 3, 5);
    }
}

static void on_leak(void)
{
    /* The gate takes the blow: a flash and a jolt through the fortress. */
    float gx = (float)FL_GOAL.x - 6.0f;
    float gy = (float)FL_GOAL.y - 10.0f;
    emit(P_FLASH, gx, gy, 0.0f, 0.0f, 6, 12);
    ring(gx, gy, 6, 2.0f, 9);
    burst(P_DEBRIS, gx, gy, 5, 0.8f, 2.0f, 20, 1, 1.6f);
    fx_shake(3, 8);
}

static void on_death(const enemy_snap_t *e)
{
    float bx, by, top;
    enemy_body(e->x, e->y, e->kind, e->flies, &bx, &by, &top);
    float size = enemy_size(e->kind);
    int kind = e->kind;
    float ground = e->y;

    /* The bounty hops off every kill. */
    emit(P_COIN, bx, top, frand(-0.4f, 0.4f), -2.2f, 18, 2);

    if (kind == FL_ENEMY_CROW) {
        /* A burst of feathers drifting down to where its shadow was. */
        for (int k = 0; k < 9; ++k) {
            particle_t *p = emit(P_FEATHER, bx + frand(-5.0f, 5.0f),
                                 by + frand(-4.0f, 4.0f), frand(-1.2f, 1.2f),
                                 -frand(0.2f, 1.2f), 60 + (int)(fx_rand() % 20u), 3);
            p->floor = ground + frand(-3.0f, 3.0f);
        }
        emit(P_FLASH, bx, by, 0.0f, 0.0f, 4, 6);
        return;
    }
    if (kind == FL_ENEMY_SLIME || kind == FL_ENEMY_SLIMELET || kind == FL_ENEMY_SAP) {
        int drops = kind == FL_ENEMY_SLIME ? 12 : 6;
        for (int k = 0; k < drops; ++k) {
            particle_t *p = emit(P_DROP, bx, by, frand(-2.0f, 2.0f),
                                 -frand(1.0f, 2.8f), 30, 1 + (int)(fx_rand() % 2u));
            p->floor = ground + frand(-4.0f, 4.0f);
        }
        decal(D_SPLAT, (int)bx, (int)ground, (int)(size * 0.3f), (int)(size * 0.12f) + 2,
              160);
        ring(bx, ground - 2.0f, 3, 1.2f, 7);
        return;
    }
    if (is_boss(kind)) {
        /* A boss goes out in a chain of blasts and a long shudder. */
        for (int k = 0; k < 6; ++k) {
            particle_t *f = emit(P_FLASH, bx + frand(-22.0f, 22.0f),
                                 by + frand(-22.0f, 14.0f), 0.0f, 0.0f, 6, 10 + k);
            f->delay = (uint8_t)(k * 5);
            particle_t *r = emit(P_RING, f->x, f->y, 2.0f, 0.0f, 10, 4);
            r->delay = f->delay;
        }
        for (int k = 0; k < 26; ++k) {
            particle_t *p = emit(P_DEBRIS, bx + frand(-14.0f, 14.0f),
                                 by + frand(-14.0f, 10.0f), frand(-3.0f, 3.0f),
                                 -frand(1.5f, 4.5f), 40 + (int)(fx_rand() % 20u),
                                 1 + (int)(fx_rand() % 3u));
            p->floor = ground + frand(-10.0f, 10.0f);
            p->delay = (uint8_t)(fx_rand() % 24u);
        }
        for (int k = 0; k < 8; ++k) {
            particle_t *p = emit(P_DUST, bx + frand(-26.0f, 26.0f),
                                 ground + frand(-6.0f, 4.0f), frand(-0.6f, 0.6f),
                                 -frand(0.0f, 0.3f), 36, 6 + k % 4);
            p->delay = (uint8_t)(10 + k * 2);
        }
        ring(bx, ground, 8, 4.0f, 18);
        decal(D_CRATER, (int)bx, (int)ground, 34, 12, 300);
        fx_shake(6, 34);
        return;
    }
    bool heavy = size > ENEMY_DRAW_SIZE;
    if (kind == FL_ENEMY_GOLEM || kind == FL_ENEMY_TITAN || kind == FL_ENEMY_HUSK) {
        for (int k = 0; k < (heavy ? 12 : 8); ++k) {
            particle_t *p = emit(P_DEBRIS, bx + frand(-8.0f, 8.0f),
                                 by + frand(-8.0f, 6.0f), frand(-2.2f, 2.2f),
                                 -frand(1.2f, 3.2f), 34 + (int)(fx_rand() % 12u),
                                 2 + (int)(fx_rand() % 2u));
            p->floor = ground + frand(-5.0f, 6.0f);
        }
        for (int k = 0; k < 4; ++k) {
            emit(P_DUST, bx + frand(-10.0f, 10.0f), ground - 2.0f,
                 frand(-0.5f, 0.5f), -0.1f, 26, 5 + k % 2);
        }
        fx_shake(kind == FL_ENEMY_TITAN ? 4 : 2, kind == FL_ENEMY_TITAN ? 12 : 6);
        return;
    }
    /* Anything else: a pop of smoke, a few chips, a wisp going up. */
    emit(P_FLASH, bx, by, 0.0f, 0.0f, 4, 6);
    for (int k = 0; k < 3; ++k) {
        emit(P_DUST, bx + frand(-4.0f, 4.0f), by + frand(-2.0f, 4.0f),
             frand(-0.4f, 0.4f), -frand(0.1f, 0.5f), 16 + k * 3, 3 + k);
    }
    for (int k = 0; k < 4; ++k) {
        particle_t *p = emit(P_DEBRIS, bx, by, frand(-1.6f, 1.6f),
                             -frand(1.0f, 2.4f), 24, 1);
        p->floor = ground + frand(-3.0f, 4.0f);
    }
    emit(P_WISP, bx, top, 0.0f, -0.7f, 26, 1);
}

static void on_burrow(const fl_enemy_t *enemy)
{
    float x = enemy->x;
    float y = enemy->y;
    bool big = enemy->kind == FL_ENEMY_WYRM;
    int clods = big ? 16 : 8;
    for (int k = 0; k < clods; ++k) {
        particle_t *p = emit(P_CLOD, x + frand(-4.0f, 4.0f), y - 2.0f,
                             frand(-1.8f, 1.8f) * (big ? 1.5f : 1.0f),
                             -frand(1.2f, 3.0f), 26, big ? 2 : 1);
        p->floor = y + frand(-3.0f, 5.0f);
    }
    for (int k = 0; k < (big ? 5 : 2); ++k) {
        emit(P_DUST, x + frand(-8.0f, 8.0f), y - 1.0f, frand(-0.5f, 0.5f),
             -0.1f, 22, big ? 6 : 4);
    }
    ring(x, y, 3, big ? 2.4f : 1.4f, 9);
    if (big) {
        fx_shake(3, 10);
    }
}

static void on_crack(const fl_enemy_t *enemy)
{
    float bx, by, top;
    enemy_body(enemy->x, enemy->y, enemy->kind, false, &bx, &by, &top);
    (void)top;
    for (int k = 0; k < 10; ++k) {
        particle_t *p = emit(P_DEBRIS, bx + frand(-8.0f, 8.0f),
                             by + frand(-8.0f, 4.0f), frand(-2.4f, 2.4f),
                             -frand(1.4f, 3.0f), 32, 2);
        p->floor = enemy->y + frand(-4.0f, 6.0f);
    }
    emit(P_FLASH, bx, by, 0.0f, 0.0f, 5, 9);
    fx_shake(2, 6);
}

static void on_ward(const fl_enemy_t *enemy)
{
    float bx, by, top;
    enemy_body(enemy->x, enemy->y, enemy->kind, false, &bx, &by, &top);
    (void)top;
    ring(bx, by, 6, 2.6f, 10);
    burst(P_SHARD, bx, by, 10, 1.4f, 2.6f, 14, 2, 0.2f);
}

static void on_built(const fl_tower_t *tower)
{
    float x = (float)tower->x;
    float y = (float)tower->y;
    for (int k = 0; k < 6; ++k) {
        float side = (float)(k - 3) / 3.0f;
        particle_t *p = emit(P_DUST, x + side * 16.0f, y, side * 0.8f, -0.1f,
                             18 + k, 4);
        p->floor = y;
    }
    for (int k = 0; k < 5; ++k) {
        particle_t *p = emit(P_DEBRIS, x + frand(-6.0f, 6.0f), y - 12.0f,
                             frand(-1.4f, 1.4f), -frand(1.0f, 2.2f), 22, 1);
        p->floor = y + frand(-2.0f, 4.0f);
    }
    for (int k = 0; k < 4; ++k) {
        particle_t *t = emit(P_TWINKLE, x + frand(-12.0f, 12.0f),
                             y - frand(10.0f, 40.0f), 0.0f, -0.2f, 10, 4);
        t->delay = (uint8_t)(4 + k * 3);
    }
    ring(x, y, 6, 1.8f, 9);
}

static void on_upgraded(const fl_tower_t *tower)
{
    float x = (float)tower->x;
    float y = (float)tower->y;
    /* Light rising up the new stone. */
    for (int k = 0; k < 8; ++k) {
        particle_t *t = emit(P_TWINKLE, x + frand(-12.0f, 12.0f), y - 4.0f,
                             0.0f, -frand(0.8f, 1.6f), 14, 3 + k % 3);
        t->delay = (uint8_t)(k * 2);
    }
    ring(x, y - 2.0f, 8, 2.0f, 10);
    emit(P_FLASH, x, y - 26.0f, 0.0f, 0.0f, 6, 10);
}

static void on_sold(const tower_snap_t *tower)
{
    float x = (float)tower->x;
    float y = (float)tower->y;
    for (int k = 0; k < 5; ++k) {
        emit(P_DUST, x + frand(-12.0f, 12.0f), y - frand(0.0f, 20.0f),
             frand(-0.4f, 0.4f), -0.2f, 20, 5);
    }
    for (int k = 0; k < 5; ++k) {
        particle_t *c = emit(P_COIN, x + frand(-6.0f, 6.0f), y - 20.0f,
                             frand(-1.0f, 1.0f), -frand(1.8f, 2.8f), 20, 2);
        c->delay = (uint8_t)(k * 2);
    }
}

static void on_nova(const fl_hero_t *hero)
{
    float x = (float)hero->x;
    float y = (float)hero->y - 8.0f;
    ring(x, y, 6, 4.0f, 12);
    ring(x, y, 4, 2.6f, 14);
    burst(P_SHARD, x, y, 16, 2.0f, 3.6f, 16, 2, 0.0f);
    emit(P_FLASH, x, y, 0.0f, 0.0f, 6, 14);
    decal(D_RIME, (int)x, (int)hero->y, 44, 16, 120);
    fx_shake(2, 8);
}

static void on_hero_strike(const fl_hero_t *hero)
{
    /* The spear's thrust leaves a glint ahead of the warden. */
    emit(P_TWINKLE, (float)hero->x - 12.0f, (float)hero->y - 22.0f, 0.0f, 0.0f, 5, 4);
    burst(P_SPARK, (float)hero->x - 14.0f, (float)hero->y - 20.0f, 2, 0.8f, 1.4f, 4, 1, 0.2f);
}

/* ------------------------------------------------------------------------
 * The per-tick comparison
 * --------------------------------------------------------------------- */

static void snapshot(const fl_state_t *model)
{
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        const fl_enemy_t *e = &model->enemies[i];
        enemy_snap_t *s = &S.enemies[i];
        s->alive = e->alive;
        s->kind = (uint8_t)e->kind;
        s->x = e->x;
        s->y = e->y;
        s->hp = e->hp;
        s->dist = e->dist;
        s->burrowed = e->burrowed;
        s->stage = e->stage;
        s->ward = e->ward_ticks;
        s->flies = e->alive && fl_enemy_flies(e);
    }
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        const fl_tower_t *t = &model->towers[i];
        tower_snap_t *s = &S.towers[i];
        s->active = t->active;
        s->kind = (uint8_t)t->kind;
        s->level = t->level;
        s->shot_tick = t->shot_tick;
        s->x = t->x;
        s->y = t->y;
    }
    for (int i = 0; i < FL_MAX_ARCS; ++i) {
        S.arc_ttl[i] = model->arcs[i].ttl;
    }
    for (int i = 0; i < FL_MAX_HEROES; ++i) {
        S.hero_skill[i] = model->heroes[i].skill_timer;
        S.hero_cooldown[i] = model->heroes[i].cooldown;
        S.hero_alive[i] = model->heroes[i].active && model->heroes[i].alive;
    }
}

static void note_shot(shot_snap_t *s, const fl_projectile_t *p)
{
    s->active = 1;
    s->kind = p->kind;
    s->flags = p->flags;
    s->aoe = p->aoe;
    s->x = p->x;
    s->y = p->y;
    s->tx = p->tx;
    s->ty = p->ty;
}

static void start_shot(shot_snap_t *s, const fl_projectile_t *p)
{
    note_shot(s, p);
    s->sx = p->x;
    s->sy = p->y;
    float dx = p->tx - p->x;
    float dy = p->ty - p->y;
    s->total = sqrtf(dx * dx + dy * dy);
    if (s->total < 1.0f) {
        s->total = 1.0f;
    }
}

static void compare_shots(const fl_state_t *model)
{
    for (int i = 0; i < FL_MAX_PROJECTILES; ++i) {
        const fl_projectile_t *p = &model->projectiles[i];
        shot_snap_t *s = &S.shots[i];
        if (!p->active) {
            if (s->active) {
                on_impact(s);
                s->active = 0;
            }
            continue;
        }
        if (!s->active) {
            start_shot(s, p);
            continue;
        }
        /* The same slot may have landed and fired again within one tick:
         * a new kind, or a jump no flight could make. */
        float jump = fabsf(p->x - s->x) + fabsf(p->y - s->y);
        float step = (float)p->speed / 30.0f * (float)(model->speed ? model->speed : 1);
        if (p->kind != s->kind || jump > step * 2.0f + 6.0f) {
            on_impact(s);
            start_shot(s, p);
            continue;
        }
        note_shot(s, p);
    }
}

static void compare_enemies(const fl_state_t *model)
{
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        const fl_enemy_t *e = &model->enemies[i];
        enemy_snap_t *s = &S.enemies[i];
        if (S.jolt[i] > 0) {
            S.jolt[i]--;
        }
        bool same = s->alive && e->alive && s->kind == (uint8_t)e->kind
                    && e->dist >= s->dist - 4.0f && e->dist <= s->dist + 60.0f;
        if (s->alive && !same) {
            float gx = s->x - (float)FL_GOAL.x;
            float gy = s->y - (float)FL_GOAL.y;
            if (gx * gx + gy * gy < 40.0f * 40.0f && s->hp > 0.0f) {
                on_leak();
            } else {
                on_death(s);
            }
            S.jolt[i] = 0;
        }
        if (!e->alive) {
            continue;
        }
        if (!same) {
            continue;   /* just arrived */
        }
        if (e->hp < s->hp - 0.01f) {
            on_hit(i, e, s->hp - e->hp);
        }
        if (e->burrowed != s->burrowed) {
            on_burrow(e);
        }
        if (e->kind == FL_ENEMY_GOLEM && e->stage > s->stage) {
            on_crack(e);
        }
        if (e->ward_ticks > 0 && s->ward == 0) {
            on_ward(e);
        }
        /* Status leaves a trail: embers off the burning, frost off the
         * slowed. */
        if (e->burn_ticks > 0 && (model->tick + (uint32_t)i) % 5u == 0u && !e->burrowed) {
            float bx, by, top;
            enemy_body(e->x, e->y, e->kind, fl_enemy_flies(e), &bx, &by, &top);
            emit(P_EMBER, bx + frand(-4.0f, 4.0f), by - 2.0f, frand(-0.2f, 0.2f),
                 -frand(0.5f, 0.9f), 14, 1);
        }
        if (e->slow_ticks > 0 && (model->tick + (uint32_t)i) % 9u == 0u && !e->burrowed) {
            emit(P_SHARD, e->x + frand(-5.0f, 5.0f), e->y - frand(2.0f, 8.0f),
                 0.0f, 0.2f, 10, 1);
        }
    }
}

static void compare_towers(const fl_state_t *model)
{
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        const fl_tower_t *t = &model->towers[i];
        tower_snap_t *s = &S.towers[i];
        if (s->active && !t->active) {
            on_sold(s);
            continue;
        }
        if (!t->active) {
            continue;
        }
        bool fresh = !s->active || s->x != t->x || s->y != t->y
                     || s->kind != (uint8_t)t->kind;
        if (fresh) {
            on_built(t);
            continue;
        }
        if (t->level > s->level) {
            on_upgraded(t);
        }
        /* The shot timer starts again from zero on every shot. */
        bool fired = t->shot_tick < s->shot_tick
                     || (t->shot_tick == 0 && s->shot_tick != 0);
        if (fired && t->kind != FL_TOWER_BARRACKS && t->frozen_ticks == 0) {
            on_muzzle(t);
        }
    }
}

static void compare_arcs(const fl_state_t *model)
{
    for (int i = 0; i < FL_MAX_ARCS; ++i) {
        const fl_arc_t *arc = &model->arcs[i];
        if (arc->ttl <= S.arc_ttl[i] || arc->count < 2) {
            continue;
        }
        /* A fresh bolt or beam: sparks on everything it struck. */
        for (int k = 1; k < arc->count; ++k) {
            float x = (float)arc->x[k];
            float y = (float)arc->y[k];
            if (arc->kind == FL_TOWER_STORM) {
                burst(P_SPARK, x, y, 4, 1.0f, 2.2f, 5, 1, 0.3f);
            } else if (arc->kind == FL_TOWER_RUNE) {
                burst(P_TWINKLE, x, y, 2, 0.4f, 1.0f, 8, 3, 0.4f);
                ring(x, y, 3, 1.2f, 8);
            } else if (arc->kind == FL_ARC_FREEZE) {
                burst(P_SHARD, (float)arc->x[0], (float)arc->y[0], 8, 1.0f, 2.2f,
                      14, 2, 0.6f);
            }
        }
    }
}

static void compare_heroes(const fl_state_t *model)
{
    for (int i = 0; i < FL_MAX_HEROES; ++i) {
        const fl_hero_t *h = &model->heroes[i];
        bool alive = h->active && h->alive;
        if (S.hero_alive[i] && !alive) {
            for (int k = 0; k < 6; ++k) {
                emit(P_DUST, (float)h->x + frand(-10.0f, 10.0f), (float)h->y - 4.0f,
                     frand(-0.4f, 0.4f), -0.2f, 24, 4);
            }
            emit(P_WISP, (float)h->x, (float)h->y - 30.0f, 0.0f, -0.6f, 34, 2);
            continue;
        }
        if (!alive || !S.hero_alive[i]) {
            continue;
        }
        if (h->skill_timer > S.hero_skill[i] + 10u) {
            on_nova(h);
        }
        if (h->cooldown > S.hero_cooldown[i] + 2u) {
            on_hero_strike(h);
        }
    }
}

/* Run the particles one tick on. */
static void step_particles(void)
{
    for (int i = 0; i < FX_PARTICLES; ++i) {
        particle_t *p = &S.particles[i];
        if (p->life == 0) {
            continue;
        }
        if (p->delay > 0) {
            p->delay--;
            continue;
        }
        p->age++;
        if (p->age >= p->life) {
            p->life = 0;
            continue;
        }
        switch (p->kind) {
        case P_DEBRIS:
        case P_CLOD:
        case P_DROP:
        case P_COIN:
            if (p->landed) {
                break;
            }
            p->vy += p->kind == P_COIN ? 0.18f : 0.32f;
            p->x += p->vx;
            p->y += p->vy;
            if (p->kind != P_COIN && p->vy > 0.0f && p->y >= p->floor) {
                p->y = p->floor;
                if (p->kind == P_DROP) {
                    /* A drop splashes flat where it lands. */
                    p->landed = 1;
                } else if (fabsf(p->vy) > 1.2f) {
                    p->vy *= -0.38f;
                    p->vx *= 0.6f;
                } else {
                    p->landed = 1;
                }
            }
            break;
        case P_FEATHER:
            if (p->landed) {
                break;
            }
            p->vx *= 0.9f;
            p->vy = p->vy < 0.45f ? p->vy + 0.08f : 0.45f;
            p->x += p->vx + sinf((float)p->age * 0.25f + (float)p->spin) * 0.6f;
            p->y += p->vy;
            if (p->y >= p->floor) {
                p->y = p->floor;
                p->landed = 1;
            }
            break;
        case P_SPARK:
        case P_SHARD:
            p->x += p->vx;
            p->y += p->vy;
            p->vx *= 0.82f;
            p->vy = p->vy * 0.82f + (p->kind == P_SHARD ? 0.08f : 0.0f);
            break;
        case P_EMBER:
        case P_WISP:
            p->x += p->vx + sinf((float)p->age * 0.4f + (float)p->spin) * 0.35f;
            p->y += p->vy;
            break;
        case P_SMOKE:
        case P_DUST:
        case P_TWINKLE:
            p->x += p->vx;
            p->y += p->vy;
            p->vx *= 0.9f;
            p->vy *= 0.94f;
            break;
        default:
            break;
        }
    }
    for (int i = 0; i < FX_DECALS; ++i) {
        decal_t *d = &S.decals[i];
        if (d->life > 0 && ++d->age >= d->life) {
            d->life = 0;
        }
    }
    if (S.shake_ticks > 0) {
        S.shake_ticks--;
        if (S.shake_ticks == 0) {
            S.shake = 0;
        }
    }
}

void fx_update(const fl_state_t *model)
{
    if (S.primed && model->run_serial != S.run_serial) {
        fx_reset();
    }
    if (!S.primed) {
        S.primed = true;
        S.run_serial = model->run_serial;
        S.tick = model->tick;
        snapshot(model);
        for (int i = 0; i < FL_MAX_PROJECTILES; ++i) {
            if (model->projectiles[i].active) {
                start_shot(&S.shots[i], &model->projectiles[i]);
            }
        }
        return;
    }
    if (model->tick == S.tick) {
        return;   /* the same frame drawn again */
    }
    S.tick = model->tick;
    compare_towers(model);
    compare_shots(model);
    compare_enemies(model);
    compare_arcs(model);
    compare_heroes(model);
    snapshot(model);
    step_particles();
}

/* ------------------------------------------------------------------------
 * Drawing
 * --------------------------------------------------------------------- */

static void draw_decal(const decal_t *d)
{
    float fade = 1.0f - (float)d->age / (float)d->life;
    switch (d->kind) {
    case D_SCORCH:
        draw_dither_ellipse(d->x, d->y, d->rx, d->ry, 0.55f * fade, FL_INK);
        break;
    case D_CRATER:
        /* A ring of thrown earth round a darker pit. */
        draw_dither_ellipse(d->x, d->y, d->rx, d->ry, 0.35f * fade, FL_INK);
        draw_dither_ellipse(d->x, d->y, d->rx / 2 + 1, d->ry / 2 + 1, 0.7f * fade,
                            FL_INK);
        if (fade > 0.4f) {
            DrawEllipseLines(d->x, d->y, (float)d->rx, (float)d->ry, FL_INK);
        }
        break;
    case D_SPLAT:
        draw_dither_ellipse(d->x, d->y, d->rx, d->ry, 0.8f * fade, FL_PAPER);
        if (fade > 0.3f) {
            DrawEllipseLines(d->x, d->y, (float)d->rx, (float)d->ry, FL_INK);
        }
        break;
    case D_RIME:
    default:
        draw_dither_ellipse(d->x, d->y, d->rx, d->ry, 0.45f * fade, FL_PAPER);
        break;
    }
}

void fx_draw_ground(const fl_state_t *model)
{
    (void)model;
    for (int i = 0; i < FX_DECALS; ++i) {
        if (S.decals[i].life > 0) {
            draw_decal(&S.decals[i]);
        }
    }
}

/* A plus-shaped glint: ink case, paper core. */
static void draw_glint(int x, int y, int r)
{
    if (r <= 0) {
        DrawRectangle(x, y, 1, 1, FL_PAPER);
        return;
    }
    DrawRectangle(x - r - 1, y - 1, r * 2 + 3, 3, FL_INK);
    DrawRectangle(x - 1, y - r - 1, 3, r * 2 + 3, FL_INK);
    DrawRectangle(x - r, y, r * 2 + 1, 1, FL_PAPER);
    DrawRectangle(x, y - r, 1, r * 2 + 1, FL_PAPER);
}

static void draw_particle(const particle_t *p, uint32_t tick)
{
    int x = (int)lroundf(p->x);
    int y = (int)lroundf(p->y);
    float t = (float)p->age / (float)p->life;
    switch (p->kind) {
    case P_SPARK: {
        int ex = (int)lroundf(p->x - p->vx * 2.0f);
        int ey = (int)lroundf(p->y - p->vy * 2.0f);
        if (t < 0.5f) {
            DrawLineEx((Vector2){(float)x, (float)y}, (Vector2){(float)ex, (float)ey},
                       3.0f, FL_INK);
        }
        DrawLine(x, y, ex, ey, FL_PAPER);
        break;
    }
    case P_DEBRIS:
    case P_CLOD: {
        /* Settled chips blink out over their last ticks. */
        if (p->landed && p->life - p->age < 8 && (p->age / 2u) % 2u) {
            break;
        }
        int s = p->size + 1;
        DrawRectangle(x - 1, y - 1, s + 1, s + 1, FL_INK);
        if (p->kind == P_DEBRIS && s > 1) {
            int corner = (p->age / 3 + (uint8_t)p->spin) % 2;
            DrawRectangle(x + corner, y, 1, 1, FL_PAPER);
        }
        break;
    }
    case P_DUST:
        draw_dither_disc(x, y, p->size + p->age / 3, 0.45f * (1.0f - t), FL_INK);
        break;
    case P_SMOKE: {
        int r = p->size + p->age / 4;
        draw_dither_disc(x, y, r, 0.5f * (1.0f - t), FL_INK);
        if (t < 0.5f) {
            draw_dither_disc(x - 1, y - 1, r / 2, 0.4f * (1.0f - t * 2.0f), FL_PAPER);
        }
        break;
    }
    case P_RING: {
        int r = p->size + (int)(p->vx * (float)p->age);
        if (t < 0.6f) {
            DrawCircleLines(x, y, (float)(r + 1), FL_INK);
            DrawCircleLines(x, y, (float)r, FL_PAPER);
            DrawCircleLines(x, y, (float)(r - 1), FL_INK);
        } else {
            draw_dotted_circle(x, y, r, (int)(tick * 4u));
        }
        break;
    }
    case P_FEATHER: {
        if (p->landed && p->life - p->age < 12 && (p->age / 2u) % 2u) {
            break;
        }
        float a = p->landed ? 0.3f : sinf((float)p->age * 0.25f + (float)p->spin) * 0.9f;
        float ca = cosf(a) * 3.0f;
        float sa = sinf(a) * 3.0f;
        Vector2 a0 = {(float)x - ca, (float)y - sa};
        Vector2 a1 = {(float)x + ca, (float)y + sa};
        DrawLineEx(a0, a1, 3.0f, FL_INK);
        DrawLineV(a0, a1, FL_PAPER);
        break;
    }
    case P_SHARD:
        draw_shard(x, y, t < 0.5f ? p->size : (p->size > 1 ? p->size - 1 : 1));
        break;
    case P_EMBER:
        DrawRectangle(x, y, 1, 1, ((p->age + (uint8_t)p->spin) / 2u) % 2u ? FL_PAPER : FL_INK);
        if (t < 0.4f) {
            DrawRectangle(x, y + 1, 1, 1, FL_INK);
        }
        break;
    case P_DROP:
        if (p->landed) {
            DrawRectangle(x - 2, y, 5, 1, FL_INK);
            DrawRectangle(x - 1, y, 3, 1, FL_PAPER);
        } else {
            DrawCircle(x, y, (float)p->size + 1.0f, FL_INK);
            DrawRectangle(x, y - 1, 1, 1, FL_PAPER);
        }
        break;
    case P_TWINKLE: {
        /* Swells, then shrinks back to a point. */
        float s = sinf(t * PI_F);
        draw_glint(x, y, (int)lroundf((float)p->size * s));
        break;
    }
    case P_FLASH: {
        int r = (int)((float)p->size * (1.0f - t * 0.6f));
        for (int k = 0; k < 8; ++k) {
            float a = (float)k * 0.7853982f + (float)(p->spin) * 0.2f;
            float len = (float)r * (k % 2 ? 0.6f : 1.0f);
            Vector2 tip = {(float)x + cosf(a) * len, (float)y + sinf(a) * len};
            DrawLineEx((Vector2){(float)x, (float)y}, tip, 3.0f, FL_INK);
            DrawLineV((Vector2){(float)x, (float)y}, tip, FL_PAPER);
        }
        if (t < 0.5f) {
            DrawCircle(x, y, (float)r * 0.4f + 1.0f, FL_PAPER);
        }
        break;
    }
    case P_COIN: {
        /* A spinning coin: it narrows and widens as it turns. */
        int w = (int)(fabsf(cosf((float)p->age * 0.5f)) * 2.0f) + 1;
        DrawRectangle(x - w - 1, y - 3, w * 2 + 3, 7, FL_INK);
        DrawRectangle(x - w, y - 2, w * 2 + 1, 5, FL_PAPER);
        if (w > 1) {
            DrawRectangle(x, y - 1, 1, 3, FL_INK);
        }
        break;
    }
    case P_WISP:
        DrawRectangle(x - 1, y - 1, 3, 3, FL_INK);
        DrawRectangle(x, y, 1, 1, FL_PAPER);
        if (t < 0.7f) {
            DrawRectangle(x, y + 2, 1, 2, FL_PAPER);
        }
        break;
    default:
        break;
    }
}

void fx_draw_air(const fl_state_t *model)
{
    for (int i = 0; i < FX_PARTICLES; ++i) {
        const particle_t *p = &S.particles[i];
        if (p->life == 0 || p->delay > 0) {
            continue;
        }
        draw_particle(p, model->tick);
    }
}

/* --- shots in flight ----------------------------------------------------- */

/* An arrow along its flight: shaft, head and fletching; a ballista bolt
 * is longer and heavier. */
static void draw_arrow(float x, float y, float dx, float dy, bool bolt,
                       uint32_t tick)
{
    float len = bolt ? 8.0f : 5.5f;
    Vector2 head = {x + dx * len, y + dy * len};
    Vector2 tail = {x - dx * len, y - dy * len};
    float nx = -dy;
    float ny = dx;
    /* A faint wake behind it. */
    for (int k = 1; k <= 2; ++k) {
        int wx = (int)lroundf(tail.x - dx * (float)(k * 4));
        int wy = (int)lroundf(tail.y - dy * (float)(k * 4));
        if (((unsigned)k + tick) % 2u == 0u) {
            DrawRectangle(wx, wy, 1, 1, FL_PAPER);
        }
    }
    DrawLineEx(tail, head, bolt ? 4.0f : 3.0f, FL_INK);
    DrawLineV(tail, head, FL_PAPER);
    float hl = bolt ? 4.0f : 3.0f;
    Vector2 tip = {head.x + dx * hl, head.y + dy * hl};
    Vector2 l = {head.x + nx * hl * 0.8f, head.y + ny * hl * 0.8f};
    Vector2 r = {head.x - nx * hl * 0.8f, head.y - ny * hl * 0.8f};
    DrawTriangle(tip, l, r, FL_INK);
    DrawLineV(head, tip, FL_PAPER);
    /* Fletching: two short vanes swept back. */
    for (int s = -1; s <= 1; s += 2) {
        Vector2 vane = {tail.x - dx * 3.0f + nx * 2.5f * (float)s,
                        tail.y - dy * 3.0f + ny * 2.5f * (float)s};
        DrawLineV(tail, vane, FL_INK);
    }
}

/* A frost bolt: a spinning crystal with a glinting core. */
static void draw_frost_bolt(float x, float y, uint32_t tick, int seed)
{
    int ix = (int)lroundf(x);
    int iy = (int)lroundf(y);
    float spin = (float)tick * 0.6f + (float)seed;
    Vector2 pts[4];
    for (int k = 0; k < 4; ++k) {
        float a = spin + (float)k * 1.5707963f;
        float r = k % 2 ? 2.5f : 5.0f;
        pts[k] = (Vector2){x + cosf(a) * r, y + sinf(a) * r};
    }
    for (int k = 0; k < 4; ++k) {
        DrawLineEx(pts[k], pts[(k + 1) % 4], 3.0f, FL_INK);
    }
    DrawTriangle(pts[0], pts[1], pts[2], FL_PAPER);
    DrawTriangle(pts[0], pts[2], pts[3], FL_PAPER);
    if ((tick + (unsigned)seed) % 4u < 2u) {
        DrawRectangle(ix, iy - 1, 1, 3, FL_INK);
        DrawRectangle(ix - 1, iy, 3, 1, FL_INK);
    }
}

/* A boulder in flight: a rough stone turning over, a crack across it. */
static void draw_boulder(float x, float y, float r, uint32_t tick, bool burning)
{
    int ix = (int)lroundf(x);
    int iy = (int)lroundf(y);
    DrawCircle(ix, iy, r + 1.0f, FL_INK);
    DrawCircle(ix, iy, r, FL_PAPER);
    float a = (float)tick * 0.45f;
    /* Shade the far half with a dither, then the crack. */
    for (int dy = 0; dy <= (int)r; ++dy) {
        int half = (int)sqrtf(r * r - (float)(dy * dy));
        for (int dx = -half; dx <= half; ++dx) {
            if (((ix + dx + iy + dy) & 1) == 0) {
                DrawRectangle(ix + dx, iy + dy, 1, 1, FL_INK);
            }
        }
    }
    Vector2 c0 = {x + cosf(a) * r * 0.8f, y + sinf(a) * r * 0.8f};
    Vector2 c1 = {x - cosf(a) * r * 0.5f, y - sinf(a) * r * 0.5f};
    DrawLineV(c0, c1, FL_INK);
    DrawRectangle(ix - 1, iy - (int)r + 1, 2, 1, FL_PAPER);
    if (burning) {
        int frame = (int)((tick / 2u) % EMBER_FLAME_FRAMES);
        draw_padded(fv_assets.fx, fv_assets.ember_flame[frame],
                    x - PROJECTILE_DRAW_SIZE / 2.0f, y - PROJECTILE_DRAW_SIZE + 2.0f,
                    PROJECTILE_DRAW_SIZE);
    }
}

/* Lobbed shots fly a curve: the model moves them along the ground, the
 * view lifts them by a sine of how far they have come. */
static float lob_height(const shot_snap_t *s, float x, float y, float tx, float ty)
{
    float dx = tx - x;
    float dy = ty - y;
    float left = sqrtf(dx * dx + dy * dy);
    float progress = 1.0f - left / s->total;
    if (progress < 0.0f) {
        progress = 0.0f;
    } else if (progress > 1.0f) {
        progress = 1.0f;
    }
    float peak = s->total * 0.42f;
    if (peak > 56.0f) {
        peak = 56.0f;
    }
    /* It leaves from the arm's height and drops onto the target. */
    return sinf(progress * PI_F) * peak + (1.0f - progress) * 6.0f;
}

void fx_draw_projectiles(const fl_state_t *model)
{
    for (int i = 0; i < FL_MAX_PROJECTILES; ++i) {
        const fl_projectile_t *shot = &model->projectiles[i];
        if (!shot->active) {
            continue;
        }
        float dx = shot->tx - shot->x;
        float dy = shot->ty - shot->y;
        float length = sqrtf(dx * dx + dy * dy);
        if (length > 0.5f) {
            dx /= length;
            dy /= length;
        } else {
            dx = 1.0f;
            dy = 0.0f;
        }
        const shot_snap_t *snap = &S.shots[i];
        switch (shot->kind) {
        case FL_TOWER_GUARD:
            draw_arrow(shot->x, shot->y, dx, dy, (shot->flags & FL_SHOT_PIERCE) != 0,
                       model->tick);
            break;
        case FL_TOWER_FROST:
            draw_frost_bolt(shot->x, shot->y, model->tick, i);
            break;
        case FL_TOWER_SIEGE:
        case FL_TOWER_EMBER: {
            float h = snap->active ? lob_height(snap, shot->x, shot->y, shot->tx,
                                                shot->ty)
                                   : 0.0f;
            /* The shadow on the ground shrinks as the shot climbs. */
            int sr = 4 - (int)(h / 20.0f);
            draw_ground_shadow((int)shot->x, (int)shot->y + 2, sr < 2 ? 2 : sr, 1);
            float sy = shot->y - h;
            if (shot->kind == FL_TOWER_SIEGE) {
                float r = shot->aoe > 24 ? 4.5f : 3.5f;
                draw_boulder(shot->x, sy, r, model->tick + (uint32_t)i * 5u,
                             (shot->flags & FL_SHOT_BURN) != 0);
            } else {
                /* The fire pot tumbles and sheds sparks behind it. */
                for (int k = 1; k <= 3; ++k) {
                    int tx = (int)shot->x - (int)lroundf(dx * (float)(k * 5));
                    int ty = (int)sy - (int)lroundf(dy * (float)(k * 5)) + k
                             - (int)((model->tick + (unsigned)k) % 2u);
                    Color c = ((unsigned)k + model->tick / 2u) % 2u ? FL_PAPER : FL_INK;
                    DrawRectangle(tx, ty, 4 - k, 4 - k, c);
                }
                int frame = (int)((model->tick / 3u + (unsigned)i) % EMBER_FLAME_FRAMES);
                draw_padded(fv_assets.fx, fv_assets.ember_pot[frame],
                            shot->x - PROJECTILE_DRAW_SIZE / 2.0f,
                            sy - PROJECTILE_DRAW_SIZE / 2.0f, PROJECTILE_DRAW_SIZE);
            }
            break;
        }
        default:
            draw_padded(fv_assets.fx, fv_assets.projectile,
                        shot->x - PROJECTILE_DRAW_SIZE / 2.0f,
                        shot->y - PROJECTILE_DRAW_SIZE / 2.0f, PROJECTILE_DRAW_SIZE);
            break;
        }
    }
}

/* --- status marks over enemies ------------------------------------------ */

void fx_draw_status(const fl_state_t *model)
{
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        const fl_enemy_t *e = &model->enemies[i];
        if (!e->alive || e->burrowed) {
            continue;
        }
        float bx, by, top;
        int jx, jy;
        fx_enemy_jolt(i, &jx, &jy);
        enemy_body(e->x + (float)jx, e->y + (float)jy, e->kind, fl_enemy_flies(e),
                   &bx, &by, &top);
        if (e->burn_ticks > 0) {
            /* Flames lick up its back. */
            int frame = (int)((model->tick / 2u + (unsigned)i) % EMBER_FLAME_FRAMES);
            draw_padded(fv_assets.fx, fv_assets.ember_flame[frame],
                        bx - PROJECTILE_DRAW_SIZE / 2.0f + 3.0f,
                        by - PROJECTILE_DRAW_SIZE + 4.0f, PROJECTILE_DRAW_SIZE);
        }
        if (e->stun_ticks > 0) {
            /* Three little stars wheel round its head. */
            for (int k = 0; k < 3; ++k) {
                float a = (float)model->tick * 0.3f + (float)k * 2.0943951f;
                int sx = (int)lroundf(bx + cosf(a) * 7.0f);
                int sy = (int)lroundf(top - 3.0f + sinf(a) * 2.0f);
                draw_glint(sx, sy, 1);
            }
        }
        if (e->hex_ticks > 0) {
            /* A turning rune: a diamond with a dot in its heart. */
            float a = (float)model->tick * 0.15f;
            int cx = (int)bx;
            int cy = (int)top - 6;
            Vector2 pts[4];
            for (int k = 0; k < 4; ++k) {
                float b = a + (float)k * 1.5707963f;
                pts[k] = (Vector2){(float)cx + cosf(b) * 4.0f,
                                   (float)cy + sinf(b) * 2.5f};
            }
            for (int k = 0; k < 4; ++k) {
                DrawLineEx(pts[k], pts[(k + 1) % 4], 3.0f, FL_INK);
            }
            for (int k = 0; k < 4; ++k) {
                DrawLineV(pts[k], pts[(k + 1) % 4], FL_PAPER);
            }
            DrawRectangle(cx, cy, 1, 1, FL_PAPER);
        }
    }
}
