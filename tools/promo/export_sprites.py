#!/usr/bin/env python3
"""从游戏图集导出宣传片要用的精灵：15 种敌人的行走帧、霜卫、七种塔的徽章头像。

每个精灵写成一张横向帧条 film/public/sprites/<名字>.png，sprites.json 记下格子大小和帧数。
用法：python3 tools/promo/export_sprites.py
"""
from __future__ import annotations

import json
from pathlib import Path

from PIL import Image

HERE = Path(__file__).resolve().parent
ASSETS = HERE.parents[1] / "assets_src"
OUTPUT = HERE / "film/public/sprites"

ENEMIES = ["wolf", "raider", "skitter", "husk", "shaman", "banner", "sap", "titan", "crow",
           "golem", "slime", "slimelet", "burrower", "wyrm", "king"]


def load_atlases() -> dict[str, Image.Image]:
    """名字 -> 单帧图像，覆盖所有图集。"""
    frames: dict[str, Image.Image] = {}
    for path in sorted(ASSETS.glob("atlas_*.json")):
        atlas = json.loads(path.read_text())
        sheet = Image.open(ASSETS / atlas["image"]).convert("RGBA")
        cell_w = sheet.width // atlas["columns"]
        cell_h = sheet.height // atlas["rows"]
        for index, frame in enumerate(atlas["frames"]):
            col, row = index % atlas["columns"], index // atlas["columns"]
            frames[frame["name"]] = sheet.crop((col * cell_w, row * cell_h,
                                                (col + 1) * cell_w, (row + 1) * cell_h))
    return frames


def strip(images: list[Image.Image]) -> Image.Image:
    w, h = images[0].size
    out = Image.new("RGBA", (w * len(images), h), (0, 0, 0, 0))
    for i, image in enumerate(images):
        out.paste(image, (i * w, 0))
    return out


def main() -> int:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    frames = load_atlases()
    info = {}
    groups = {f"enemy_{k}": [f"enemy_{k}_{i}" for i in range(6)] for k in ENEMIES}
    groups["hero_warden"] = [f"hero_warden_{i}" for i in range(4)]
    for name in frames:
        if name.startswith("card_"):
            groups[name] = [name]
    for name, parts in groups.items():
        missing = [p for p in parts if p not in frames]
        if missing:
            raise SystemExit(f"missing frames: {missing}")
        images = [frames[p] for p in parts]
        strip(images).save(OUTPUT / f"{name}.png")
        info[name] = {"w": images[0].width, "h": images[0].height, "frames": len(images)}
    (OUTPUT / "sprites.json").write_text(json.dumps(info, indent=1))
    print(f"{len(info)} sprites in {OUTPUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
