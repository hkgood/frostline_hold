#!/usr/bin/env python3
"""宣传片的声音：旁白、配乐、游戏音效，全部锁在 film/src/timeline.json 的帧上。

- 旁白：edge-tts 的「云健」，低沉的史诗男声；每句的起点或终点由时间轴给定，
  实际时长写进 film/src/voice.json，供画面上的字幕使用。
- 配乐：这里用代码合成，D 小调、120 BPM（每拍正好 15 帧），和弦取自游戏标题曲的
  i - VI - III - VII，钟声旋律引用标题曲动机，首领段落引用首领曲的弗里吉亚 riff。
- 音效：录制时每帧响起的游戏音效（manifest.json），按镜头的取帧方式映射到成片的帧。
- 混音：旁白出现时配乐自动让开（侧链压低），最后整体做峰值归一。

用法：~/.venvs/frostline-promo/bin/python tools/promo/audio.py [--lang en]
"""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import wave
from pathlib import Path

import numpy as np
from scipy.signal import butter, fftconvolve, sosfilt

HERE = Path(__file__).resolve().parent
FILM = HERE / "film"
ASSETS = HERE.parents[1] / "assets_src"
TIMELINE = json.loads((FILM / "src/timeline.json").read_text())
MANIFEST = json.loads((FILM / "public/clips/manifest.json").read_text())
VOICE_DIR = FILM / "public/audio/voice"

# 每种语言的旁白：edge-tts 声音、默认语速、时间轴里取哪个字段的文字，以及输出文件
LANGS = {
    "zh": {"voice": "zh-CN-YunjianNeural", "rate": "-8%", "text": "zh",
           "output": FILM / "public/audio/soundtrack.wav", "json": FILM / "src/voice.json"},
    "en": {"voice": "en-US-ChristopherNeural", "rate": "-8%", "text": "en",
           "output": FILM / "public/audio/soundtrack_en.wav", "json": FILM / "src/voice_en.json"},
}

SR = 48000
FPS = TIMELINE["fps"]
DURATION = TIMELINE["durationInFrames"] / FPS
BEAT = 60.0 / TIMELINE["bpm"]
EDGE_TTS = Path(sys.executable).parent / "edge-tts"
RNG = np.random.default_rng(20261031)


def f2t(frame: float) -> float:
    return frame / FPS


def note(name: str) -> float:
    """'D3'、'A#2'、'Eb3' -> 频率。"""
    names = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
    semitone = names[name[0]]
    rest = name[1:]
    if rest[0] in "#b":
        semitone += 1 if rest[0] == "#" else -1
        rest = rest[1:]
    midi = 12 * (int(rest) + 1) + semitone
    return 440.0 * 2 ** ((midi - 69) / 12)


# ---- 基础构件 --------------------------------------------------------------------------

class Track:
    """立体声混音轨：add(信号, 起始秒, 增益, 声像)。"""

    def __init__(self, seconds: float):
        self.data = np.zeros((2, int(seconds * SR) + SR))

    def add(self, signal: np.ndarray, start: float, gain: float = 1.0, pan: float = 0.0) -> None:
        if signal.ndim == 1:
            left, right = np.sqrt(0.5 * (1 - pan)), np.sqrt(0.5 * (1 + pan))
            signal = np.vstack([signal * left, signal * right]) * np.sqrt(2)
        s0 = int(start * SR)
        if s0 < 0:
            signal, s0 = signal[:, -s0:], 0
        end = min(self.data.shape[1], s0 + signal.shape[1])
        if end > s0:
            self.data[:, s0:end] += gain * signal[:, : end - s0]


def t_axis(seconds: float) -> np.ndarray:
    return np.arange(int(seconds * SR)) / SR


def env(seconds: float, attack: float, release: float, curve: float = 1.0) -> np.ndarray:
    t = t_axis(seconds)
    a = np.clip(t / max(attack, 1e-4), 0, 1)
    r = np.clip((seconds - t) / max(release, 1e-4), 0, 1)
    return (a * r) ** curve


