#!/usr/bin/env python3
"""录制宣传片用的游戏素材：真实的 C 游戏代码，逐帧导出无损 480x480 PNG。

promo_module.c 把游戏的模型、渲染、音效规则编进一个 Host 模块，并导出 promo_* 导演接口。
这里按分镜摆好每个局面、逐帧推进，把画面写进 film/public/clips/<镜头>/NNNN.png，
并在 manifest.json 里记下每个镜头的帧数、每帧响起的音效和关键时刻（冰霜新星、首领登场）。

用法（需要 Pillow 和一个 Host C 编译器）：
    python3 tools/promo/capture.py            # 录制全部镜头（中文界面）
    python3 tools/promo/capture.py --lang en  # 英文界面，写进 film/public/clips_en/
    python3 tools/promo/capture.py --probe    # 只导出几张带建塔点编号的地图，挑机位用
"""
from __future__ import annotations

import argparse
import ctypes
import json
import math
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Callable

from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[1]
ENGINE = Path(os.environ.get("RAYLIB_LITE_ENGINE",
                             PROJECT.parent / "ESP-Mosaic/submodule/raylib-lite-engine"))
HOST_CC = Path.home() / "esp/host-cc"
GAME = 480
# 游戏的界面语言（frostline_text.h：FL_LANG_ZH = 0，FL_LANG_EN = 1）与各自的录像目录
LANGS = {"zh": (0, HERE / "film/public/clips"), "en": (1, HERE / "film/public/clips_en")}

# 塔的种类（frostline_model.h 的 fl_tower_kind_t）
GUARD, FROST, SIEGE, STORM, RUNE, BARRACKS, EMBER = range(1, 8)
# 关卡下标（frostline_level.h 的 FL_LEVELS）
LEVEL_PASS, LEVEL_MINE, LEVEL_GLACIER, LEVEL_FALLS, LEVEL_CITADEL = 0, 9, 12, 18, 21
ALL_ENEMIES = (1 << 15) - 1


# ---- 编译与驱动 --------------------------------------------------------------------

def compile_module(workdir: Path) -> Path:
    """把 game.sim.json 的源码（换掉运行时适配层）和 promo_module.c 编成 Host 动态库。"""
    manifest = json.loads((PROJECT / "game.sim.json").read_text())
    replaced = {"main/game_entry.c", "components/app_game_runtime/app_game_runtime.c"}
    sources = [PROJECT / s for s in manifest["sources"] if s not in replaced]
    sources.append(HERE / "promo_module.c")
    sources += [ENGINE / p for p in (
        "host/host_module_bridge.c", "host/host_video_backend.c", "host/host_clock.c",
        "src/runtime/raylib_lite_raylib_port.c", "host/host_asset_runtime.c",
        "src/renderer/raylib_lite_renderer.c", "src/renderer/raylib_lite_renderer_raylib.c",
        "src/renderer/raylib_lite_rgb565.c", "src/renderer/raylib_lite_raylib_impl.c",
        "src/fx/raylib_lite_fx.c", "src/renderer/raylib_lite_tilemap.c")]
    includes = [ENGINE / "host/include", ENGINE / "host",
                ENGINE / "examples/common_components/examples_common/include",
                ENGINE / "include/raylib_lite", ENGINE / "compat/raylib/include",
                PROJECT / "main", PROJECT / "assets/generated",
                PROJECT / "managed_components/georgik__raylib/include",
                PROJECT / "managed_components/georgik__raylib/raylib/src"]
    library = workdir / "frostline_promo.so"
    compiler = shutil.which("cc", path=f"{HOST_CC}:{os.environ.get('PATH', '')}") or "cc"
    command = [compiler, "-shared", "-fPIC", "-O2", "-std=c11", "-w",
               "-DRAYLIB_LITE_HOST_SIMULATION=1", *map(str, sources)]
    for include in includes:
        command += ["-I", str(include)]
    command += ["-lm", "-o", str(library)]
    subprocess.run(command, check=True)
    return library


def prepare_assets() -> None:
    """跑一遍项目的素材生成与打包，保证 assets/generated 与源码一致。"""
    sys.path.insert(0, str(ENGINE / "host"))
    import run_game  # noqa: E402  (引擎自带工具)
    run_game.prepare_project_assets(PROJECT)


class HostInput(ctypes.Structure):
    _fields_ = [("type", ctypes.c_int), ("code", ctypes.c_int), ("x", ctypes.c_int32),
                ("y", ctypes.c_int32), ("track_id", ctypes.c_int32), ("pressed", ctypes.c_bool),
                ("value_x", ctypes.c_float), ("value_y", ctypes.c_float), ("value_z", ctypes.c_float)]


