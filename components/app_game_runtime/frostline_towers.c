/* The towers on the battlefield: the body, the parts that move on it, and
 * the tells of a shot gathering and leaving.
 *
 * Every point on a tower comes from data/frostline_tower_rig.h, generated
 * from the rig the art is drawn to (assets_src/artgen/tower_rig.py), so a
 * bolt leaves the ballista the player sees and a boulder sits in the bucket.
 *
 * Two clocks drive all the motion, and both belong to the model:
 *   - shot_tick counts up from the moment a tower fires: the release;
 *   - the cooldown left over its full length is how far the reload has
 *     come, so a frozen tower stops mid-reload without any state here.
 * A key pose holds at least three ticks, so 3x speed, which runs three
 * ticks a frame, still shows every one.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#include "private/frostline_view_internal.h"
#include "../app_game/data/frostline_tower_rig.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* Parts are 32 px cells centred on the point they hang from; the door's
 * anchor is the middle of its sill instead. These match PART_CENTER and
 * DOOR_ANCHOR in assets_src/artgen/tower_art.py. */
#define PART_DRAW_SIZE 32.0f
#define PART_CENTER 16.0f
#define DOOR_ANCHOR_Y 24.0f

/* The catapult's poses, in the order of SIEGE_POSES in tower_rig.py. */
enum {
    SIEGE_COCKED,   /* arm down and back, a stone in the bucket */
    SIEGE_SWING,    /* the throw: the arm half way, a smear behind it */
    SIEGE_STOP,     /* the arm slammed into the padded stop */
    SIEGE_BOUNCE,   /* knocked back off it */
    SIEGE_WIND,     /* winched half way down again */
    SIEGE_POSE_COUNT,
};
static const char *const SIEGE_POSE_NAMES[SIEGE_POSE_COUNT] = {
    "cocked", "swing", "stop", "bounce", "wind",
};

/* The kiln's fire: four flickers and the surge as a pot goes up. */
#define EMBER_FLICKERS 4

/* The guard tower's turning head: what is mounted, and where it is in its
 * shot. The order matches GUARD_HEADS and GUARD_STATES in tower_rig.py. */
enum { HEAD_CROSSBOW, HEAD_BALLISTA, HEAD_REPEATER, HEAD_COUNT };
enum { HEAD_LOADED, HEAD_LOOSED, HEAD_DRAWN, HEAD_STATE_COUNT };
static const char *const HEAD_NAMES[HEAD_COUNT] = {
    "crossbow", "ballista", "repeater",
};
static const char *const HEAD_STATE_NAMES[HEAD_STATE_COUNT] = {
    "loaded", "loosed", "drawn",
};

static const char *const VARIANT_NAMES[FL_TOWER_VARIANTS] = {
    "lv1", "lv2", "lv3a", "lv3b",
};

static struct {
    raylib_lite_atlas_t parts;
    raylib_lite_asset_id_t still[FL_TOWER_KIND_COUNT][FL_TOWER_VARIANTS];
    raylib_lite_asset_id_t siege[FL_TOWER_VARIANTS][SIEGE_POSE_COUNT][2];
    raylib_lite_asset_id_t ember[FL_TOWER_VARIANTS][EMBER_FLICKERS + 1];
    raylib_lite_asset_id_t head[HEAD_COUNT][HEAD_STATE_COUNT][FL_GUARD_DIRECTIONS];
    raylib_lite_asset_id_t orbit[FL_TOWER_KIND_COUNT];
    raylib_lite_asset_id_t spare_pot;
    raylib_lite_asset_id_t door_open;
} T;

