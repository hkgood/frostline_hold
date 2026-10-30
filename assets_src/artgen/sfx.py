"""The sound catalogue for Frostline Hold.

Each entry is a small recipe built from the generic voices in ``audio.py``.
The names are the file names: the Host simulator serves them over
``/sfx/<name>.wav``, and the packer turns each into a ``<name>.sound`` asset
for the device.

The rules that shaped these recipes:

* the speaker is small, so the useful band is roughly 400 Hz to 6 kHz;
* the art is a woodcut, so nothing rings on for long and every hit has a
  sharp transient;
* the field is busy, so the shot sounds are short and quiet, and the sounds
  that carry meaning (a leak, a wave, the end of a run) are the loud ones.
"""
from . import audio

RATE = audio.RATE


def _bell(freq, duration, amp=1.0, decay=None, partials=((1.0, 1.0),
                                                          (2.01, 0.35),
                                                          (3.04, 0.12))):
    """A struck metal bell: a few inharmonic partials, fast decay."""
    parts = []
    for ratio, weight in partials:
        parts.append(audio.tone(freq * ratio, freq * ratio, duration, "sine",
                                amp * weight, decay=decay or duration * 0.35))
    return audio.mixed(duration, parts)


def _click(freq=2400.0, amp=1.0, decay=0.004):
    """A tiny hard transient. This is what makes a hit feel like a hit."""
    return audio.svf(audio.noise(0.02, 1.0, decay=decay), freq, 1.1, "band")


def _wood(freq_start, freq_end, duration, amp=1.0, decay=None):
    """A hollow wooden knock: a pitch drop plus a body resonance."""
    body = audio.tone(freq_start, freq_end, duration, "triangle", amp,
                      decay=decay or duration * 0.3)
    knock = audio.svf(audio.noise(0.03, 0.5 * amp, decay=0.012), 900.0, 1.4,
                      "band")
    return audio.mixed(duration, [body, knock])


# --------------------------------------------------------------------- #
# Interface
# --------------------------------------------------------------------- #
def ui_tap():
    """A dry wooden click for buttons."""
    buffer = audio.Buffer(0.09)
    buffer.add(_click(2000.0, 0.8, 0.003))
    buffer.add(audio.tone(1400.0, 900.0, 0.06, "sine", 0.5, decay=0.025))
    return buffer


def ui_deny():
    """A dull refusal: two short low knocks that refuse to rise."""
    buffer = audio.Buffer(0.26)
    buffer.add(audio.tone(160.0, 120.0, 0.1, "square", 0.5, decay=0.045), 0.0)
    buffer.add(audio.tone(140.0, 105.0, 0.12, "square", 0.5, decay=0.05), 0.11)
    buffer.add(audio.svf(audio.noise(0.08, 0.3, decay=0.03), 400.0, 1.2, "low"),
               0.0)
    return buffer


def build():
    """A tower goes up: a solid wooden post driven into the ground."""
    buffer = audio.Buffer(0.34)
    buffer.add(_wood(330.0, 140.0, 0.3, 0.9, decay=0.075))
    buffer.add(audio.tone(760.0, 700.0, 0.16, "sine", 0.18, decay=0.09), 0.01)
    return buffer


def upgrade():
    """Two bells a fifth apart, the second answering the first."""
    buffer = audio.Buffer(0.6)
    buffer.add(_bell(audio.note_frequency("D6"), 0.42, 0.55, 0.14))
    buffer.add(_bell(audio.note_frequency("A6"), 0.5, 0.42, 0.17), 0.09)
    buffer.add(_click(3200.0, 0.25, 0.003), 0.09)
    return buffer


def sell():
    """Coins change hands: three quick metal pings falling away."""
    buffer = audio.Buffer(0.42)
    for index, (freq, offset) in enumerate(((1500.0, 0.0), (1150.0, 0.07),
                                            (820.0, 0.14))):
        buffer.add(_bell(freq, 0.2, 0.4 - index * 0.06, 0.07), offset)
        buffer.add(_click(2800.0, 0.18, 0.002), offset)
    return buffer


# --------------------------------------------------------------------- #
# Combat
# --------------------------------------------------------------------- #
def shot_guard():
    """A crossbow: a string snap with a short wooden body."""
    buffer = audio.Buffer(0.14)
    buffer.add(audio.tone(950.0, 430.0, 0.11, "saw", 0.42, decay=0.03))
    buffer.add(_click(3000.0, 0.55, 0.003))
    buffer.add(_wood(420.0, 240.0, 0.08, 0.22, decay=0.02), 0.005)
    return buffer


def shot_frost():
    """Ice: a bright hiss that sweeps upward and dissolves."""
    buffer = audio.Buffer(0.3)
    count = int(0.22 * RATE)
    sweep = audio.sweep(900.0, 5200.0, count, "exp")
    buffer.add(audio.svf(audio.noise(0.22, 1.0, decay=0.07), sweep, 1.6,
                         "band"))
    buffer.add(audio.one_pole_high(audio.noise(0.22, 0.5, decay=0.07), sweep),
               0.0)
    buffer.add(audio.tone(2400.0, 3600.0, 0.16, "sine", 0.16, decay=0.05))
    buffer.add(audio.tone(700.0, 1100.0, 0.14, "sine", 0.14, decay=0.05))
    return buffer


