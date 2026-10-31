"""Generate the C text header from strings.py and the pixel fonts.

The header holds every string in both languages and the glyphs the Chinese
strings need, drawn from two bitmap fonts:

  * Fusion Pixel 12px for body text,
  * GNU Unifont 16px for titles and buttons.

Each glyph row is a uint16_t with bit 15 as the leftmost pixel, cropped to
the rows any kept glyph uses, so a font costs about 26 bytes per character.
Regenerate with: python3 assets_src/artgen/generate_text.py [--check]
"""
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

from artgen import bdf  # noqa: E402
from artgen import level_geom as geom  # noqa: E402
from artgen import levels as campaign  # noqa: E402
from artgen import strings as S  # noqa: E402
from artgen import tower_data as tower  # noqa: E402

FONTS_DIR = HERE.parent / "fonts"
FONTS = (
    # C prefix, file, what it is for
    ("FL_FONT_BODY", "fusion-pixel-12px-zh_hans.bdf", "body text"),
    ("FL_FONT_TITLE", "unifont-16px.bdf", "titles and buttons"),
)

TOWER_KEYS = [kind for kind in tower.TOWER_ORDER]


def _c_string(text):
    """A C literal; non-ASCII stays as UTF-8 escapes so the header is ASCII."""
    out = '"'
    after_escape = False
    for byte in text.encode("utf-8"):
        char = chr(byte)
        if 0x20 <= byte < 0x7F:
            # A hex escape would swallow a following hex digit: close the
            # literal first.
            if after_escape and char in "0123456789abcdefABCDEF":
                out += '" "'
            out += {'"': '\\"', "\\": "\\\\"}.get(char, char)
            after_escape = False
        else:
            out += f"\\x{byte:02x}"
            after_escape = True
    return out + '"'


def entries():
    """(id, english, chinese) in header order."""
    rows = [(f"UI_{key.upper()}", en, zh) for key, en, zh in S.UI]
    rows.append(("LANG_ZH", S.LANGUAGE_NAMES[0], S.LANGUAGE_NAMES[0]))
    rows.append(("LANG_EN", S.LANGUAGE_NAMES[1], S.LANGUAGE_NAMES[1]))
    rows.append(("TOWER_NONE", "", ""))
    for key in TOWER_KEYS:
        en, zh = S.TOWERS[key]
        rows.append((f"TOWER_{key.upper()}", en, zh))
    for key in TOWER_KEYS:
        for stage in range(2):
            for option in range(2):
                en, zh = S.UPGRADES[key][stage][option]
                rows.append((f"UPGRADE_{key.upper()}_{stage}{option}", en, zh))
    enemy_order = [row[1] for row in geom.ENEMY_STATS]
    table = {key: rest for key, *rest in S.ENEMIES}
    for name in enemy_order:
        if name not in table:
            raise SystemExit(f"strings.py has no enemy {name!r}")
        en, zh, _, _ = table[name]
        rows.append((f"ENEMY_{name.upper()}", en, zh))
    for name in enemy_order:
        _, _, en, zh = table[name]
        rows.append((f"ENEMY_NOTE_{name.upper()}", en, zh))
    for level in campaign.LEVELS:
        key = level["key"]
        if key not in S.LEVELS:
            raise SystemExit(f"strings.py has no level {key!r}")
        en, zh = S.LEVELS[key]
        rows.append((f"LEVEL_{key.upper()}", en, zh))
    for index, (en, zh) in enumerate(S.ENDING):
        rows.append((f"ENDING_{index}", en, zh))
    return rows


def _check_formats(rows):
    import re
    spec = re.compile(r"%[0-9]*l?[sdu]")
    for name, en, zh in rows:
        if spec.findall(en) != spec.findall(zh):
            raise SystemExit(f"format codes differ in {name}: {en!r} / {zh!r}")


