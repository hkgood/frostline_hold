"""A tiny software synthesiser for Frostline Hold.

Everything is plain Python: no numpy, no sample library. The engine plays
24 kHz mono signed 16-bit PCM, so the samples here are written at that rate
and the packer never has to resample them.

The sound language follows the art. The world is a woodcut: dry, carved,
percussive. Every hit has a fast attack and a short decay, nothing rings on
for long, and the whole mix lives between roughly 400 Hz and 6 kHz because
the board's speaker is small and loses both extremes.

The pieces below are deliberately generic. The catalogue in ``sfx.py`` says
which sounds exist and what they are made of; this module only knows how to
make noise, pitch and filters.
"""
import math
import random
import struct
import wave

RATE = 24000
TAU = 2.0 * math.pi


def clamp(value, low, high):
    return low if value < low else (high if value > high else value)


# --------------------------------------------------------------------- #
# Buffers
# --------------------------------------------------------------------- #
class Buffer:
    """A mono float buffer. Voices are rendered into it and mixed."""

    def __init__(self, seconds, rate=RATE):
        self.rate = rate
        self.data = [0.0] * max(1, int(round(seconds * rate)))

    def __len__(self):
        return len(self.data)

    def add(self, samples, at=0.0, gain=1.0):
        offset = int(round(at * self.rate))
        for index, value in enumerate(samples):
            position = offset + index
            if 0 <= position < len(self.data):
                self.data[position] += value * gain
        return self

    def peak(self):
        return max((abs(value) for value in self.data), default=0.0)

    def normalize(self, target=0.9):
        peak = self.peak()
        if peak <= 1e-9:
            return self
        scale = target / peak
        self.data = [value * scale for value in self.data]
        return self

    def dc_block(self):
        """Remove the offset, which would otherwise waste headroom and make
        the small speaker thump instead of speak."""
        if not self.data:
            return self
        mean = sum(self.data) / len(self.data)
        self.data = [value - mean for value in self.data]
        return self

    def fade_out(self, seconds=0.006):
        """Every sound has to reach zero, or the speaker pops."""
        count = min(len(self.data), int(seconds * self.rate))
        if count <= 0:
            return self
        for index in range(count):
            position = len(self.data) - count + index
            self.data[position] *= 1.0 - index / count
        return self

    def write_wav(self, path):
        frames = bytearray()
        for value in self.data:
            sample = int(clamp(value, -1.0, 1.0) * 32767.0)
            frames += struct.pack("<h", sample)
        with wave.open(str(path), "wb") as handle:
            handle.setnchannels(1)
            handle.setsampwidth(2)
            handle.setframerate(self.rate)
            handle.writeframes(bytes(frames))
        return path


# --------------------------------------------------------------------- #
# Oscillators
# --------------------------------------------------------------------- #
def _osc(waveform, phase):
    if waveform == "sine":
        return math.sin(phase)
    if waveform == "square":
        return 1.0 if math.sin(phase) >= 0.0 else -1.0
    if waveform == "saw":
        return 2.0 * ((phase / TAU) % 1.0) - 1.0
    if waveform == "triangle":
        position = (phase / TAU) % 1.0
        return 4.0 * abs(position - 0.5) - 1.0
    if waveform == "pulse":
        return 1.0 if (phase / TAU) % 1.0 < 0.32 else -1.0
    raise ValueError("unknown waveform: " + waveform)


# --------------------------------------------------------------------- #
# Filters
# --------------------------------------------------------------------- #
def one_pole_low(samples, cutoff):
    """A stable one-pole lowpass.

    ``cutoff`` is either a constant frequency or a per-sample list, which
    turns it into a sweep. Unlike the resonant filter this cannot run away,
    so it is the right tool for a falling sweep such as an explosion.
    """
    output = []
    state = 0.0
    for index, sample in enumerate(samples):
        frequency = cutoff[index] if isinstance(cutoff, list) else cutoff
        frequency = clamp(frequency, 20.0, RATE * 0.45)
        coefficient = 1.0 - math.exp(-2.0 * math.pi * frequency / RATE)
        state += coefficient * (sample - state)
        output.append(state)
    return output


def one_pole_high(samples, cutoff):
    """The complement of the one-pole lowpass."""
    low = one_pole_low(samples, cutoff)
    return [sample - state for sample, state in zip(samples, low)]