void towers_init(void)
{
    memset(&T, 0, sizeof(T));
    T.parts = raylib_lite_atlas_load("parts.atlas");
    if (T.parts.texture.id == 0) {
        printf("frostline: cannot load parts.atlas\n");
    }
    char name[48];
    for (int kind = FL_TOWER_GUARD; kind < FL_TOWER_KIND_COUNT; ++kind) {
        for (int v = 0; v < FL_TOWER_VARIANTS; ++v) {
            snprintf(name, sizeof(name), "tower_%s_%s", FL_TOWER_KEYS[kind],
                     VARIANT_NAMES[v]);
            T.still[kind][v] = raylib_lite_asset_id(name);
        }
    }
    for (int v = 0; v < FL_TOWER_VARIANTS; ++v) {
        for (int pose = 0; pose < SIEGE_POSE_COUNT; ++pose) {
            for (int left = 0; left < 2; ++left) {
                snprintf(name, sizeof(name), "tower_siege_%s_%s_%c",
                         VARIANT_NAMES[v], SIEGE_POSE_NAMES[pose],
                         left ? 'l' : 'r');
                T.siege[v][pose][left] = raylib_lite_asset_id(name);
            }
        }
        for (int frame = 0; frame <= EMBER_FLICKERS; ++frame) {
            if (frame == EMBER_FLICKERS) {
                snprintf(name, sizeof(name), "tower_ember_%s_surge",
                         VARIANT_NAMES[v]);
            } else {
                snprintf(name, sizeof(name), "tower_ember_%s_%d",
                         VARIANT_NAMES[v], frame);
            }
            T.ember[v][frame] = raylib_lite_asset_id(name);
        }
    }
    for (int head = 0; head < HEAD_COUNT; ++head) {
        for (int state = 0; state < HEAD_STATE_COUNT; ++state) {
            for (int dir = 0; dir < FL_GUARD_DIRECTIONS; ++dir) {
                snprintf(name, sizeof(name), "head_%s_%s_%d", HEAD_NAMES[head],
                         HEAD_STATE_NAMES[state], dir);
                T.head[head][state][dir] = raylib_lite_asset_id(name);
            }
        }
    }
    T.orbit[FL_TOWER_FROST] = raylib_lite_asset_id("orbit_shard");
    T.orbit[FL_TOWER_STORM] = raylib_lite_asset_id("orbit_stone");
    T.orbit[FL_TOWER_RUNE] = raylib_lite_asset_id("orbit_rune");
    T.spare_pot = raylib_lite_asset_id("spare_pot");
    T.door_open = raylib_lite_asset_id("door_open");
}

void towers_shutdown(void)
{
    raylib_lite_atlas_unload(T.parts);
    memset(&T, 0, sizeof(T));
}

bool towers_ready(void)
{
    return T.parts.texture.id != 0;
}

/* ------------------------------------------------------------------------
 * The two clocks
 * --------------------------------------------------------------------- */

/* How far the reload has come: 0 just after a shot, 1 ready to fire. */
static float reload_of(const fl_tower_t *tower, const fl_tower_stats_t *stat)
{
    if (tower->cooldown == 0 || stat->cooldown == 0) {
        return 1.0f;
    }
    float left = (float)tower->cooldown / (float)stat->cooldown;
    return left >= 1.0f ? 0.0f : 1.0f - left;
}

/* A rig point on the screen, for a tower facing right unless `mirror`. */
static void rig_point(const fl_tower_t *tower, fl_rig_point_t p, bool mirror,
                      int *x, int *y)
{
    *x = tower->x + (mirror ? -p.dx : p.dx);
    *y = tower->y - p.h;
}

static void draw_part(raylib_lite_asset_id_t id, int x, int y)
{
    draw_padded(T.parts, id, (float)x - PART_CENTER, (float)y - PART_CENTER,
                PART_DRAW_SIZE);
}

/* ------------------------------------------------------------------------
 * The catapult
 * --------------------------------------------------------------------- */

/* The throw lands each pose for a few ticks, then the reload winches the
 * arm down; it waits cocked once it is half way. */
static int siege_pose(const fl_tower_t *tower, float reload)
{
    uint8_t t = tower->shot_tick;
    if (t < 3) {
        return SIEGE_SWING;
    }
    if (t < 7) {
        return SIEGE_STOP;
    }
    if (t < 10) {
        return SIEGE_BOUNCE;
    }
    if (t < 13) {
        return SIEGE_STOP;
    }
    return reload < 0.5f ? SIEGE_WIND : SIEGE_COCKED;
}