def _font_block(prefix, path, codepoints):
    font = bdf.load(path, keep=set(codepoints))
    missing = [cp for cp in codepoints if not font.has(cp)]
    if missing:
        text = "".join(chr(cp) for cp in missing)
        raise SystemExit(f"{path.name} lacks glyphs for: {text}")
    top, bottom = font.line_height, 0
    for cp in codepoints:
        for y, row in enumerate(font.glyph(cp)[1]):
            if any(row):
                top = min(top, y)
                bottom = max(bottom, y + 1)
    height = bottom - top
    lines = [f"#define {prefix}_COUNT {len(codepoints)}",
             f"#define {prefix}_HEIGHT {height}",
             f"static const uint16_t {prefix}_CODES[{prefix}_COUNT] = {{"]
    for start in range(0, len(codepoints), 12):
        chunk = codepoints[start:start + 12]
        lines.append("    " + ", ".join(f"0x{cp:04x}" for cp in chunk) + ",")
    lines.append("};")
    lines.append(f"static const uint8_t {prefix}_ADVANCE[{prefix}_COUNT] = {{")
    advances = [font.glyph(cp)[0] for cp in codepoints]
    for start in range(0, len(advances), 24):
        lines.append("    " + ", ".join(str(a) for a in advances[start:start + 24]) + ",")
    lines.append("};")
    lines.append(f"static const uint16_t {prefix}_ROWS[{prefix}_COUNT][{prefix}_HEIGHT] = {{")
    for cp in codepoints:
        rows = font.glyph(cp)[1][top:bottom]
        words = []
        for row in rows:
            value = 0
            for x, bit in enumerate(row[:16]):
                if bit:
                    value |= 1 << (15 - x)
            words.append(f"0x{value:04x}")
        label = chr(cp) if cp < 0x7F else f"U+{cp:04X}"
        label = label.replace("\\", "backslash").replace("*", "star")
        lines.append("    {" + ", ".join(words) + "}, /* " + label + " */")
    lines.append("};")
    return "\n".join(lines)


def render():
    rows = entries()
    _check_formats(rows)
    used = set(range(0x20, 0x7F))
    for _, en, zh in rows:
        used.update(ord(ch) for ch in zh)
        used.update(ord(ch) for ch in en)
    codepoints = sorted(used)
    out = ["/* Generated by assets_src/artgen/generate_text.py -- do not edit by hand.",
           " *",
           " * Source of truth: assets_src/artgen/strings.py and assets_src/fonts/",
           " * Fonts: Fusion Pixel 12px (OFL-1.1), GNU Unifont 16px (OFL-1.1).",
           " */",
           "#ifndef FROSTLINE_TEXT_DATA_H",
           "#define FROSTLINE_TEXT_DATA_H",
           "",
           "#include <stdint.h>",
           "",
           "typedef enum {"]
    for name, _, _ in rows:
        out.append(f"    FL_TXT_{name},")
    out.append("    FL_TXT_COUNT")
    out.append("} fl_text_id_t;")
    out.append("")
    out.append(f"#define FL_TXT_UPGRADE_FIRST FL_TXT_UPGRADE_{TOWER_KEYS[0].upper()}_00")
    first_enemy = geom.ENEMY_STATS[0][1].upper()
    out.append(f"#define FL_TXT_ENEMY_FIRST FL_TXT_ENEMY_{first_enemy}")
    out.append(f"#define FL_TXT_ENEMY_NOTE_FIRST FL_TXT_ENEMY_NOTE_{first_enemy}")
    first_level = campaign.LEVELS[0]["key"].upper()
    out.append(f"#define FL_TXT_LEVEL_FIRST FL_TXT_LEVEL_{first_level}")
    out.append(f"#define FL_TXT_ENDING_COUNT {len(S.ENDING)}")
    out.append("")
    out.append("/* The tables below belong to frostline_text.c alone. */")
    out.append("#ifdef FROSTLINE_TEXT_TABLES")
    out.append("")
    out.append("/* [language][id]: language 0 is Chinese, 1 is English. */")
    out.append("static const char *const FL_TEXT[2][FL_TXT_COUNT] = {")
    for column in (2, 1):
        out.append("    {")
        for row in rows:
            out.append(f"        {_c_string(row[column])},")
        out.append("    },")
    out.append("};")
    out.append("")
    for prefix, filename, purpose in FONTS:
        out.append(f"/* {purpose} */")
        out.append(_font_block(prefix, FONTS_DIR / filename, codepoints))
        out.append("")
    out.append("#endif /* FROSTLINE_TEXT_TABLES */")
    out.append("")
    out.append("#endif")
    return "\n".join(out) + "\n"


def target_path():
    return (HERE.parent.parent / "components" / "app_game_runtime"
            / "include" / "frostline_text_data.h")


def main():
    out = target_path()
    text = render()
    if "--check" in sys.argv:
        if not out.exists() or out.read_text(encoding="utf-8") != text:
            print("frostline_text_data.h is out of date", file=sys.stderr)
            return 1
        print("frostline_text_data.h is up to date")
        return 0
    out.write_text(text, encoding="utf-8")
    print("wrote", out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