def svf(samples, cutoff, resonance=1.2, mode="low"):
    """A Chamberlin state-variable filter, used for resonant bands.

    ``cutoff`` is either a constant frequency or a list, one value per
    sample, which turns it into a sweep. ``mode`` picks low, band or high.

    The frequency and the damping are kept inside the range where the
    structure stays stable. A sweep that runs past it makes the state
    diverge, and the whole sound turns into a spike.
    """
    low = band = 0.0
    damping = clamp(1.0 / max(0.5, resonance), 0.3, 1.2)
    output = []
    for index, sample in enumerate(samples):
        frequency = cutoff[index] if isinstance(cutoff, list) else cutoff
        frequency = clamp(frequency, 30.0, RATE / 8.0)
        coefficient = 2.0 * math.sin(math.pi * frequency / RATE)
        low += coefficient * band
        high = sample - low - damping * band
        band += coefficient * high
        value = low if mode == "low" else (band if mode == "band" else high)
        output.append(clamp(value, -8.0, 8.0))
    return output


def sweep(start, end, count, curve="linear"):
    """A per-sample frequency ramp."""
    if curve == "exp":
        start = max(1.0, start)
        end = max(1.0, end)
        return [start * (end / start) ** (index / max(1, count - 1))
                for index in range(count)]
    if curve == "log":
        start = max(1.0, start)
        end = max(1.0, end)
        return [end * (start / end) ** (index / max(1, count - 1))
                for index in range(count)]
    if count <= 1:
        return [float(start)]
    step = (end - start) / (count - 1)
    return [start + step * index for index in range(count)]


# --------------------------------------------------------------------- #
# Voices
# --------------------------------------------------------------------- #
def envelope(count, decay, attack=0.002, curve=2.0):
    """A percussive envelope: fast attack, exponential decay."""
    attack_samples = max(1, int(attack * RATE))
    values = []
    for index in range(count):
        if index < attack_samples:
            values.append(index / attack_samples)
            continue
        position = (index - attack_samples) / max(1.0, decay * RATE)
        values.append(math.exp(-curve * position))
    return values


def tone(freq_start, freq_end=None, duration=0.2, waveform="sine", amp=1.0,
         decay=None, attack=0.002, vibrato=0.0, vibrato_rate=6.0,
         curve="linear"):
    """A pitched voice with an optional pitch sweep and vibrato."""
    count = max(1, int(duration * RATE))
    if freq_end is None:
        freq_end = freq_start
    frequencies = sweep(freq_start, freq_end, count, curve)
    shape = envelope(count, decay if decay is not None else duration * 0.55,
                     attack)
    phase = 0.0
    output = []
    for index in range(count):
        frequency = frequencies[index]
        if vibrato:
            frequency *= 1.0 + vibrato * math.sin(
                TAU * vibrato_rate * index / RATE)
        phase += TAU * frequency / RATE
        output.append(_osc(waveform, phase) * shape[index] * amp)
    return output


def noise(duration=0.2, amp=1.0, decay=None, attack=0.001, seed=1234,
          curve=2.0):
    """White noise with a percussive envelope."""
    count = max(1, int(duration * RATE))
    rng = random.Random(seed)
    shape = envelope(count, decay if decay is not None else duration * 0.5,
                     attack, curve)
    return [rng.uniform(-1.0, 1.0) * shape[index] * amp
            for index in range(count)]


def mixed(duration, parts, amp=1.0):
    """Sum several equal-length parts."""
    count = max(len(part) for part in parts)
    output = [0.0] * count
    for part in parts:
        for index, value in enumerate(part):
            output[index] += value
    return [value * amp for value in output]


# --------------------------------------------------------------------- #
# Notes
# --------------------------------------------------------------------- #
_NOTE_OFFSETS = {"C": -9, "D": -7, "E": -5, "F": -4, "G": -2, "A": 0, "B": 2}


def note_frequency(name):
    """``"D5"`` -> 587.33 Hz. Sharps use a trailing ``#``."""
    letter = name[0].upper()
    sharp = "#" in name
    octave = int(name[-1])
    semitones = _NOTE_OFFSETS[letter] + (octave - 4) * 12 + (1 if sharp else 0)
    return 440.0 * (2.0 ** (semitones / 12.0))
