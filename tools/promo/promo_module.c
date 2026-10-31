// SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary
/* 宣传片录制用的 Host 模块。
 *
 * 复用游戏真实的模型（frostline_model.c）、渲染（frostline_view.c 等）与音效规则
 * （frostline_audio.c），只把 app_game_runtime.c 换成这里：模块直接持有 fl_state_t，
 * 并额外导出一组 promo_* 函数，让 capture.py 像导演一样摆好局面（跳关、给金币、
 * 建塔、升级、放英雄、放冰霜新星、拖动战役长卷），再逐帧录下真实的游戏画面。
 *
 * 只在 Host 录制时编译，不进固件，也不改游戏本身的任何代码。 */
#include <stdio.h>
#include <string.h>

#include "../../components/app_game/include/frostline_model.h"
#include "../../components/app_game/data/frostline_level.h"
#include "../../components/app_game_runtime/include/frostline_audio.h"
#include "../../components/app_game_runtime/include/frostline_view.h"
#include "host_asset_runtime.h"
#include "raylib_lite_game_module.h"
#include "raylib_lite_raylib.h"

#define PROMO_SIZE 480
#define PROMO_TICK_HZ 30
#define PROMO_ALL_ENEMIES ((1u << FL_ENEMY_KIND_COUNT) - 1u)

typedef struct {
    fl_state_t model;
    frostline_audio_t audio;
    bool pointer_held;
    int32_t pointer_track;
    char sound[24];       /* 本帧播放的音效名（去掉 Host 的 _b 变体），没有则为空 */
} promo_state_t;

/* 录制工具单线程使用：Python 侧的 promo_* 调用通过它找到唯一的模块实例 */
static promo_state_t *s_promo;

/* 镜头调度：新出生的敌人血量按百分比缩放（0 表示不缩放），英雄是否不倒 */
static int s_hp_percent;
static int s_armor_cap = -1;     /* 新出生敌人的护甲上限，-1 表示不限 */
static int s_spawn_cap;          /* 同一批敌人的出生间隔上限（帧），0 表示照关卡数据 */
static bool s_hero_guard;
static bool s_scaled[FL_MAX_ENEMIES];

static void apply_direction(fl_state_t *m)
{
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        fl_enemy_t *e = &m->enemies[i];
        if (!e->alive) {
            s_scaled[i] = false;
            continue;
        }
        if (!s_scaled[i]) {
            /* 首领不缩放：它得撑满整个镜头 */
            bool boss = e->behaviour == FL_BEHAVIOUR_WYRM || e->behaviour == FL_BEHAVIOUR_KING;
            if (s_hp_percent > 0 && !boss) {
                e->max_hp = e->max_hp * (float)s_hp_percent / 100.0f;
                e->hp = e->max_hp;
            }
            if (s_armor_cap >= 0 && e->armor > s_armor_cap && !boss) {
                e->armor = (int16_t)s_armor_cap;
            }
        }
        s_scaled[i] = true;
    }
    if (s_spawn_cap > 0 && m->spawn_timer > s_spawn_cap) {
        m->spawn_timer = (uint8_t)s_spawn_cap;
    }
    if (s_hero_guard) {
        for (int i = 0; i < FL_MAX_HEROES; ++i) {
            if (m->heroes[i].active) {
                m->heroes[i].hp = m->heroes[i].max_hp;
            }
        }
    }
}

/* ------------------------------------------------------------------ 模块生命周期 */

static int initialize(void *value, const char *asset_root)
{
    promo_state_t *state = value;
    s_promo = state;
    raylib_lite_host_assets_set_root(asset_root);
    fl_state_reset(&state->model, 0x464C5354u);
    fl_state_open_title(&state->model);
    frostline_view_init();
    if (!frostline_view_ready()) {
        return -1;
    }
    /* 默认所有敌人都已“见过”，避免新敌人介绍卡挡住镜头；需要时由 promo_seen 改回 */
    frostline_view_set_seen_enemies(PROMO_ALL_ENEMIES);
    frostline_audio_init(&state->audio);
    InitWindow(PROMO_SIZE, PROMO_SIZE, "frostline_promo");
    SetTargetFPS(PROMO_TICK_HZ);
    return 0;
}