static void draw_siege(const fl_state_t *model, const fl_tower_t *tower,
                       int variant, const fl_tower_stats_t *stat, float x,
                       float y)
{
    float reload = reload_of(tower, stat);
    int pose = siege_pose(tower, reload);
    bool left = fl_tower_faces_left(tower);
    /* The whole frame jumps a pixel as the arm hits its stop. */
    float kick = (tower->shot_tick >= 3 && tower->shot_tick < 5) ? 1.0f : 0.0f;
    draw_padded(fv_assets.units, T.siege[variant][pose][left ? 1 : 0], x,
                y + kick, TOWER_DRAW_SIZE);

    /* The next stone is heaved into the bucket late in the reload; it
     * drops in a pixel high and settles. */
    if (pose != SIEGE_COCKED || reload < 0.8f) {
        return;
    }
    int lx, ly;
    rig_point(tower, FL_TOWER_RIGS[tower->kind][variant].load, left, &lx, &ly);
    float r = stat->aoe > 24 ? 4.5f : 3.5f;
    float drop = reload < 0.88f ? 1.0f : 0.0f;
    /* The tick turns the stone's crack and works its flames: a stone at
     * rest only burns. */
    bool burning = stat->burn != 0;
    fx_draw_boulder((float)lx, (float)ly - drop, r, burning ? model->tick : 0u,
                    burning, stat->stun != 0);
}

/* ------------------------------------------------------------------------
 * The guard tower's ballista
 * --------------------------------------------------------------------- */

static int guard_head(const fl_tower_t *tower)
{
    if (tower->level < 2) {
        return HEAD_CROSSBOW;
    }
    return fl_tower_option(tower, 0) == 1 ? HEAD_REPEATER : HEAD_BALLISTA;
}

/* The bow springs straight and kicks back as it looses, is hauled back
 * bare, and has a bolt laid in once the reload is well on. */
static void draw_guard_head(const fl_tower_t *tower, int variant, float reload)
{
    int state = HEAD_LOADED;
    if (tower->shot_tick < 5) {
        state = HEAD_LOOSED;
    } else if (reload < 0.6f) {
        state = HEAD_DRAWN;
    }
    int mx, my;
    rig_point(tower, FL_TOWER_RIGS[tower->kind][variant].muzzle, false, &mx, &my);
    draw_part(T.head[guard_head(tower)][state][tower->aim & 7u], mx, my);
}

/* ------------------------------------------------------------------------
 * The kiln
 * --------------------------------------------------------------------- */

static void draw_ember(const fl_state_t *model, const fl_tower_t *tower,
                       int index, int variant, float reload, float x, float y)
{
    /* Each kiln flickers on its own beat; a shot throws the fire up. */
    int frame = (int)((model->tick / 4u + (unsigned)index * 3u) % EMBER_FLICKERS);
    if (tower->shot_tick < 5) {
        frame = EMBER_FLICKERS;
    }
    if (tower->frozen_ticks > 0) {
        frame = 0;
    }
    float kick = tower->shot_tick < 3 ? 1.0f : 0.0f;
    draw_padded(fv_assets.units, T.ember[variant][frame], x, y + kick,
                TOWER_DRAW_SIZE);

    /* The spare pots stacked by the kiln: the one on the side it threw
     * towards is gone until the reload brings another. */
    const fl_tower_rig_t *rig = &FL_TOWER_RIGS[tower->kind][variant];
    int thrown = fl_tower_faces_left(tower) ? 0 : 1;
    for (int k = 0; k < 2; ++k) {
        if (rig->spare[k].dx == 0 && rig->spare[k].h == 0) {
            continue;
        }
        if (k == thrown && reload < 0.7f) {
            continue;
        }
        int px, py;
        rig_point(tower, rig->spare[k], false, &px, &py);
        draw_part(T.spare_pot, px, py);
    }
}

/* ------------------------------------------------------------------------
 * Things circling a tower, and the tells of a charge
 * --------------------------------------------------------------------- */

/* Shards round the nova, charged stones round the static spire, rune
 * blocks round the obelisk: the far half of the ring passes behind the
 * tower, the near half in front. A shot throws the ring wide; a frozen
 * tower's ring drops to the ground and lies still. */