def lowpass(x: np.ndarray, cutoff: float, order: int = 4) -> np.ndarray:
    return sosfilt(butter(order, min(cutoff, SR * 0.45), "low", fs=SR, output="sos"), x)


def highpass(x: np.ndarray, cutoff: float, order: int = 2) -> np.ndarray:
    return sosfilt(butter(order, cutoff, "high", fs=SR, output="sos"), x)


def bandpass(x: np.ndarray, lo: float, hi: float, order: int = 2) -> np.ndarray:
    return sosfilt(butter(order, [lo, min(hi, SR * 0.45)], "band", fs=SR, output="sos"), x)


def saw(freq: float | np.ndarray, seconds: float, phase: float = 0.0) -> np.ndarray:
    t = t_axis(seconds)
    ph = np.cumsum(np.broadcast_to(freq, t.shape) / SR) + phase
    return 2.0 * (ph % 1.0) - 1.0


def sine(freq: float | np.ndarray, seconds: float) -> np.ndarray:
    t = t_axis(seconds)
    return np.sin(2 * np.pi * np.cumsum(np.broadcast_to(freq, t.shape) / SR))


def noise(seconds: float) -> np.ndarray:
    return RNG.uniform(-1, 1, int(seconds * SR))


def reverb_ir(seconds: float = 2.8, damp: float = 3.2) -> np.ndarray:
    """合成的大厅混响：左右去相关的指数衰减噪声，越往后越暗。"""
    t = t_axis(seconds)
    ir = np.vstack([noise(seconds), noise(seconds)]) * np.exp(-damp * t)
    ir[0] = lowpass(ir[0], 6000)
    ir[1] = lowpass(ir[1], 6000)
    ir[:, : int(0.012 * SR)] *= np.linspace(0, 1, int(0.012 * SR))
    return ir / np.sqrt(np.sum(ir ** 2) / 2)


# ---- 乐器 -------------------------------------------------------------------------------

def boom(seconds: float = 3.0, amp: float = 1.0) -> np.ndarray:
    """电影预告的低频冲击：下滑的正弦、短促的瞬态。"""
    t = t_axis(seconds)
    freq = 34 + 110 * np.exp(-t * 9)
    body = sine(freq, seconds) * np.exp(-t * 1.6)
    click = lowpass(noise(seconds), 1800) * np.exp(-t * 40) * 0.8
    return amp * np.tanh(1.6 * (body + click))


def taiko(amp: float = 1.0, pitch: float = 1.0) -> np.ndarray:
    seconds = 0.7
    t = t_axis(seconds)
    body = sine((58 + 70 * np.exp(-t * 28)) * pitch, seconds) * np.exp(-t * 7)
    skin = bandpass(noise(seconds), 200, 1800) * np.exp(-t * 30) * 0.6
    return amp * np.tanh(1.4 * (body + skin))


def tick(amp: float = 1.0) -> np.ndarray:
    """切镜头时的金属轻击，像冰面裂开的一声。"""
    seconds = 0.25
    t = t_axis(seconds)
    return amp * highpass(noise(seconds), 3000) * np.exp(-t * 35)


def riser(seconds: float, amp: float = 1.0) -> np.ndarray:
    """上扬的噪声扫频 + 上滑的锯齿音，在冲击点前收住。"""
    t = t_axis(seconds)
    grow = (t / seconds) ** 2.2
    hiss = highpass(noise(seconds), 2500) * grow
    tone = lowpass(saw(180 * 2 ** (2.5 * t / seconds), seconds), 3000) * grow * 0.35
    out = hiss + tone
    out[-int(0.02 * SR):] *= np.linspace(1, 0, int(0.02 * SR))
    return amp * out


def reverse_crash(seconds: float = 1.2, amp: float = 1.0) -> np.ndarray:
    t = t_axis(seconds)
    crash = highpass(noise(seconds), 1500) * np.exp(-t * 3.0)
    return amp * crash[::-1]