static void shutdown(void *value)
{
    promo_state_t *state = value;
    frostline_audio_shutdown(&state->audio);
    frostline_view_shutdown();
    s_promo = NULL;
}

static void input(void *value, const raylib_lite_game_input_v1_t *event)
{
    promo_state_t *state = value;
    if (!event || event->type != RAYLIB_LITE_GAME_INPUT_POINTER) {
        return;
    }
    /* 与 app_game_runtime.c 相同：只跟随第一根手指 */
    bool mine = state->pointer_held && state->pointer_track == event->track_id;
    if (event->pressed && (!state->pointer_held || mine)) {
        if (!state->pointer_held) {
            frostline_audio_tap(&state->audio);
        }
        state->pointer_held = true;
        state->pointer_track = event->track_id;
        fl_set_pointer(&state->model, event->x, event->y, true);
        frostline_view_set_pointer(event->x, event->y, true);
    } else if (!event->pressed && (!state->pointer_held || mine)) {
        state->pointer_held = false;
        fl_set_pointer(&state->model, event->x, event->y, false);
        frostline_view_set_pointer(event->x, event->y, false);
    }
}

/* 与 app_game_runtime.c 的 music_for 一致：标题曲、战斗鼓点、首领曲 */
static uint8_t music_for(const fl_state_t *model)
{
    switch (model->phase) {
    case FL_PHASE_TITLE:
    case FL_PHASE_MENU:
    case FL_PHASE_SETTINGS:
    case FL_PHASE_ENDING:
        return FL_MUSIC_TITLE;
    case FL_PHASE_WAVE:
        for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
            const fl_enemy_t *enemy = &model->enemies[i];
            if (enemy->alive && (enemy->champion || enemy->behaviour == FL_BEHAVIOUR_WYRM
                                 || enemy->behaviour == FL_BEHAVIOUR_KING)) {
                return FL_MUSIC_BOSS;
            }
        }
        return FL_MUSIC_BATTLE;
    default:
        return FL_MUSIC_NONE;
    }
}

static void update(void *value)
{
    promo_state_t *state = value;
    fl_state_update(&state->model);
    apply_direction(&state->model);
    fl_audio_frame_t frame = {
        .events = fl_events_take(&state->model),
        .tick = state->model.tick,
        .wave_active = state->model.phase == FL_PHASE_WAVE,
        .music = music_for(&state->model),
        .run_over = state->model.phase == FL_PHASE_WON || state->model.phase == FL_PHASE_LOST,
        .kill_kind = state->model.last_kill_kind,
    };
    /* 清空后再更新：更新完名字非空，就说明这一帧真的响了一个音效 */
    state->audio.name[0] = '\0';
    frostline_audio_update(&state->audio, &frame);
    snprintf(state->sound, sizeof(state->sound), "%s", frostline_audio_name(&state->audio));
    size_t length = strlen(state->sound);
    if (length > 2 && strcmp(state->sound + length - 2, "_b") == 0) {
        state->sound[length - 2] = '\0';
    }
}

static int render(void *value)
{
    promo_state_t *state = value;
    BeginDrawing();
    if (!raylib_lite_raylib_frame_available()) {
        EndDrawing();
        return -1;
    }
    ClearBackground(FL_INK);
    frostline_view_draw(&state->model);
    EndDrawing();
    return 0;
}

static uint32_t state_hash(const void *value)
{
    return ((const promo_state_t *)value)->model.tick;
}

static int state_json(const void *value, char *output, size_t capacity)
{
    const promo_state_t *state = value;
    return fl_state_json_sfx(&state->model, state->sound, output, capacity);
}