static void draw_orbiters(const fl_state_t *model, const fl_tower_t *tower,
                          int index, int variant, bool near_half)
{
    const fl_tower_rig_t *rig = &FL_TOWER_RIGS[tower->kind][variant];
    int count = rig->orbit_count;
    if (count == 0) {
        return;
    }
    float turn = tower->kind == FL_TOWER_STORM   ? 0.09f
                 : tower->kind == FL_TOWER_FROST ? 0.05f
                                                 : 0.035f;
    float t = (float)model->tick + (float)index * 17.0f;
    bool frozen = tower->frozen_ticks > 0;
    float spread = 0.0f;
    if (tower->shot_tick < 8 && !frozen) {
        spread = 5.0f * (1.0f - (float)tower->shot_tick / 8.0f);
    }
    float cx = (float)tower->x - 1.0f;
    float cy = (float)(tower->y - rig->orbit_h);
    for (int k = 0; k < count; ++k) {
        float a = (float)k * 6.2831853f / (float)count;
        if (!frozen) {
            a += t * turn;
        }
        float s = sinf(a);
        if ((s >= 0.0f) != near_half) {
            continue;
        }
        float px = cx + cosf(a) * ((float)rig->orbit_rx + spread);
        float py = cy + s * (float)rig->orbit_ry;
        if (frozen) {
            py = (float)tower->y - 2.0f + s * (float)rig->orbit_ry;
        } else if (tower->kind != FL_TOWER_FROST) {
            /* Stones bob as they go; the nova's shards hold their height. */
            py += sinf(t * 0.12f + (float)k * 1.7f) * 1.5f;
        }
        draw_part(T.orbit[tower->kind], (int)lroundf(px), (int)lroundf(py));
    }
}

/* The last ticks of a reload, shown on the tower, so the player sees a
 * shot coming: the rune eye draws motes up the obelisk, a glint climbs
 * the frost crystal, the storm orb crackles. */
static void draw_charge(const fl_state_t *model, const fl_tower_t *tower,
                        int index, int variant)
{
    if (tower->frozen_ticks > 0) {
        return;
    }
    const fl_tower_rig_t *rig = &FL_TOWER_RIGS[tower->kind][variant];
    float mxf, myf;
    fl_tower_muzzle(tower, &mxf, &myf);
    int mx = (int)lroundf(mxf);
    int my = (int)lroundf(myf);
    int base = tower->y - rig->charge_h;
    int cd = (int)tower->cooldown;
    switch (tower->kind) {
    case FL_TOWER_RUNE: {
        enum { WINDOW = 15 };
        if (cd == 0 || cd > WINDOW) {
            break;
        }
        /* Three motes spiral up round the outside of the obelisk and
         * close in on the eye; on the dark stone only a paper cross in an
         * ink case reads. */
        float p0 = (float)(WINDOW - cd) / (float)WINDOW;
        for (int k = 0; k < 3; ++k) {
            float p = fmodf(p0 * 1.5f + (float)k / 3.0f, 1.0f);
            int y = base + (int)lroundf((float)(my - base) * p);
            float swing = sinf(p * 6.0f + (float)k * 2.1f);
            float side = swing < 0.0f ? -1.0f : 1.0f;
            int x = mx + (int)lroundf((swing * 4.0f + side * 5.0f) * (1.0f - p));
            draw_glint(x, y, p > 0.8f ? 2 : 1);
        }
        break;
    }
    case FL_TOWER_FROST: {
        enum { WINDOW = 10 };
        if (cd == 0 || cd > WINDOW) {
            break;
        }
        /* A glint runs up the crystal's lit edge and flares at its tip. */
        float p = (float)(WINDOW - cd) / (float)WINDOW;
        int y = base + (int)lroundf((float)(my - base) * p);
        if (p > 0.7f) {
            draw_glint(mx, my, 2);
        } else {
            draw_glint(mx + 3 - (int)lroundf(3.0f * p), y, 1);
        }
        break;
    }
    case FL_TOWER_STORM: {
        /* Crackle as it charges, and now and then while it waits. */
        enum { WINDOW = 8 };
        bool charging = cd > 0 && cd <= WINDOW;
        uint32_t beat = jag_hash(model->tick / 3u + (uint32_t)index * 7919u);
        bool idle_spark = cd == 0 && beat % 11u == 0u;
        if (!charging && !idle_spark) {
            break;
        }
        int sparks = charging ? 2 : 1;
        for (int k = 0; k < sparks; ++k) {
            uint32_t h = jag_hash(model->tick * 31u + (uint32_t)(index * 5 + k));
            float a = (float)(h % 628u) / 100.0f;
            float r0 = 4.0f;
            float r1 = 7.0f + (float)((h >> 10) % 3u);
            int x0 = mx + (int)lroundf(cosf(a) * r0);
            int y0 = my + (int)lroundf(sinf(a) * r0);
            int x1 = mx + (int)lroundf(cosf(a + 0.4f) * r1);
            int y1 = my + (int)lroundf(sinf(a + 0.4f) * r1);
            DrawLine(x0 - 1, y0, x1 - 1, y1, FL_INK);
            DrawLine(x0 + 1, y0, x1 + 1, y1, FL_INK);
            DrawLine(x0, y0, x1, y1, FL_PAPER);
        }
        break;
    }
    default:
        break;
    }
}

