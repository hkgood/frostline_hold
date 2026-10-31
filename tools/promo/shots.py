"""分镜：每个镜头怎么摆局面、录多少帧。由 capture.py 调用。

镜头名对应 film/src/timeline.json 里的 clip；坐标都是游戏的 480x480 像素。
"""
from __future__ import annotations

from capture import (BARRACKS, EMBER, FROST, GUARD, LEVEL_CITADEL, LEVEL_FALLS, LEVEL_MINE,
                     RUNE, SIEGE, STORM, Recorder)

# 霜瀑关（主战场）的七塔布阵：(塔位, 塔种, 二级分支, 三级分支)
FALLS_TOWERS = [(3, STORM, 0, 1), (4, RUNE, 1, 0), (0, SIEGE, 0, 1), (6, FROST, 1, 0),
                (5, EMBER, 0, 0), (1, GUARD, 0, 1), (8, BARRACKS, 0, 0)]


def build(rec: Recorder, layout, levels: int = 2) -> dict[int, list[int]]:
    """按布阵建塔并升级 levels 次；返回 {塔种: [x, y]}，供合成时镜头对准。"""
    g = rec.g
    where = {}
    for spot, kind, a, b in layout:
        tower = g.build(spot, kind)
        if tower < 0:
            raise RuntimeError(f"cannot build {kind} on spot {spot}")
        for option in (a, b)[:levels]:
            g.upgrade(tower, option)
        where[kind] = [g.spot_x(spot), g.spot_y(spot)]
    return where


def reset_direction(rec: Recorder) -> None:
    g = rec.g
    g.hp_percent(0)
    g.armor_cap(-1)
    g.spawn_cap(0)
    g.hero_guard(0)
    g.speed(1)


# ---- 各个镜头 ----------------------------------------------------------------------

def title(rec: Recorder) -> None:
    """标题页：夜幕拉开、极光、飘雪、城堡。"""
    rec.g.title()
    rec.clip("title", 240)


def king(rec: Recorder) -> None:
    """冰封之王（最终关最后一波）：开场钩子与首领段落都用它，冰冻光束打在塔上。"""
    g = rec.g
    reset_direction(rec)
    g.level(LEVEL_CITADEL, 18, 9999)
    g.hp_percent(30)
    # 塔沿着国王下山的路摆，不放英雄：国王一路走下来，沿途把塔冻住
    build(rec, [(2, FROST, 0, 0), (6, GUARD, 0, 1), (3, RUNE, 0, 0), (14, SIEGE, 1, 1),
                (10, STORM, 1, 0), (12, EMBER, 1, 0)], levels=1)
    g.prep(1)
    rec.run(12)
    g.lives(20)

    def act(i: int, entry: dict) -> None:
        if i in (36, 110, 190, 270):
            g.boss_cast()
            entry["marks"].setdefault("freeze", []).append(i + 1)

    rec.clip("king", 330, each=act)


def ring(rec: Recorder) -> None:
    """真实的建造交互：点空塔位，七塔建造环弹开，点两下雷暴塔建成。"""
    g = rec.g
    reset_direction(rec)
    g.level(LEVEL_FALLS, 6, 9999)
    g.hp_percent(40)
    build(rec, [(3, FROST, 0, 0), (4, GUARD, 0, 0), (0, SIEGE, 0, 0), (1, RUNE, 0, 0)], levels=1)
    g.prep(1)
    rec.run(170)
    pad = (g.spot_x(8), g.spot_y(8))
    target = {}

    def act(i: int, entry: dict) -> None:
        if i == 8:
            g.touch(*pad, True)
        elif i == 10:
            g.touch(*pad, False)
            entry["marks"]["ring_open"] = i
        elif i == 28 or i == 46:
            if not target:
                for k in range(g.ring_count()):
                    if g.ring_entry(k) == STORM:
                        target["xy"] = (g.ring_x(k), g.ring_y(k))
                entry["marks"]["storm_medal"] = list(target["xy"])
            g.touch(*target["xy"], True)
            entry["marks"]["pick" if i == 28 else "build"] = i
        elif i == 30 or i == 48:
            g.touch(*target["xy"], False)

    entry = rec.clip("ring", 110, each=act)
    entry["marks"]["pad"] = list(pad)