def braam(root: str, seconds: float, amp: float = 1.0) -> np.ndarray:
    """低音铜管的“轰”：几支失谐锯齿叠成五度，滤波器先开后合，再过一点失真。"""
    out = np.zeros(int(seconds * SR))
    for name, gain in ((root, 1.0), (root[:-1] + str(int(root[-1]) + 1), 0.6)):
        f = note(name)
        for detune in (-0.6, 0.0, 0.7):
            out += gain * saw(f * 2 ** (detune / 100 * 8), seconds, RNG.uniform())
        out += 0.5 * gain * saw(f * 1.5, seconds, RNG.uniform())
    t = t_axis(seconds)
    cutoff_env = 180 + 1600 * np.exp(-t * 2.2) * np.minimum(1, t / 0.06)
    # 分段滤波近似随时间变化的低通
    chunk = int(0.05 * SR)
    filtered = np.zeros_like(out)
    for s in range(0, len(out), chunk):
        filtered[s:s + chunk] = lowpass(out[max(0, s - 2048):s + chunk], float(cutoff_env[s]), 2)[-len(out[s:s + chunk]):]
    return amp * np.tanh(0.9 * filtered) * env(seconds, 0.03, seconds * 0.6)


def pad(chord: list[str], seconds: float, amp: float = 1.0, bright: float = 1400) -> np.ndarray:
    """合唱般的垫音：每个音七支失谐锯齿，缓起缓收。"""
    out = np.zeros(int(seconds * SR))
    for name in chord:
        f = note(name)
        for k in range(7):
            out += saw(f * 2 ** ((k - 3) * 0.07 / 12), seconds, RNG.uniform())
    out = lowpass(out / (7 * len(chord)), bright, 2)
    return amp * out * env(seconds, min(0.8, seconds * 0.3), min(1.2, seconds * 0.5))


def bell(freq: float, seconds: float = 2.5, amp: float = 1.0) -> np.ndarray:
    """冰铃：不谐和泛音的加法合成，像冰柱被风敲响。"""
    t = t_axis(seconds)
    out = np.zeros_like(t)
    for ratio, gain, decay in ((1.0, 1.0, 2.2), (2.0, 0.5, 3.0), (2.76, 0.35, 4.0),
                               (5.4, 0.18, 6.0), (8.93, 0.08, 9.0)):
        out += gain * np.sin(2 * np.pi * freq * ratio * t) * np.exp(-decay * t)
    return amp * out * np.minimum(1, t / 0.003) * 0.4


def string_note(freq: float, seconds: float, amp: float = 1.0) -> np.ndarray:
    tone = saw(freq, seconds) + 0.6 * saw(freq * 1.003, seconds, 0.3)
    return amp * lowpass(tone, 2600, 2) * env(seconds, 0.008, seconds * 0.7, 1.4) * 0.5


def sub(freq: float, seconds: float, amp: float = 1.0) -> np.ndarray:
    return amp * sine(freq, seconds) * env(seconds, 0.5, 0.8)


# ---- 配乐 -------------------------------------------------------------------------------