/* The spire's tip and the obelisk's eye flare as they fire. */
static void draw_flare(const fl_tower_t *tower)
{
    if ((tower->kind != FL_TOWER_STORM && tower->kind != FL_TOWER_RUNE)
        || tower->shot_tick >= 6) {
        return;
    }
    float mxf, myf;
    fl_tower_muzzle(tower, &mxf, &myf);
    int mx = (int)lroundf(mxf);
    int my = (int)lroundf(myf);
    int flare = 6 - (int)tower->shot_tick;
    DrawCircle(mx, my, (float)(flare / 2 + 2), FL_INK);
    DrawCircle(mx, my, (float)(flare / 2 + 1), FL_PAPER);
    if (tower->kind == FL_TOWER_STORM) {
        /* Four short rays, turned a little every flare. */
        for (int r = 0; r < 4; ++r) {
            float a = (float)r * 1.5707963f + (float)tower->shot_tick * 0.4f;
            int ex = mx + (int)lroundf(cosf(a) * (float)(flare + 3));
            int ey = my + (int)lroundf(sinf(a) * (float)(flare + 3));
            DrawLine(mx, my, ex, ey, FL_PAPER);
        }
    }
}

/* The hall's door stands open while one of its soldiers is on the
 * doorstep, coming out. */
static void draw_door(const fl_state_t *model, const fl_tower_t *tower,
                      int index, int variant)
{
    bool open = false;
    for (int i = 0; i < FL_MAX_SOLDIERS && !open; ++i) {
        const fl_soldier_t *s = &model->soldiers[i];
        if (!s->active || !s->alive || !s->moving || s->tower != (uint8_t)index) {
            continue;
        }
        float dx = s->x - (float)tower->x;
        float dy = s->y - (float)tower->y;
        open = dx * dx + dy * dy < 18.0f * 18.0f;
    }
    if (!open) {
        return;
    }
    int x, y;
    rig_point(tower, FL_TOWER_RIGS[tower->kind][variant].door, false, &x, &y);
    draw_padded(T.parts, T.door_open, (float)x - PART_CENTER,
                (float)y - DOOR_ANCHOR_Y, PART_DRAW_SIZE);
}

/* ------------------------------------------------------------------------
 * Frost, selection, and the tower as a whole
 * --------------------------------------------------------------------- */

/* Selection: four corner brackets round the sprite, ink with a paper core so
 * they read on snow and on forest alike. */
static void draw_brackets(int x, int y, int w, int h)
{
    const int arm = 6;
    for (int corner = 0; corner < 4; ++corner) {
        bool right = corner & 1;
        bool bottom = corner & 2;
        int cx = right ? x + w - 1 : x;
        int cy = bottom ? y + h - 1 : y;
        int hx = right ? cx - arm + 1 : cx;
        int vy = bottom ? cy - arm + 1 : cy;
        DrawRectangle(hx - 1, cy - 1, arm + 2, 3, FL_INK);
        DrawRectangle(cx - 1, vy - 1, 3, arm + 2, FL_INK);
        DrawRectangle(hx, cy, arm, 1, FL_PAPER);
        DrawRectangle(cx, vy, 1, arm, FL_PAPER);
    }
}

/* One prism of ice: a paper wedge in an ink case, with a facet line down
 * its lit side. */
