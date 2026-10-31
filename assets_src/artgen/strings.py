"""Every word the game prints, in both of its languages.

This table is the one source of the text: generate_text.py turns it into a
C header with the strings and the pixel glyphs of every Chinese character
used, so a character appears in the firmware only if some string needs it.

English is drawn with the engine's 5x7 capitals, so the English column is
upper case and keeps to plain ASCII. Format codes (%u, %d, %lu, %s) must
match between the two columns: the view passes the same arguments to
either.
"""

# Index 0 is Chinese (the default), 1 is English.
LANGUAGES = ("zh", "en")

# --- Interface ------------------------------------------------------------
UI = [
    # key, English, Chinese
    ("game_title", "FROSTLINE HOLD", "霜线要塞"),
    ("game_subtitle", "A WINTER TOWER DEFENSE", "凛冬塔防"),
    ("title_start", "START", "开始征程"),
    ("title_settings", "SETTINGS", "设置"),
    ("title_hint", "TAP TO BEGIN", "点击屏幕开始"),
    ("tap_again", "TAP AGAIN", "再点确认"),
    ("hud_wave", "WAVE", "波次"),
    ("won_title", "HOLD SECURED", "守住了！"),
    ("lost_title", "THE GATE FALLS", "城门失守"),
    ("btn_map", "MAP", "地图"),
    ("btn_retry", "RETRY", "重试"),
    ("btn_next", "NEXT", "下一关"),
    ("btn_resume", "RESUME", "继续"),
    ("btn_ending", "ENDING", "结局"),
    ("btn_fight", "FIGHT", "出征"),
    ("btn_menu", "MENU", "菜单"),
    ("btn_back", "BACK", "返回"),
    ("paused", "PAUSED", "暂停"),
    ("end_kills", "KILLS %u      WAVE BONUS %lu", "击杀 %u    波次奖励 %lu"),
    ("end_lives", "LIVES %u      BONUS %lu", "生命 %u    额外奖励 %lu"),
    ("end_wave", "WAVE %02u/%02u      BEST %lu", "波次 %02u/%02u    最佳 %lu"),
    ("end_unlock", "NEXT STAGE UNLOCKS  %s", "下一关解锁：%s"),
    ("wave_of", "WAVE %02u/%02u", "波次 %02u/%02u"),
    ("stage_n", "STAGE %u", "第 %u 关"),
    ("waves_n", "%u WAVES", "共 %u 波"),
    ("best_n", "BEST %lu", "最佳 %lu"),
    ("new_tower", "NEW %s", "新塔：%s"),
    ("sell_n", "SELL %u", "出售 %u"),
    ("stat_storm", "DMG %d  RNG %d  JUMPS %d", "伤害%d 射程%d 弹跳%d"),
    ("stat_barracks", "MEN %d  HP %d  DMG %d", "士兵%d 生命%d 伤害%d"),
    ("stat_ember", "DMG %d  RNG %d  BURN %ds", "伤害%d 射程%d 燃烧%d秒"),
    ("stat_default", "DMG %d  RNG %d  CD %d", "伤害%d 射程%d 冷却%d"),
    ("hero_name", "WARDEN", "霜卫"),
    ("new_enemy", "NEW ENEMY", "新的敌人"),
    ("boss_comes", "A BOSS APPROACHES", "首领来袭"),
    ("settings", "SETTINGS", "设置"),
    ("set_music", "MUSIC", "音乐"),
    ("set_sound", "SOUND", "音效"),
    ("set_haptics", "HAPTICS", "震动"),
    ("set_language", "LANGUAGE", "语言"),
    ("set_theme", "DISPLAY", "显示模式"),
    ("theme_day", "DAY", "日间"),
    ("theme_night", "NIGHT", "夜间"),
    ("set_reset", "RESET PROGRESS", "重置进度"),
    ("set_confirm", "TAP AGAIN TO ERASE", "再点一次清除"),
    ("set_cleared", "PROGRESS ERASED", "进度已清除"),
    ("on", "ON", "开"),
    ("off", "OFF", "关"),
    ("ending_title", "THE WINTER BREAKS", "寒冬终结"),
    ("ending_thanks", "THANK YOU FOR PLAYING", "感谢游玩"),
    ("ending_hint", "TAP TO RETURN", "点击返回"),
]

# Each language names itself, so the option can always be read.
LANGUAGE_NAMES = ("中文", "ENGLISH")

# --- Towers, in TOWER_ORDER, with their upgrade options ---------------------
TOWERS = {
    "guard": ("GUARD", "箭塔"),
    "frost": ("FROST", "冰霜塔"),
    "siege": ("SIEGE", "投石塔"),
    "storm": ("STORM", "雷暴塔"),
    "rune": ("RUNE", "符文塔"),
    "barracks": ("BARRACKS", "盾卫营"),
    "ember": ("EMBER", "余烬塔"),
}

# [stage][option] per tower: the level 2 pick, then the level 3 pick.
UPGRADES = {
    "guard": ((("BALLISTA", "巨弩"), ("REPEATER", "连弩")),
              (("PIERCE", "穿甲"), ("MARKSMAN", "神射"))),
    "frost": ((("DEEPFREEZE", "深冻"), ("GLACIER", "冰川")),
              (("NOVA", "霜爆"), ("RAY", "冰光"))),
    "siege": ((("HAIL", "碎石雨"), ("FIREPOT", "火罐")),
              (("CONCUSSION", "震荡"), ("SHATTER", "粉碎"))),
    "storm": ((("OVERLOAD", "过载"), ("FORK", "分叉")),
              (("STATIC", "静电"), ("TEMPEST", "风暴"))),
    "rune": ((("ARCANE", "奥术"), ("HEX", "咒印")),
             (("PRISM", "棱镜"), ("OBELISK", "方尖碑"))),
    "barracks": ((("SHIELDWALL", "盾墙"), ("SPEARS", "长矛")),
                 (("VETERANS", "老兵"), ("BERSERK", "狂战"))),
    "ember": ((("INFERNO", "炼狱"), ("TAR", "焦油")),
              (("WILDFIRE", "野火"), ("MAGMA", "熔岩"))),
}