def score(music: Track, wet: Track) -> None:
    """按场景写谱。music 是干声，wet 送进混响。"""
    sc = TIMELINE["scenes"]
    at = lambda scene, beats=0.0: f2t(sc[scene]["from"]) + beats * BEAT  # noqa: E731
    chords = {"Dm": ["D3", "A3", "F4"], "Bb": ["A#2", "F3", "D4"], "F": ["F3", "C4", "A4"],
              "C": ["C3", "G3", "E4"], "Gm": ["G2", "D3", "A#3"], "Eb": ["D#3", "A#3", "G4"]}

    # 开场钩子：冰裂、低频冲击、渐起的低鸣与冰铃
    music.add(boom(3.0, 0.9), f2t(6))
    music.add(sub(note("D1"), 1.9, 0.5), 0.1)
    wet.add(pad(chords["Dm"], 2.0, 0.25, 900), 0.0)
    for i, n in enumerate(("A5", "D6", "F6")):
        wet.add(bell(note(n), 2.0, 0.25), f2t(20) + i * 0.18, pan=(-0.5, 0.2, 0.6)[i])
    music.add(reverse_crash(1.1, 0.5), f2t(60) - 1.1)
    music.add(riser(1.2, 0.35), f2t(60) - 1.2)

    # 标题：重击，D 小调垫音，钟声唱出标题曲的动机（D F E A）
    music.add(boom(3.5, 1.0), at("title"))
    music.add(braam("D1", 2.5, 0.55), at("title"))
    wet.add(pad(chords["Dm"], 3.2, 0.35), at("title"))
    for beat, name in ((0.0, "D5"), (1.0, "F5"), (1.5, "E5"), (2.0, "A4"), (4.0, "D5")):
        wet.add(bell(note(name), 2.6, 0.55), at("title", beat), pan=0.1)
        wet.add(bell(note(name) * 2, 1.4, 0.12), at("title", beat + 0.75), pan=-0.3)
    music.add(riser(1.5, 0.4), at("build") - 1.5)
    music.add(reverse_crash(1.0, 0.5), at("build") - 1.0)

    # 建塔：太鼓与弦乐八分音符进入，每个塔的切镜头落一下鼓
    progression = ["Dm", "Bb", "F", "C", "Dm"]
    ostinato = ["D3", "D3", "F3", "D3", "A3", "D3", "F3", "E3"]
    ost_bb = ["A#2", "A#2", "D3", "A#2", "F3", "A#2", "D3", "C3"]
    for bar in range(5):
        start = at("build", bar * 2)
        wet.add(pad(chords[progression[bar]], 2 * BEAT * 1.05, 0.22), start)
    for step in range(20):
        name = (ostinato if (step // 8) % 2 == 0 else ost_bb)[step % 8]
        music.add(string_note(note(name), BEAT * 0.5, 0.42), at("build", step * 0.5), pan=-0.25)
    for beat in range(10):
        music.add(taiko(0.9 if beat % 2 == 0 else 0.6), at("build", beat))
        if beat >= 3:
            music.add(taiko(0.35, 1.5), at("build", beat + 0.5))
            music.add(tick(0.18), at("build", beat), pan=0.4 * (-1) ** beat)
    music.add(boom(2.0, 0.7), at("build", 3))

    # 新星：鼓点加密、上扬，冲击前四帧全部静音，然后一记重锤
    nova_hit = f2t(TIMELINE["hits"][4])
    for beat in range(0, 10):
        t0 = at("nova", beat * 0.5)
        if t0 >= nova_hit - f2t(4):
            break
        music.add(taiko(0.55 + 0.04 * beat, 1.0 + 0.03 * beat), t0)
    for step in range(0, 22):
        t0 = at("nova", step * 0.25)
        if t0 >= nova_hit - f2t(4):
            break
        music.add(string_note(note(ostinato[step % 8]) * 2, BEAT * 0.25, 0.25 + step * 0.012), t0, pan=0.25)
    wet.add(pad(chords["Gm"], 1.6, 0.25), at("nova"))
    wet.add(pad(chords["Bb"], 1.4, 0.3), at("nova") + 1.6)
    music.add(riser(nova_hit - f2t(4) - at("nova", 1), 0.55), at("nova", 1))
    music.add(reverse_crash(1.4, 0.7), nova_hit - 1.4 - f2t(4))
    music.add(boom(4.0, 1.25), nova_hit)
    music.add(braam("D1", 3.5, 0.9), nova_hit)
    wet.add(pad(chords["Dm"] + ["D5"], 3.5, 0.4, 2200), nova_hit)
    for i, n in enumerate(("D6", "A6", "F6", "D7")):
        wet.add(bell(note(n), 3.0, 0.3), nova_hit + i * 0.06, pan=(-0.6, 0.6, -0.2, 0.3)[i])
    for beat in range(3, 6):
        music.add(taiko(0.7 if beat % 2 else 0.5), nova_hit + beat * BEAT)

    # 敌人图鉴：一路推进的鼓与十六分音符弦乐
    for bar in range(4):
        wet.add(pad(chords[("Dm", "Bb", "Gm", "C")[bar]], 2 * BEAT * 1.05, 0.25), at("roster", bar * 2))
    for step in range(32):
        name = (ostinato if (step // 8) % 2 == 0 else ost_bb)[step % 8]
        music.add(string_note(note(name), BEAT * 0.25, 0.38), at("roster", step * 0.25), pan=-0.2)
    for beat in range(8):
        music.add(taiko(0.85 if beat % 2 == 0 else 0.6), at("roster", beat))
        music.add(taiko(0.35, 1.6), at("roster", beat + 0.5))
        music.add(tick(0.12), at("roster", beat + 0.25), pan=0.5)
    music.add(boom(2.0, 0.6), at("roster"))
    # 泰坦、巨蠕、冰封之王三张大牌落下（Roster.tsx 的 giantAt）
    for i, beat in enumerate((4, 5, 6)):
        music.add(boom(1.4, 0.45 + 0.12 * i), at("roster", beat))
        music.add(braam(("A1", "A#1", "D1")[i], 0.9, 0.3), at("roster", beat))
    music.add(riser(1.5, 0.45), at("boss") - 1.5)
    music.add(reverse_crash(1.0, 0.6), at("boss") - 1.0)

    # 首领：引用首领曲的弗里吉亚 riff（降二级压着主音），战鼓每拍落下
    riff = ["D2", "D2", "D#2", "D2", "D2", "A1", "C2", "D#2"]
    music.add(boom(3.0, 1.1), at("boss"))
    music.add(braam("D1", 2.5, 0.8), at("boss"))
    for step in range(40):
        name = riff[step % 8]
        music.add(lowpass(saw(note(name), BEAT * 0.48) + 0.5 * saw(note(name) * 2, BEAT * 0.48), 900)
                  * env(BEAT * 0.48, 0.005, BEAT * 0.3) * 0.35, at("boss", step * 0.5))
    for beat in range(10):
        music.add(taiko(1.0 if beat % 2 == 0 else 0.7, 0.85), at("boss", beat))
        if beat % 2 == 1:
            music.add(taiko(0.5, 0.85), at("boss", beat + 0.5))
    wet.add(pad(chords["Dm"], 2.0, 0.25, 900), at("boss"))
    wet.add(pad(chords["Eb"], 2.0, 0.28, 900), at("boss", 4))
    music.add(boom(2.5, 0.9), f2t(TIMELINE["hits"][7]))
    music.add(braam("D#1", 1.6, 0.5), f2t(TIMELINE["hits"][7]))

    # 战役长卷：抬升到 VI-VII-i，钟声更亮，鼓变成半速
    music.add(boom(2.5, 0.8), at("campaign"))
    for i, (name, beats) in enumerate((("Bb", 2), ("C", 2), ("Dm", 2))):
        wet.add(pad(chords[name] + [chords[name][0][:-1] + "5"], beats * BEAT * 1.1, 0.35, 2400),
                at("campaign", i * 2))
    for beat, name in ((0, "D5"), (1, "F5"), (1.5, "G5"), (2, "A5"), (3, "C6"), (4, "A5"), (5, "D6")):
        wet.add(bell(note(name), 2.2, 0.5), at("campaign", beat), pan=0.15)
    for beat in (0, 2, 4):
        music.add(taiko(0.8), at("campaign", beat))
    music.add(riser(2.0, 0.5), f2t(TIMELINE["hits"][9]) - 2.0)
    music.add(reverse_crash(1.2, 0.7), f2t(TIMELINE["hits"][9]) - 1.2)

    # 终章：胜利的钟声，最后一记重锤与长长的混响尾巴
    wet.add(pad(chords["F"], 1.0, 0.3, 2000), at("finale"))
    final = f2t(TIMELINE["hits"][9])
    music.add(boom(3.5, 1.2), final)
    music.add(braam("D1", 2.8, 0.85), final)
    wet.add(pad(["D3", "A3", "D4", "F4", "E5"], 2.8, 0.45, 2600), final)
    for i, n in enumerate(("D5", "F5", "E5", "A4", "D5")):
        wet.add(bell(note(n), 2.2, 0.5), final + 0.25 + i * 0.32, pan=(-0.3, 0.3, -0.1, 0.2, 0.0)[i])


# ---- 旁白 -------------------------------------------------------------------------------

def decode(path: Path) -> np.ndarray:
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", str(path), "-f", "f32le", "-ac", "1",
                          "-ar", str(SR), "-"], capture_output=True, check=True).stdout
    return np.frombuffer(raw, dtype=np.float32).astype(np.float64)


def synth_voice(text: str, voice: str, rate: str) -> np.ndarray:
    """edge-tts 合成一句，按声音、语速和文字缓存；去掉首尾静音。"""
    VOICE_DIR.mkdir(parents=True, exist_ok=True)
    key = hashlib.sha1(f"{voice}|{rate}|{text}".encode()).hexdigest()[:12]
    path = VOICE_DIR / f"{key}.mp3"
    if not path.exists():
        subprocess.run([str(EDGE_TTS), "--voice", voice, f"--rate={rate}", "--pitch=-10Hz",
                        "--text", text, "--write-media", str(path)], check=True)
    pcm = decode(path)
    loud = np.flatnonzero(np.abs(pcm) > 0.02)
    pcm = pcm[max(0, loud[0] - int(0.03 * SR)): loud[-1] + int(0.08 * SR)]
    return pcm


def voice_track(track: Track, conf: dict) -> list[dict]:
    """把每句旁白放到时间轴上；返回字幕用的 {id, from, duration}（帧）。
    英文可以单独覆盖：en_vo 是实际念的文字（比如把句号换成逗号，缩短停顿；字幕仍用 en），
    rate_en / at_en / end_en 覆盖语速和起止点。"""
    lines = []
    lang = conf["text"]

    def timing(line: dict, key: str):
        """中文用时间轴原值；其他语言先找 key_<lang>，没有再用原值。"""
        return line.get(key) if lang == "zh" else line.get(f"{key}_{lang}", line.get(key))

    for line in TIMELINE["voice"]:
        text = line.get(f"{lang}_vo", line[lang])
        # 语速：中文是 rate，其他语言只认 rate_<lang>（中文的 rate 不适用于英文）
        rate = line.get("rate" if lang == "zh" else f"rate_{lang}", conf["rate"])
        pcm = synth_voice(text, conf["voice"], rate)
        # 一点低频厚度与轻微混响，让声音更“预告片”
        pcm = pcm + 0.25 * lowpass(pcm, 220)
        seconds = len(pcm) / SR
        at = timing(line, "at")
        start = f2t(at) if at is not None else f2t(timing(line, "end")) - seconds
        track.add(pcm, start)
        lines.append({"id": line["id"], "from": round(start * FPS), "duration": round(seconds * FPS)})
        print(f"{line['id']}: {text} {seconds:.2f}s at {start:.2f}s, ends {start + seconds:.2f}s")
    return lines


# ---- 游戏音效 ---------------------------------------------------------------------------

SFX_GAIN = {"nova": 1.0, "boss": 0.9, "freeze": 0.9, "burrow": 0.5, "win": 0.9, "ui_tap": 0.7,
            "build": 0.8, "upgrade": 0.6, "wave_start": 0.5, "crack": 0.5, "impact": 0.35,
            "kill": 0.28, "shot_storm": 0.35, "shot_rune": 0.3, "shot_siege": 0.3, "shot_ember": 0.3,
            "shot_frost": 0.22, "shot_guard": 0.22, "clash": 0.25, "hero_hit": 0.2}


def src_at(segments: list, local: float) -> float:
    seg = [s for s in segments if s[0] <= local][-1]
    return seg[1] + (local - seg[0]) * seg[2]


def game_sfx(track: Track) -> None:
    """录制时记下的音效帧 -> 成片帧：逐帧扫一遍每个镜头的取帧映射。"""
    cache: dict[str, np.ndarray] = {}
    last: dict[str, float] = {}
    scenes = TIMELINE["scenes"]
    for use in TIMELINE["clips"]:
        sounds = {s["frame"]: s["name"] for s in MANIFEST[use["clip"]]["sounds"]}
        seen = set()
        for local in range(use["duration"]):
            src = int(src_at(use["segments"], local))
            if src in seen or src not in sounds:
                continue
            seen.add(src)
            name = sounds[src]
            if name not in SFX_GAIN:
                continue
            t = f2t(scenes[use["scene"]]["from"] + use["at"] + local)
            if t - last.get(name, -9) < 0.12:
                continue
            last[name] = t
            if name not in cache:
                cache[name] = decode(ASSETS / f"{name}.wav")
            track.add(cache[name], t, SFX_GAIN[name] * 0.55, pan=RNG.uniform(-0.3, 0.3))


def title_theme(track: Track) -> None:
    """游戏自己的标题曲，淡淡地垫在标题与终章下面，让配乐带着游戏的味道。"""
    theme = decode(ASSETS / "music_title.wav")
    sc = TIMELINE["scenes"]
    for scene, offset in (("title", 0.0), ("finale", 6.0)):
        start, seconds = f2t(sc[scene]["from"]), f2t(sc[scene]["duration"])
        part = theme[int(offset * SR): int((offset + seconds) * SR)]
        part = part * env(len(part) / SR, 0.3, 0.6)
        track.add(part, start, 0.35)


# ---- 混音 -------------------------------------------------------------------------------

def follower(x: np.ndarray, attack: float = 0.01, release: float = 0.35) -> np.ndarray:
    """旁白的包络（取块最大值再平滑），用来压低配乐。"""
    block = int(0.01 * SR)
    n = len(x) // block
    peaks = np.abs(x[: n * block]).reshape(n, block).max(axis=1)
    out = np.zeros(n)
    acc = 0.0
    a, r = np.exp(-0.01 / attack), np.exp(-0.01 / release)
    for i, p in enumerate(peaks):
        acc = p + (acc - p) * (a if p > acc else r)
        out[i] = acc
    smooth = np.repeat(out, block)
    return np.pad(smooth, (0, len(x) - len(smooth)), mode="edge")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--lang", choices=sorted(LANGS), default="zh", help="旁白语言")
    conf = LANGS[parser.parse_args().lang]
    music, wet, sfx, voice = (Track(DURATION) for _ in range(4))
    lines = voice_track(voice, conf)
    conf["json"].write_text(json.dumps(lines, indent=1, ensure_ascii=False))
    score(music, wet)
    title_theme(music)
    game_sfx(sfx)

    ir = reverb_ir()
    reverb = np.vstack([fftconvolve(wet.data[0], ir[0])[: wet.data.shape[1]],
                        fftconvolve(wet.data[1], ir[1])[: wet.data.shape[1]]])
    send = np.vstack([fftconvolve(music.data.mean(axis=0), ir[0])[: music.data.shape[1]],
                      fftconvolve(music.data.mean(axis=0), ir[1])[: music.data.shape[1]]])
    bed = music.data + 0.55 * reverb + 0.18 * send + 0.25 * wet.data

    v_mono = voice.data.mean(axis=0)
    duck = 1.0 - 0.5 * np.clip(follower(v_mono) / 0.25, 0, 1)
    v_rev = np.vstack([fftconvolve(v_mono, ir[0][: SR])[: len(v_mono)],
                       fftconvolve(v_mono, ir[1][: SR])[: len(v_mono)]])
    mix = bed * duck * 0.8 + sfx.data * 0.9 + voice.data * 1.35 + v_rev * 0.08

    n = int(DURATION * SR)
    mix = mix[:, :n]
    fade = int(0.35 * SR)
    mix[:, -fade:] *= np.linspace(1, 0, fade)
    mix = np.tanh(mix / np.max(np.abs(mix)) * 1.15) / np.tanh(1.15) * 0.93
    pcm = (mix.T * 32767).astype(np.int16)
    conf["output"].parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(conf["output"]), "wb") as out:
        out.setnchannels(2)
        out.setsampwidth(2)
        out.setframerate(SR)
        out.writeframes(pcm.tobytes())
    print(f"wrote {conf['output']} ({n / SR:.1f}s) and {conf['json']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