static void draw_ice_spike(int x, int base, int h, int w)
{
    if (h < 2) {
        return;
    }
    DrawTriangle((Vector2){(float)x, (float)(base - h - 1)},
                 (Vector2){(float)(x - w - 1), (float)(base + 1)},
                 (Vector2){(float)(x + w + 1), (float)(base + 1)}, FL_INK);
    DrawTriangle((Vector2){(float)x, (float)(base - h)},
                 (Vector2){(float)(x - w), (float)base},
                 (Vector2){(float)(x + w), (float)base}, FL_PAPER);
    DrawLine(x, base - h + 2, x + w / 2, base, FL_INK);
}

/* A tower the frost king has frozen: a crust of prisms round its plinth
 * and rime glittering up its body. The prisms sink back as it thaws. */
static void draw_frozen(const fl_tower_t *tower, uint32_t tick)
{
    static const int8_t SPIKES[][3] = {   /* dx, height, half width */
        {-15, 10, 3}, {-9, 18, 4}, {-2, 12, 3}, {5, 21, 4}, {12, 13, 3},
        {17, 8, 2},
    };
    float thaw = (float)tower->frozen_ticks / 24.0f;
    if (thaw > 1.0f) {
        thaw = 1.0f;
    }
    int base = tower->y + 4;
    /* Rime: paper flecks scattered over the body, a few winking. */
    for (int k = 0; k < 14; ++k) {
        uint32_t h = jag_hash((uint32_t)tower->x * 131u + (uint32_t)k * 977u);
        int fx = tower->x - 12 + (int)(h % 25u);
        int fy = tower->y - 6 - (int)((h >> 8) % 34u);
        if (((h >> 16) + tick / 4u) % 7u == 0u) {
            DrawRectangle(fx - 1, fy, 3, 1, FL_PAPER);
            DrawRectangle(fx, fy - 1, 1, 3, FL_PAPER);
        } else {
            DrawRectangle(fx, fy, 1, 1, FL_PAPER);
        }
    }
    for (size_t k = 0; k < sizeof(SPIKES) / sizeof(SPIKES[0]); ++k) {
        draw_ice_spike(tower->x + SPIKES[k][0], base,
                       (int)((float)SPIKES[k][1] * thaw), SPIKES[k][2]);
    }
}

void towers_draw(const fl_state_t *model, int i)
{
    const fl_tower_t *tower = &model->towers[i];
    int kind = (int)tower->kind;
    if (kind <= FL_TOWER_NONE || kind >= FL_TOWER_KIND_COUNT) {
        return;
    }
    int variant = fl_tower_variant(tower);
    fl_tower_stats_t stat = fl_tower_stats(tower);
    float reload = reload_of(tower, &stat);
    float x = (float)tower->x - TOWER_DRAW_SIZE / 2.0f;
    float y = (float)tower->y - TOWER_DRAW_SIZE + SPRITE_FOOT;

    draw_orbiters(model, tower, i, variant, false);
    switch (kind) {
    case FL_TOWER_SIEGE:
        draw_siege(model, tower, variant, &stat, x, y);
        break;
    case FL_TOWER_EMBER:
        draw_ember(model, tower, i, variant, reload, x, y);
        break;
    default:
        draw_padded(fv_assets.units, T.still[kind][variant], x, y,
                    TOWER_DRAW_SIZE);
        break;
    }
    if (kind == FL_TOWER_GUARD) {
        draw_guard_head(tower, variant, reload);
    } else if (kind == FL_TOWER_BARRACKS) {
        draw_door(model, tower, i, variant);
    }
    draw_orbiters(model, tower, i, variant, true);
    draw_charge(model, tower, i, variant);
    draw_flare(tower);

    /* Level pips on the plinth: filled for every level reached. */
    for (int level = 0; level < 3; ++level) {
        int px = tower->x - 10 + level * 7;
        bool filled = level < tower->level;
        DrawRectangle(px, tower->y + 3, 6, 4, FL_INK);
        if (!filled) {
            DrawRectangle(px + 1, tower->y + 4, 4, 2, FL_PAPER);
        }
    }
    if (tower->frozen_ticks > 0) {
        draw_frozen(tower, model->tick);
    }
    if (model->selected_tower == (int8_t)i) {
        draw_brackets((int)x + 4, (int)y, (int)TOWER_DRAW_SIZE - 8,
                      (int)TOWER_DRAW_SIZE);
    }
}
