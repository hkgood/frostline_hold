"""Build hook: render every sound effect and the battle theme.

This runs in both the Host simulator and the ESP-IDF build, so the WAV files
in assets_src are always reproducible from code.

The packer converts each WAV into the engine's own MSN1 container (16-bit
PCM for the short effects, IMA ADPCM for anything a second or longer), and
the Host simulator serves the same files over ``/sfx/<name>.wav``.

The hook also writes a waveform sheet so the sound design can be reviewed
without listening to twenty files one by one.
"""
import json
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

from PIL import Image, ImageDraw, ImageFont  # noqa: E402

from artgen import sfx  # noqa: E402

PREVIEW_WIDTH = 1180
ROW_HEIGHT = 58
LABEL_WIDTH = 210
WAVE_WIDTH = PREVIEW_WIDTH - LABEL_WIDTH - 30
HEADER = 74

FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FONT_BOLD = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"


def _font(path, size):
    try:
        return ImageFont.truetype(path, size)
    except OSError:
        return ImageFont.load_default()


def render_sounds():
    """Render every entry in the catalogue and write the WAV files."""
    written = []
    for name, recipe in sfx.CATALOGUE.items():
        buffer = recipe()
        buffer.dc_block()
        buffer.fade_out(0.02 if name == "music_battle" else 0.006)
        buffer.normalize(0.88)
        path = HERE / (name + ".wav")
        buffer.write_wav(path)
        # The Host browser only plays a sound when the name changes, so every
        # effect gets a second spelling pointing at the same audio. The alias
        # never reaches the device: the manifest only lists the real names.
        shutil.copyfile(path, HERE / (name + "_b.wav"))
        written.append((name, path))
    return written


def sync_manifest(names):
    """Keep the manifest's sound list in step with the catalogue."""
    path = HERE / "game_assets.json"
    manifest = json.loads(path.read_text(encoding="utf-8"))
    manifest["sounds"] = [{"source": name + ".wav"} for name in names]
    path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def write_preview(written):
    """A waveform sheet: one row per sound, so the whole catalogue reads at
    a glance."""
    title_font = _font(FONT_BOLD, 26)
    label_font = _font(FONT_BOLD, 16)
    small_font = _font(FONT_PATH, 13)

    rows = len(written)
    height = HEADER + rows * ROW_HEIGHT + 24
    image = Image.new("RGB", (PREVIEW_WIDTH, height), (255, 255, 255))
    draw = ImageDraw.Draw(image)

    draw.text((24, 20), "FROSTLINE HOLD  ·  SOUND CATALOGUE", fill=(0, 0, 0),
              font=title_font)
    draw.text((24, 50),
              "24000 Hz mono · 16-bit PCM · wooden, percussive, 400 Hz - 6 kHz "
              "for the small speaker",
              fill=(70, 70, 70), font=small_font)

    for index, (name, path) in enumerate(written):
        top = HEADER + index * ROW_HEIGHT
        middle = top + ROW_HEIGHT // 2
        if index % 2 == 0:
            draw.rectangle([0, top, PREVIEW_WIDTH, top + ROW_HEIGHT],
                           fill=(247, 247, 247))
        draw.text((24, middle - 16), name, fill=(0, 0, 0), font=label_font)

        import wave as wave_module
        with wave_module.open(str(path), "rb") as handle:
            frames = handle.getnframes()
            raw = handle.readframes(frames)
        import struct
        samples = struct.unpack("<%dh" % frames, raw)

        left = LABEL_WIDTH
        right = LABEL_WIDTH + WAVE_WIDTH
        draw.line([left, middle, right, middle], fill=(205, 205, 205))

        peak = max(1, max(abs(value) for value in samples))
        columns = WAVE_WIDTH
        step = max(1, frames // columns)
        points = []
        for column in range(columns):
            start = column * step
            end = min(frames, start + step)
            if start >= end:
                break
            chunk = samples[start:end]
            low = min(chunk)
            high = max(chunk)
            x = left + column
            top_y = middle - (high / peak) * (ROW_HEIGHT * 0.36)
            bottom_y = middle - (low / peak) * (ROW_HEIGHT * 0.36)
            points.append((x, top_y, bottom_y))
        for x, top_y, bottom_y in points:
            draw.line([x, top_y, x, bottom_y], fill=(0, 0, 0))

        seconds = frames / 24000.0
        draw.text((right + 8, middle - 8), "%.2fs" % seconds, fill=(90, 90, 90),
                  font=small_font)

    output = HERE.parent / "assets" / "generated" / "sfx_waveforms.png"
    output.parent.mkdir(parents=True, exist_ok=True)
    image.save(output)
    return output


def main():
    written = render_sounds()
    sync_manifest([name for name, _ in written])
    preview = write_preview(written)
    total = sum(path.stat().st_size for _, path in written)
    print("sfx: %d files, %.1f KB raw" % (len(written), total / 1024.0))
    print("preview: %s" % preview)


if __name__ == "__main__":
    main()