# --- Enemies, in the order of level_geom.ENEMY_STATS ------------------------
# key, English name, Chinese name, English note, Chinese note
ENEMIES = [
    ("wolf", "GREY WOLF", "雪狼",
     "FAST, AND RUNS IN PACKS.", "成群奔袭，速度很快。"),
    ("raider", "RAIDER", "劫掠者",
     "A SHIELDED NORTHMAN. TOUGH.", "持盾的北地战士，皮糙肉厚。"),
    ("skitter", "ICE BEETLE", "冰甲虫",
     "SHELLED, AND COMES IN SWARMS.", "甲壳坚硬，成群疾行。"),
    ("husk", "HUSK", "铁甲蛮兵",
     "HEAVY PLATE SHRUGS OFF WEAK HITS.", "重甲护身，弱小的攻击收效甚微。"),
    ("shaman", "FROST SHAMAN", "霜萨满",
     "HEALS ITS ALLIES. KILL IT FIRST.", "不断治疗身边的同伴，优先击杀！"),
    ("banner", "STANDARD BEARER", "旗手",
     "ITS BANNER HARDENS THOSE NEARBY.", "战旗让周围敌人的护甲提升。"),
    ("sap", "FROST SLIME", "冰泥怪",
     "COSTS EXTRA LIVES IF IT GETS IN.", "漏过城门会多扣生命。"),
    ("titan", "ICE TITAN", "冰霜泰坦",
     "SHRUGS OFF SLOWS AND STUNS.", "无视减速与眩晕的巨像。"),
    ("crow", "FROST CROW", "冰鸦",
     "FLIES OVER SOLDIERS. SIEGE AND EMBER CANNOT HIT IT.",
     "飞越士兵阻拦，投石塔与余烬塔打不到它。"),
    ("golem", "STONE GOLEM", "石傀儡",
     "AT HALF HEALTH ITS SHELL CRACKS: NO ARMOUR, MORE SPEED.",
     "血量过半时外壳碎裂，失去护甲但速度加快。"),
    ("slime", "GREAT SLIME", "冰史莱姆",
     "BURSTS INTO THREE SLIMELETS WHEN IT DIES.", "死后分裂成三只小史莱姆。"),
    ("slimelet", "SLIMELET", "小史莱姆",
     "SMALL, BUT MANY.", "弱小，但数量很多。"),
    ("burrower", "BURROWER", "掘地虫",
     "DIVES UNDER THE ROAD, OUT OF REACH.", "时常钻入地下，此时无法被攻击。"),
    ("wyrm", "MINE WYRM", "矿洞巨蠕",
     "BOSS. TUNNELS, THEN SURFACES WITH A BROOD.",
     "首领。潜入地下穿行，出土时召唤甲虫。"),
    ("king", "THE FROST KING", "冰封之王",
     "BOSS. FREEZES TOWERS AND WARDS ITSELF AT EACH THIRD.",
     "首领。冰冻附近的塔，每失去三分之一生命便召出护盾。"),
]

# --- Levels, by key ----------------------------------------------------------
LEVELS = {
    "pass": ("FROSTLINE PASS", "霜线山口"),
    "pines": ("PINEWOOD TRAIL", "松林小径"),
    "ford": ("FROZEN FORD", "冰河渡口"),
    "mere": ("GLASSMERE", "镜湖"),
    "reeds": ("REED SHALLOWS", "芦苇冰滩"),
    "hamlet": ("FOOTHILL HAMLET", "山脚村落"),
    "ridge": ("HOLLOW RIDGE", "空谷山脊"),
    "crag": ("EAGLE CRAG", "鹰嘴崖"),
    "haul": ("OLD HAUL ROAD", "旧运矿道"),
    "mine": ("IRONVEIN MINE", "铁脉矿洞"),
    "camp": ("MINERS' CAMP", "矿工营地"),
    "moraine": ("MORAINE SLOPE", "冰碛坡"),
    "glacier": ("WHITEMAW GLACIER", "白喉冰川"),
    "crevasse": ("CREVASSE BRIDGE", "冰隙桥"),
    "henge": ("BURIED HENGE", "雪埋石阵"),
    "ruins": ("SUNKEN RUINS", "沉眠遗迹"),
    "gateroad": ("ANCIENT PASS", "古道关隘"),
    "icicle": ("ICICLE WOOD", "冰柱林"),
    "falls": ("FROZEN FALLS", "冰封瀑布"),
    "vale": ("RIME VALE", "霜雾河谷"),
    "outer": ("OUTER WARD", "王城外郭"),
    "citadel": ("RIME CITADEL", "冰封要塞"),
}

# --- The ending, one line per entry ------------------------------------------
ENDING = [
    ("THE FROST KING HAS FALLEN.", "冰封之王倒下了。"),
    ("THE HUNDRED-YEAR WINTER THAWS.", "笼罩北境百年的寒冬开始消退。"),
    ("CARAVAN BELLS RING IN THE PASS,", "山口重新响起商队的铃声，"),
    ("LAMPS BURN IN THE MINES AGAIN,", "矿洞里又亮起了灯火，"),
    ("AND FROSTLINE HOLD STILL STANDS.", "而霜线要塞，依旧屹立。"),
]