def towers(rec: Recorder) -> None:
    """七座满级塔齐射：雷暴连锁、符文光束、投石、余烬火罐、冰霜、巨弩、盾卫出营。"""
    g = rec.g
    reset_direction(rec)
    g.level(LEVEL_FALLS, 14, 9999)
    g.hp_percent(35)
    g.spawn_cap(10)
    where = build(rec, FALLS_TOWERS)
    g.hero(1)
    g.prep(1)
    rec.run(230)
    g.lives(20)
    entry = rec.clip("towers", 330)
    entry["marks"]["towers"] = {str(k): v for k, v in where.items()}


def nova(rec: Recorder) -> None:
    """敌潮压线，霜卫放出冰霜新星：一击清掉一片。"""
    g = rec.g
    reset_direction(rec)
    g.level(LEVEL_FALLS, 15, 9999)
    g.hp_percent(10)
    g.armor_cap(0)
    g.spawn_cap(6)
    g.hero_guard(1)
    g.hero(1)
    g.prep(1)
    rec.run(180)
    g.hero_guard(0)
    hero = [g.post_x(1), g.post_y(1)]

    def act(i: int, entry: dict) -> None:
        if "nova" in entry["marks"]:
            return
        if (i >= 50 and g.enemies_near_hero(1) >= 12) or i >= 80:
            entry["marks"]["near"] = g.enemies_near_hero(1)
            if g.nova(1):
                entry["marks"]["nova"] = i

    entry = rec.clip("nova", 180, each=act)
    entry["marks"]["hero"] = hero


def wyrm(rec: Recorder) -> None:
    """矿洞巨蠕：首领来袭横幅，钻入地下，破土而出召唤虫群。"""
    g = rec.g
    reset_direction(rec)
    g.level(LEVEL_MINE, 15, 9999)
    g.hp_percent(40)
    build(rec, [(12, FROST, 0, 0), (0, STORM, 0, 1), (5, GUARD, 1, 0), (3, SIEGE, 0, 0),
                (10, RUNE, 1, 1), (4, BARRACKS, 0, 0), (2, FROST, 1, 1)], levels=1)
    g.hero(0)
    g.prep(20)
    rec.run(10)
    g.lives(20)
    rec.clip("wyrm", 360)


def campaign(rec: Recorder) -> None:
    """战役长卷：从霜线山口一路拖到冰封要塞，镜头经过的关卡依次被选中。"""
    g = rec.g
    reset_direction(rec)
    g.menu(0, 0)
    rec.run(20)
    width, frames = 1440 - 480, 150
    nodes = [g.node_x(i) for i in range(22)]

    def act(i: int, entry: dict) -> None:
        t = min(1.0, max(0.0, (i - 10) / (frames - 30)))
        eased = t * t * (3 - 2 * t)
        scroll = int(round(width * eased))
        g.menu_scroll(scroll)
        center = scroll + 240
        g.menu_pick(min(range(22), key=lambda k: abs(nodes[k] - center)))

    rec.clip("campaign", frames, each=act)


def victory(rec: Recorder) -> None:
    """最终关三星守住：结算面板。"""
    g = rec.g
    reset_direction(rec)
    g.level(LEVEL_CITADEL, 18, 9999)
    build(rec, [(2, FROST, 0, 0), (10, STORM, 1, 0), (6, GUARD, 0, 1), (3, RUNE, 0, 0),
                (14, SIEGE, 1, 1), (12, BARRACKS, 0, 1), (0, EMBER, 1, 0)])
    g.hero(0)
    g.prep(400)
    rec.run(40)

    def act(i: int, entry: dict) -> None:
        if i == 2:
            g.win(1873, 48260)

    rec.clip("victory", 120, each=act)


def ending(rec: Recorder) -> None:
    """通关后的结局动画。"""
    reset_direction(rec)
    rec.g.ending()
    rec.clip("ending", 240)


SHOTS = {f.__name__: f for f in (title, king, ring, towers, nova, wyrm, campaign, victory, ending)}


def record_all(rec: Recorder, only: list[str] | None) -> None:
    for name, shot in SHOTS.items():
        if only and name not in only:
            continue
        shot(rec)