def shot_siege():
    """A stone thrown by a catapult: heavy, dull, with grit on top."""
    buffer = audio.Buffer(0.3)
    buffer.add(_wood(250.0, 130.0, 0.26, 1.0, decay=0.07))
    buffer.add(audio.svf(audio.noise(0.09, 0.5, decay=0.03), 750.0, 1.0, "low"))
    return buffer


def impact():
    """The area blast: a wide burst of noise falling away over a thump."""
    buffer = audio.Buffer(0.55)
    count = int(0.4 * RATE)
    sweep = audio.sweep(5200.0, 260.0, count, "exp")
    buffer.add(audio.one_pole_low(
        audio.noise(0.4, 1.0, decay=0.28, curve=1.3), sweep))
    buffer.add(audio.tone(130.0, 55.0, 0.34, "sine", 0.75, decay=0.2))
    buffer.add(audio.svf(audio.noise(0.12, 0.35, decay=0.05), 1600.0, 0.8,
                         "band"), 0.0)
    return buffer


def kill():
    """Something dies: a short crunch with a falling squeal."""
    buffer = audio.Buffer(0.2)
    buffer.add(audio.svf(audio.noise(0.12, 1.0, decay=0.035), 820.0, 1.1,
                         "band"))
    buffer.add(audio.tone(760.0, 190.0, 0.11, "square", 0.3, decay=0.03))
    return buffer


def leak():
    """One of them got through. A dark bell that will not let go."""
    buffer = audio.Buffer(0.95)
    buffer.add(audio.tone(178.0, 174.0, 0.8, "sine", 0.7, decay=0.32))
    buffer.add(audio.tone(268.0, 262.0, 0.8, "sine", 0.3, decay=0.26))
    buffer.add(audio.tone(452.0, 446.0, 0.7, "sine", 0.18, decay=0.24))
    buffer.add(audio.svf(audio.noise(0.5, 0.5, decay=0.2), 420.0, 0.8, "low"),
               0.0)
    return buffer


# --------------------------------------------------------------------- #
# Flow
# --------------------------------------------------------------------- #
def wave_start():
    """The horn that opens a wave: two notes, a fifth apart."""
    buffer = audio.Buffer(1.15)
    for freq, offset in ((165.0, 0.0), (131.0, 0.42)):
        voice = audio.mixed(0.75, [
            audio.tone(freq, freq, 0.7, "saw", 0.42, attack=0.05, decay=0.3,
                       vibrato=0.008, vibrato_rate=5.0),
            audio.tone(freq * 2.0, freq * 2.0, 0.7, "saw", 0.16, attack=0.06,
                       decay=0.28),
            audio.tone(freq * 3.0, freq * 3.0, 0.7, "sine", 0.08, attack=0.07,
                       decay=0.26),
        ])
        buffer.add(voice, offset)
    return buffer


def wave_clear():
    """The wave is held: two bright bells."""
    buffer = audio.Buffer(0.8)
    buffer.add(_bell(audio.note_frequency("A5"), 0.6, 0.5, 0.2))
    buffer.add(_bell(audio.note_frequency("E6"), 0.65, 0.38, 0.22), 0.1)
    return buffer


def hero_hit():
    """The hero takes a blow: a muffled body thud."""
    buffer = audio.Buffer(0.26)
    buffer.add(audio.svf(audio.noise(0.16, 1.0, decay=0.05), 420.0, 0.9, "low"))
    buffer.add(audio.tone(210.0, 120.0, 0.18, "sine", 0.5, decay=0.06))
    return buffer


def hero_down():
    """The hero falls: a sliding tone and a body on the ground."""
    buffer = audio.Buffer(0.55)
    buffer.add(audio.tone(520.0, 105.0, 0.4, "triangle", 0.55, decay=0.16))
    buffer.add(_wood(220.0, 110.0, 0.2, 0.5, decay=0.05), 0.33)
    return buffer


def nova():
    """The frost nova: a cold wave of ice across the road."""
    buffer = audio.Buffer(1.0)
    count = int(0.55 * RATE)
    sweep = audio.sweep(500.0, 6000.0, count, "exp")
    buffer.add(audio.svf(audio.noise(0.55, 1.0, decay=0.32, curve=1.2), sweep,
                         1.7, "band"))
    for freq, weight in ((audio.note_frequency("C7"), 0.16),
                          (audio.note_frequency("E7"), 0.12),
                          (audio.note_frequency("G7"), 0.09)):
        buffer.add(audio.tone(freq, freq * 1.02, 0.45, "sine", weight,
                              attack=0.01, decay=0.24))
    buffer.add(audio.tone(150.0, 70.0, 0.4, "sine", 0.6, decay=0.14))
    return buffer