static const raylib_lite_game_module_v1_t s_module = {
    .descriptor = {
        RAYLIB_LITE_GAME_MODULE_ABI_V1, "frostline_promo", "frostline_promo",
        PROMO_SIZE, PROMO_SIZE, PROMO_TICK_HZ, 1
    },
    .state_size = sizeof(promo_state_t),
    .initialize = initialize, .shutdown = shutdown, .input = input,
    .update = update, .render = render,
    .state_hash = state_hash, .state_json = state_json,
};

const raylib_lite_game_module_v1_t *raylib_lite_game_module_v1(void)
{
    return &s_module;
}

/* ------------------------------------------------------------------ 导演接口 */

static fl_state_t *model(void)
{
    return &s_promo->model;
}

/* 所有关卡三星通关：战役长卷全部点亮，任何关卡都能直接开打 */
static void unlock_all(void)
{
    for (int i = 0; i < FL_LEVEL_COUNT; ++i) {
        model()->level_stars[i] = 3;
        model()->level_best[i] = 9000u + (uint32_t)i * 377u;
    }
}

void promo_lang(int lang)
{
    model()->settings.lang = lang ? 1 : 0;
}

void promo_seen(uint32_t mask)
{
    frostline_view_set_seen_enemies(mask);
}

void promo_title(void)
{
    fl_state_open_title(model());
    model()->screen_ticks = 0;
}

void promo_ending(void)
{
    model()->phase = FL_PHASE_ENDING;
    model()->screen_ticks = 0;
}

/* 打开战役长卷，选中 level，画面左边缘位于长卷的 scroll 像素处 */
void promo_menu(int level, int scroll)
{
    unlock_all();
    model()->level = (uint8_t)level;
    fl_state_open_menu(model());
    model()->menu_level = (uint8_t)level;
    model()->menu_scroll = (int16_t)scroll;
    model()->screen_ticks = 0;
}

void promo_menu_scroll(int scroll)
{
    model()->menu_scroll = (int16_t)scroll;
}

void promo_menu_pick(int level)
{
    model()->menu_level = (uint8_t)level;
}

/* 开一关，直接跳到第 wave 波的备战阶段，给足金币 */
void promo_level(int level, int wave, int gold)
{
    unlock_all();
    fl_state_start_level(model(), (uint8_t)level);
    model()->wave = (uint8_t)wave;
    model()->gold = (uint16_t)gold;
    model()->prep_ticks = FL_FIRST_PREP_TICKS;
    s_promo->audio.name[0] = '\0';
}

/* 备战倒计时只剩 ticks 帧 */
void promo_prep(int ticks)
{
    model()->prep_ticks = (uint32_t)ticks;
}

void promo_hp_percent(int percent)
{
    s_hp_percent = percent;
}

void promo_armor_cap(int cap)
{
    s_armor_cap = cap;
}

void promo_spawn_cap(int ticks)
{
    s_spawn_cap = ticks;
}

void promo_hero_guard(int on)
{
    s_hero_guard = on != 0;
}

void promo_lives(int lives)
{
    model()->lives = (uint8_t)lives;
    model()->leaks = 0;
}

void promo_gold(int gold)
{
    model()->gold = (uint16_t)gold;
}

void promo_speed(int speed)
{
    model()->speed = (uint8_t)speed;
}

int promo_spot_count(void)
{
    return FL_LEVELS[model()->level].spot_count;
}

int promo_spot_x(int spot) { return FL_LEVELS[model()->level].spots[spot].x; }
int promo_spot_y(int spot) { return FL_LEVELS[model()->level].spots[spot].y; }
int promo_post_x(int post) { return FL_LEVELS[model()->level].posts[post].x; }
int promo_post_y(int post) { return FL_LEVELS[model()->level].posts[post].y; }

/* 在第 spot 个塔位建一座 kind 塔，返回塔的下标，失败返回 -1 */
int promo_build(int spot, int kind)
{
    const fl_point_t *p = &FL_LEVELS[model()->level].spots[spot];
    if (!fl_build_tower(model(), p->x, p->y, (fl_tower_kind_t)kind)) {
        return -1;
    }
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        const fl_tower_t *t = &model()->towers[i];
        if (t->active && t->x == p->x && t->y == p->y) {
            return i;
        }
    }
    return -1;
}