class Game:
    """一份正在运行的游戏：推进、触摸、截图，以及 promo_* 导演接口。"""

    def __init__(self, library: Path):
        self.api = ctypes.CDLL(str(library))
        self.api.raylib_lite_host_game_create_v1.argtypes = [ctypes.c_char_p]
        self.api.raylib_lite_host_game_create_v1.restype = ctypes.c_void_p
        self.api.raylib_lite_host_game_destroy_v1.argtypes = [ctypes.c_void_p]
        self.api.raylib_lite_host_game_input_v1.argtypes = [ctypes.c_void_p, ctypes.POINTER(HostInput)]
        self.api.raylib_lite_host_game_update_v1.argtypes = [ctypes.c_void_p]
        self.api.raylib_lite_host_game_render_rgb565_v1.argtypes = [
            ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint16), ctypes.c_size_t]
        self.api.promo_info.argtypes = [ctypes.c_char_p, ctypes.c_int]
        self.api.promo_seen.argtypes = [ctypes.c_uint32]
        self.ctx = self.api.raylib_lite_host_game_create_v1(str(PROJECT / "assets/generated").encode())
        if not self.ctx:
            raise RuntimeError("promo module failed to start (missing atlas?)")
        self.fb = (ctypes.c_uint16 * (GAME * GAME))()
        self._info = ctypes.create_string_buffer(512)

    def __getattr__(self, name: str):
        """promo_xxx(...) 直接转给 C 导演接口，例如 game.build(3, STORM)。"""
        return getattr(self.api, f"promo_{name}")

    def close(self) -> None:
        self.api.raylib_lite_host_game_destroy_v1(self.ctx)

    def touch(self, x: int, y: int, pressed: bool) -> None:
        value = HostInput(2, 0, x, y, 0, pressed, 0, 0, 0)
        self.api.raylib_lite_host_game_input_v1(self.ctx, ctypes.byref(value))

    def step(self) -> None:
        self.api.raylib_lite_host_game_update_v1(self.ctx)

    def grab(self) -> Image.Image:
        if self.api.raylib_lite_host_game_render_rgb565_v1(self.ctx, self.fb, GAME):
            raise RuntimeError("render failed")
        raw = ctypes.string_at(ctypes.addressof(self.fb), GAME * GAME * 2)
        return Image.frombytes("RGB", (GAME, GAME), raw, "raw", "BGR;16")

    def info(self) -> dict:
        self.api.promo_info(self._info, len(self._info))
        return json.loads(self._info.value)

    def spots(self) -> list[tuple[int, int]]:
        return [(self.spot_x(i), self.spot_y(i)) for i in range(self.spot_count())]


class Recorder:
    """逐帧推进并录制一个镜头；渲染每帧都跑，保持特效与横幅的内部时钟连续。"""

    def __init__(self, game: Game, output: Path):
        self.g = game
        self.output = output
        self.manifest: dict[str, dict] = {}

    def run(self, ticks: int, each: Callable[[int], None] | None = None) -> None:
        """不录制，只推进（摆局面、等敌人走到位）。"""
        for i in range(ticks):
            if each:
                each(i)
            self.g.step()
            self.g.grab()

    def clip(self, name: str, ticks: int, each: Callable[[int, dict], None] | None = None,
             until: Callable[[dict], bool] | None = None) -> dict:
        folder = self.output / name
        shutil.rmtree(folder, ignore_errors=True)
        folder.mkdir(parents=True)
        # track：每帧首领的位置（没有首领时为 null），供合成时镜头跟随
        entry = {"frames": 0, "sounds": [], "marks": {}, "track": []}
        state = self.g.info()
        for i in range(ticks):
            if each:
                each(i, entry)
            self.g.step()
            frame = self.g.grab()
            state = self.g.info()
            frame.save(folder / f"{i:04d}.png", compress_level=1)
            if state["sound"]:
                entry["sounds"].append({"frame": i, "name": state["sound"]})
            boss = state["boss_x"] >= 0
            entry["track"].append([round(state["boss_x"]), round(state["boss_y"])] if boss else None)
            entry["frames"] = i + 1
            if until and until(state):
                break
        self.manifest[name] = entry
        print(f"{name}: {entry['frames']} frames, marks {entry['marks']}, "
              f"{len(entry['sounds'])} sounds", flush=True)
        return entry


# ---- 探查：带编号的地图 -----------------------------------------------------------------

def probe(game: Game, out: Path) -> None:
    out.mkdir(parents=True, exist_ok=True)
    for level in (LEVEL_PASS, LEVEL_MINE, LEVEL_GLACIER, LEVEL_FALLS, LEVEL_CITADEL):
        game.level(level, 1, 9999)
        for _ in range(3):
            game.step()
        img = game.grab().resize((960, 960), Image.NEAREST)
        draw = ImageDraw.Draw(img)
        for i, (x, y) in enumerate(game.spots()):
            draw.ellipse([x * 2 - 14, y * 2 - 14, x * 2 + 14, y * 2 + 14], fill=(200, 40, 40))
            draw.text((x * 2 - 6, y * 2 - 7), str(i), fill=(255, 255, 255))
        for p in range(2):
            x, y = game.post_x(p), game.post_y(p)
            draw.rectangle([x * 2 - 12, y * 2 - 12, x * 2 + 12, y * 2 + 12], fill=(40, 90, 220))
            draw.text((x * 2 - 6, y * 2 - 7), f"H{p}", fill=(255, 255, 255))
        img.save(out / f"level_{level:02d}.png")
    print(f"probe images in {out}")


# ---- 主程序 -------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe", action="store_true")
    parser.add_argument("--skip-assets", action="store_true")
    parser.add_argument("--only", nargs="*", help="只录这些镜头")
    parser.add_argument("--lang", choices=sorted(LANGS), default="zh", help="游戏界面语言")
    args = parser.parse_args()
    lang, output = LANGS[args.lang]
    if not args.skip_assets:
        prepare_assets()
    with tempfile.TemporaryDirectory(prefix="frostline-promo-") as tmp:
        game = Game(compile_module(Path(tmp)))
        try:
            game.lang(lang)
            if args.probe:
                probe(game, Path("/tmp/frostline_probe"))
                return 0
            import shots  # noqa: E402  (分镜脚本)
            rec = Recorder(game, output)
            shots.record_all(rec, args.only)
        finally:
            game.close()
    manifest_path = output / "manifest.json"
    old = json.loads(manifest_path.read_text()) if manifest_path.exists() else {}
    old.update(rec.manifest)
    manifest_path.write_text(json.dumps(old, indent=1, ensure_ascii=False))
    print(f"wrote {manifest_path}")
    return 0


if __name__ == "__main__":
    sys.path.insert(0, str(HERE))
    raise SystemExit(main())