def win():
    """The run is won: a rising figure and a held chord."""
    buffer = audio.Buffer(1.8)
    figure = (("A5", 0.0), ("D6", 0.14), ("F6", 0.28), ("A6", 0.42))
    for name, offset in figure:
        buffer.add(_bell(audio.note_frequency(name), 0.5, 0.42, 0.16), offset)
    for name in ("D5", "F5", "A5", "D6"):
        buffer.add(_bell(audio.note_frequency(name), 1.0, 0.22, 0.4), 0.62)
    return buffer


def lose():
    """The run is lost: a slow fall into a low drone."""
    buffer = audio.Buffer(1.7)
    figure = (("D4", 0.0), ("C4", 0.24), ("A3", 0.48), ("F3", 0.72))
    for name, offset in figure:
        freq = audio.note_frequency(name)
        buffer.add(audio.tone(freq, freq * 0.995, 0.42, "triangle", 0.4,
                              attack=0.012, decay=0.16), offset)
    buffer.add(audio.tone(audio.note_frequency("D2"), audio.note_frequency("D2"),
                          1.3, "sine", 0.4, attack=0.05, decay=0.5))
    return buffer


# --------------------------------------------------------------------- #
# The battle theme
# --------------------------------------------------------------------- #
BPM = 84.0
BEAT = 60.0 / BPM
BARS = 8

# (beat, length in beats, note). A rest is written with an empty name.
_MELODY = (
    (0.0, 1.5, "A4"), (1.5, 0.5, "D5"), (2.0, 1.0, "C5"), (3.0, 1.0, "D5"),
    (4.0, 1.5, "F5"), (5.5, 0.5, "E5"), (6.0, 2.0, "D5"),
    (8.0, 1.0, "E5"), (9.0, 0.5, "F5"), (9.5, 0.5, "E5"), (10.0, 2.0, "D5"),
    (12.0, 2.0, "C5"), (14.0, 2.0, "A4"),
    (16.0, 1.5, "D5"), (17.5, 0.5, "E5"), (18.0, 1.5, "F5"), (19.5, 0.5, "E5"),
    (20.0, 2.0, "D5"), (22.0, 1.0, "C5"), (23.0, 1.0, "A4"),
    (24.0, 1.0, "G4"), (25.0, 1.0, "A4"), (26.0, 2.0, "B4"),
    (28.0, 2.0, "A4"),
)


def _hurdy(freq, duration):
    """A bowed folk voice: a reedy tone with a bow noise on top."""
    reed = audio.tone(freq, freq, duration, "pulse", 0.5, attack=0.035,
                      decay=duration * 0.5, vibrato=0.012, vibrato_rate=5.5)
    bow = audio.svf(audio.noise(duration, 0.16, decay=duration * 0.4,
                                seed=int(freq)), freq * 2.0, 1.4, "band")
    return audio.mixed(duration, [reed, bow])


def _frame_drum(amp=1.0):
    """A low frame drum: a thump with a short skin snap."""
    body = audio.tone(120.0, 78.0, 0.18, "sine", 0.9 * amp, decay=0.05)
    skin = audio.svf(audio.noise(0.05, 0.4 * amp, decay=0.016), 2200.0, 1.0,
                     "band")
    return audio.mixed(0.18, [body, skin])


def music_battle():
    """The theme that loops under a wave: a drone, a folk line and a drum.

    It is built to sit behind the action, not in front of it: the melody
    lives in the middle of the range and the drums only mark the first and
    the third beat of each bar.
    """
    total = BARS * 4 * BEAT
    buffer = audio.Buffer(total + 0.4)

    drone_freq = audio.note_frequency("D2")
    drone = audio.mixed(total, [
        audio.tone(drone_freq, drone_freq, total, "sine", 0.42, attack=0.4,
                   decay=0.0),
        audio.tone(drone_freq * 2.0, drone_freq * 2.0, total, "sine", 0.16,
                   attack=0.5, decay=0.0),
    ])
    buffer.add(drone, 0.0)

    for beat, length, name in _MELODY:
        if not name:
            continue
        duration = length * BEAT * 0.98
        buffer.add(_hurdy(audio.note_frequency(name), duration), beat * BEAT)

    for bar in range(BARS):
        base = bar * 4 * BEAT
        buffer.add(_frame_drum(0.55), base)
        buffer.add(_frame_drum(0.36), base + 2 * BEAT)
        # A rim tick at the back of the bar keeps the loop turning.
        buffer.add(_click(2600.0, 0.16, 0.004), base + 3.5 * BEAT)

    return buffer


# --------------------------------------------------------------------- #
# The catalogue
# --------------------------------------------------------------------- #
CATALOGUE = {
    "ui_tap": ui_tap,
    "ui_deny": ui_deny,
    "build": build,
    "upgrade": upgrade,
    "sell": sell,
    "shot_guard": shot_guard,
    "shot_frost": shot_frost,
    "shot_siege": shot_siege,
    "impact": impact,
    "kill": kill,
    "leak": leak,
    "wave_start": wave_start,
    "wave_clear": wave_clear,
    "hero_hit": hero_hit,
    "hero_down": hero_down,
    "nova": nova,
    "win": win,
    "lose": lose,
    "music_battle": music_battle,
}