/* 建造环：打开时有几个徽章、第 i 个造什么、在屏幕哪里（供模拟真实点击） */
int promo_ring_count(void) { return fl_ring_count(model()); }
int promo_ring_entry(int i) { return fl_ring_entry(model(), i); }

int promo_ring_x(int i)
{
    int x, y;
    fl_ring_slot(model(), i, &x, &y);
    return x;
}

int promo_ring_y(int i)
{
    int x, y;
    fl_ring_slot(model(), i, &x, &y);
    return y;
}

int promo_upgrade(int tower, int option)
{
    return fl_upgrade_tower(model(), tower, option) ? 1 : 0;
}

int promo_hero(int post)
{
    const fl_point_t *p = &FL_LEVELS[model()->level].posts[post];
    return fl_build_hero(model(), p->x, p->y, FL_HERO_WARDEN) ? 1 : 0;
}

int promo_nova(int hero)
{
    return fl_cast_hero_skill(model(), hero) ? 1 : 0;
}

/* 英雄脚下（冰霜新星半径内）有多少活着的敌人 */
int promo_enemies_near_hero(int hero)
{
    const fl_hero_t *h = &model()->heroes[hero];
    const float r = (float)FL_HERO_STATS[h->kind].skill_radius;
    int count = 0;
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        const fl_enemy_t *e = &model()->enemies[i];
        float dx = e->x - (float)h->x, dy = e->y - (float)h->y;
        if (e->alive && !e->burrowed && dx * dx + dy * dy <= r * r) {
            ++count;
        }
    }
    return count;
}

/* 录制状态：阶段、波次、敌人数、首领位置、本帧音效，写成一行 JSON */
int promo_info(char *output, int capacity)
{
    const fl_state_t *m = model();
    int alive = 0;
    float bx = -1, by = -1;
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        const fl_enemy_t *e = &m->enemies[i];
        if (!e->alive) {
            continue;
        }
        ++alive;
        if (e->behaviour == FL_BEHAVIOUR_WYRM || e->behaviour == FL_BEHAVIOUR_KING || e->champion) {
            bx = e->x;
            by = e->y;
        }
    }
    return snprintf(output, (size_t)capacity,
                    "{\"phase\":%d,\"wave\":%d,\"alive\":%d,\"gold\":%d,\"lives\":%d,"
                    "\"kills\":%lu,\"boss_x\":%.1f,\"boss_y\":%.1f,\"sound\":\"%s\","
                    "\"hero_x\":%d,\"hero_y\":%d}",
                    m->phase, m->wave, alive, m->gold, m->lives, (unsigned long)m->kills,
                    bx, by, s_promo->sound, m->heroes[0].x, m->heroes[0].y);
}

/* 首领下一帧立即出招：冰封之王冰冻最近的塔，矿洞巨蠕钻地或破土 */
void promo_boss_cast(void)
{
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        fl_enemy_t *e = &model()->enemies[i];
        if (e->alive && (e->behaviour == FL_BEHAVIOUR_KING || e->behaviour == FL_BEHAVIOUR_WYRM)) {
            e->phase_ticks = 0;
        }
    }
}

int promo_node_x(int level) { return FL_LEVELS[level].node.x; }

/* 直接判定胜利：满血、零漏怪，结算面板显示三星；kills/score 是面板上的战绩 */
void promo_win(int kills, int score)
{
    fl_state_t *m = model();
    m->kills = (uint32_t)kills;
    m->score = (uint32_t)score;
    m->score_waves = (uint32_t)score / 3u;
    m->wave = (uint8_t)(FL_LEVELS[m->level].wave_count + 1);
    m->lives = m->start_lives;
    m->leaks = 0;
    m->score_bonus = (uint32_t)m->lives * FL_SCORE_PER_LIFE + FL_SCORE_FLAWLESS;
    m->score += m->score_bonus;
    m->phase = FL_PHASE_WON;
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        m->enemies[i].alive = 0;
    }
    m->events |= FL_EVENT_WON;
}
