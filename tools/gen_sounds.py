#!/usr/bin/env python3
"""Synthesise the game's sound effects.

The project had exactly one sound (Sounds/Rifle.wav) and no way to get more: there
is no sample library here and nothing to record with. So these are built from
scratch out of oscillators, noise and filters -- the same approach the meshes and
the HUD art take, and for the same reason.

Written as mono 16-bit 44.1 kHz WAV, which is what Sound/Decoders/WavStreamDecoder
reads (8- and 16-bit PCM) and what Rifle.wav already is. Levels are normalised to a
common peak here and trimmed per sound in the SoundAsset's own `gain` (dB), so
balancing the mix does not mean regenerating anything.

  Groan1..3    a zombie, three utterances for the bank to choose between
  ZombieHit    a round arriving in one
  PlayerHurt   the player being bitten
  Step1..3     a boot on forest floor
  Pickup       something worth having, collected
  Reload       the rifle's bolt
  WardLit      a ward taking light: a low bell and a shimmer
  HiveOpen     a hive: stone grinding aside over a sub-bass swell
  Dawn         the bell that ends the night
  Wind         the bed under all of it, loop-joined so it can repeat forever

Everything is deterministic (fixed seeds) so regenerating gives byte-identical
files rather than a diff every run.

    python3 tools/gen_sounds.py
"""

import math
import pathlib
import struct
import wave

import numpy as np

RATE = 44100
OUT = pathlib.Path("data/Assets/Sounds")

# Loudness every sound is normalised to, as RMS in dB below full scale -- see write().
# Leaves room for the bank's own gain jitter and for several sounds landing at once
# without the mix clipping; per-sound level lives in the SoundAsset's gain.
RMS_DB = -20.0


# ----------------------------------------------------------------- primitives

def t(seconds):
    return np.arange(int(RATE * seconds)) / RATE


def noise(seconds, seed):
    return np.random.default_rng(seed).standard_normal(int(RATE * seconds))


def sine(freq, seconds, phase=0.0):
    """freq may be a scalar or a per-sample array (a glide)."""
    x = t(seconds)
    if np.isscalar(freq):
        return np.sin(2.0 * math.pi * freq * x + phase)
    # Integrate instantaneous frequency so a swept tone stays phase continuous.
    return np.sin(2.0 * math.pi * np.cumsum(freq) / RATE + phase)


def saw(freq, seconds):
    """Band-limited-ish sawtooth: summed harmonics, which is cheap and avoids the
    aliasing buzz a naive ramp gives at these low fundamentals."""
    x = t(seconds)
    out = np.zeros_like(x)
    f = np.asarray(freq, dtype=float)
    h = 1
    while np.max(f) * h < RATE * 0.45 and h <= 40:
        if np.isscalar(freq):
            out += np.sin(2.0 * math.pi * freq * h * x) / h
        else:
            out += np.sin(2.0 * math.pi * np.cumsum(f * h) / RATE) / h
        h += 1
    return out


def biquad(x, b0, b1, b2, a1, a2):
    """Direct form 1. A python loop, but these are all under four seconds."""
    y = np.zeros_like(x)
    x1 = x2 = y1 = y2 = 0.0
    for i, xi in enumerate(x):
        yi = b0 * xi + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        y[i] = yi
        x2, x1 = x1, xi
        y2, y1 = y1, yi
    return y


def bandpass(x, freq, q):
    w = 2.0 * math.pi * freq / RATE
    alpha = math.sin(w) / (2.0 * q)
    b0, b1, b2 = alpha, 0.0, -alpha
    a0, a1, a2 = 1.0 + alpha, -2.0 * math.cos(w), 1.0 - alpha
    return biquad(x, b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0)


def lowpass(x, freq, q=0.707):
    w = 2.0 * math.pi * freq / RATE
    alpha = math.sin(w) / (2.0 * q)
    cw = math.cos(w)
    b0 = (1.0 - cw) / 2.0
    b1 = 1.0 - cw
    b2 = b0
    a0, a1, a2 = 1.0 + alpha, -2.0 * cw, 1.0 - alpha
    return biquad(x, b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0)


def highpass(x, freq, q=0.707):
    w = 2.0 * math.pi * freq / RATE
    alpha = math.sin(w) / (2.0 * q)
    cw = math.cos(w)
    b0 = (1.0 + cw) / 2.0
    b1 = -(1.0 + cw)
    b2 = b0
    a0, a1, a2 = 1.0 + alpha, -2.0 * cw, 1.0 - alpha
    return biquad(x, b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0)


def env(seconds, attack, decay, curve=2.5):
    """Attack then exponential-ish decay over the whole length."""
    n = int(RATE * seconds)
    a = max(1, int(RATE * attack))
    out = np.ones(n)
    out[:a] = np.linspace(0.0, 1.0, a) ** 1.5
    d = np.linspace(0.0, 1.0, max(1, n - a))
    out[a:] = np.exp(-curve * d * (seconds / max(decay, 1e-3)) / 3.0)
    return out


def decay_env(seconds, tau):
    return np.exp(-t(seconds) / tau)


def fade(x, ms=6.0):
    """Kill the click at either end of a one-shot."""
    k = max(2, int(RATE * ms / 1000.0))
    k = min(k, len(x) // 2)
    x[:k] *= np.linspace(0.0, 1.0, k)
    x[-k:] *= np.linspace(1.0, 0.0, k)
    return x


def loop_join(x, seconds=0.6):
    """Make a bed loop seamlessly: crossfade the tail over the head and drop it, so
    the end already *is* the beginning and PlayGrain's repeat has nothing to click on."""
    k = int(RATE * seconds)
    head, body, tail = x[:k].copy(), x[k:-k].copy(), x[-k:].copy()
    ramp = np.linspace(0.0, 1.0, k)
    joined = tail * (1.0 - ramp) + head * ramp
    return np.concatenate([joined, body])


def mix(*layers):
    """Sum layers by *loudness* rather than by amplitude: each is normalised to its own
    RMS first, so the weights mean what they say.

    Worth the helper. Mixing raw layers meant a coefficient depended on how much energy
    that particular noise seed happened to carry, so the three footsteps -- same code,
    different seeds -- came out with half their energy at 77 Hz, 4.4 kHz and 2 kHz
    respectively: three different surfaces instead of three steps on one."""
    out = None
    for weight, sig in layers:
        sig = np.asarray(sig, dtype=float)
        rms = math.sqrt(float(np.mean(sig * sig)))
        scaled = sig * (weight / rms) if rms > 0.0 else sig
        out = scaled if out is None else out + scaled
    return out


def write(name, x, rms_db=RMS_DB):
    """Level by RMS, not by peak.

    Peak normalising these was a mistake worth recording: a filtered voice or a
    struck bell has one transient far above its body, so scaling that transient to
    -3 dBFS left the audible part of a groan sitting near -26 dB while a footstep --
    which is nothing *but* transient -- came out four times louder. Matching RMS
    matches what the sound is heard as. The peak is then brought back under control
    with a soft knee rather than by turning everything down to suit the worst crest
    factor in the set."""
    x = np.asarray(x, dtype=float)
    x = x - np.mean(x)
    rms = math.sqrt(float(np.mean(x * x)))
    if rms > 0.0:
        x = x / rms * (10.0 ** (rms_db / 20.0))
    ceiling = 10.0 ** (-1.0 / 20.0)
    x = ceiling * np.tanh(x / ceiling)
    data = np.clip(x, -1.0, 1.0)
    frames = (data * 32767.0).astype("<i2").tobytes()
    path = OUT / (name + ".wav")
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(frames)
    peak = 20 * math.log10(max(np.max(np.abs(data)), 1e-9))
    rms = 20 * math.log10(max(math.sqrt(float(np.mean(data * data))), 1e-9))
    print(f"  {name + '.wav':16} {len(data) / RATE:5.2f}s  rms {rms:6.1f}  peak {peak:6.1f} dBFS  crest {peak - rms:4.1f}")


# --------------------------------------------------------------------- voices

def groan(seed, length, f0, vowel):
    """A zombie utterance.

    A voice is a buzzing source shaped by resonances, so that is what this is: a
    sawtooth at a throat's fundamental, drifting and wavering, through three
    band-passes at vowel formants. The formants are swept from one vowel toward
    another across the sound, which is what stops it sitting there as a drone -- an
    utterance has to go somewhere. Breath noise rides the same envelope, because a
    moan is mostly air."""
    rng = np.random.default_rng(seed)
    x = t(length)

    # Fundamental: a slow sag over the whole moan, a 4-5 Hz waver, and a little
    # jitter so no two cycles are identical (a perfectly steady pitch reads as a
    # synthesiser, not a throat).
    drift = 1.0 - 0.16 * (x / length) ** 1.5
    waver = 1.0 + 0.03 * np.sin(2.0 * math.pi * (4.0 + rng.random()) * x)
    jitter = 1.0 + 0.008 * lowpass(rng.standard_normal(len(x)), 25.0)
    f = f0 * drift * waver * jitter

    source = saw(f, length) + 0.25 * sine(f * 2.0, length)

    # Formants, swept from the opening vowel to the closing one.
    a, b = vowel
    body = np.zeros_like(x)
    for k, (fa, fb, gain, q) in enumerate(zip(a, b, (1.0, 0.55, 0.22), (9.0, 11.0, 13.0))):
        # A time-varying biquad is not worth the loop; two fixed passes crossfaded
        # give the same impression of a moving mouth.
        first = bandpass(source, fa, q)
        second = bandpass(source, fb, q)
        ramp = np.clip((x / length - 0.15) / 0.7, 0.0, 1.0)
        body += gain * (first * (1.0 - ramp) + second * ramp)

    breath = highpass(rng.standard_normal(len(x)), 900.0) * 0.06
    shaped = (body + breath * (0.4 + 0.6 * np.abs(body) / max(np.max(np.abs(body)), 1e-9)))

    # Long attack, long fall: it starts in the chest.
    e = np.minimum(np.clip(x / (length * 0.28), 0.0, 1.0) ** 1.4,
                   np.clip((length - x) / (length * 0.45), 0.0, 1.0) ** 1.1)
    return fade(shaped * e, 25.0)


def hurt(seed, length, f0):
    """A short voiced grunt -- the same machinery as the groan, without the patience."""
    rng = np.random.default_rng(seed)
    x = t(length)
    f = f0 * (1.0 - 0.35 * (x / length))
    source = saw(f, length) + 0.3 * rng.standard_normal(len(x))
    body = (bandpass(source, 640.0, 8.0)
            + 0.5 * bandpass(source, 1180.0, 10.0)
            + 0.2 * bandpass(source, 2400.0, 12.0))
    return fade(body * decay_env(length, length * 0.28) * np.clip(x / 0.012, 0.0, 1.0), 8.0)


# -------------------------------------------------------------------- sounds

def make_groans():
    # Three vowels' worth of formants: opening -> closing. Roughly "uh" -> "aa",
    # "oh" -> "uh" and "aa" -> "eh", so the bank never says the same thing twice.
    voices = [
        (1.55, 96.0, ((640, 1180, 2400), (760, 1100, 2500))),
        (1.85, 84.0, ((520, 900, 2350), (680, 1150, 2450))),
        (1.30, 108.0, ((720, 1150, 2500), (600, 1650, 2600))),
    ]
    for i, (length, f0, vowel) in enumerate(voices, start=1):
        write(f"Groan{i}", groan(1000 + i, length, f0, vowel))


def make_zombie_hit():
    """Meat and bone: a wet slap, a crack, and half a grunt cut off."""
    length = 0.42
    x = t(length)
    slap = lowpass(noise(length, 21), 1400.0) * decay_env(length, 0.05) * 1.4
    crack = highpass(noise(length, 22), 2200.0) * decay_env(length, 0.012) * 0.5
    thud = sine(np.full(len(x), 70.0) * (1.0 - 0.3 * x / length), length) * decay_env(length, 0.08) * 0.3
    voice = hurt(23, length, 120.0) * 0.5
    write("ZombieHit", fade(mix((1.0, slap), (0.35, crack), (0.5, thud), (0.45, voice))))


def make_player_hurt():
    write("PlayerHurt", hurt(31, 0.38, 132.0))


def make_steps():
    """Boot on needles and loam: a soft thump with a scatter of crunch on top."""
    for i in range(1, 4):
        length = 0.22
        rng = np.random.default_rng(40 + i)
        x = t(length)
        thump = sine(np.full(len(x), 78.0 - 6.0 * i), length) * decay_env(length, 0.045)
        body = lowpass(noise(length, 50 + i), 1100.0 + 200.0 * i) * decay_env(length, 0.035)
        # Crunch: a handful of tiny transients in the first few tens of ms, which is
        # what separates a footstep from a drum hit.
        crunch = np.zeros(len(x))
        for _ in range(7):
            at = int(rng.uniform(0.0, 0.055) * RATE)
            n = int(0.004 * RATE)
            crunch[at:at + n] += rng.standard_normal(n) * rng.uniform(0.2, 0.6)
        crunch = highpass(crunch, 1500.0)
        # Balance measured rather than guessed: at 1.6 thump the whole step sat under
        # 80 Hz and read as a drum, at 0.22 crunch it was a hiss. Half the energy
        # should land in the low hundreds, which is where a boot on loam lives.
        write(f"Step{i}", fade(mix((0.55, thump), (1.0, body), (0.45, crunch)), 4.0))


def make_pickup():
    """Two partials a fifth apart, struck and gliding up a little: the sound of the
    night giving something back."""
    length = 0.7
    x = t(length)
    glide = 1.0 + 0.02 * (x / length)
    tone = (sine(880.0 * glide, length) * decay_env(length, 0.22)
            + 0.5 * sine(1320.0 * glide, length) * decay_env(length, 0.15)
            + 0.25 * sine(1760.0 * glide, length) * decay_env(length, 0.09))
    air = highpass(noise(length, 61), 4000.0) * decay_env(length, 0.03) * 0.15
    write("Pickup", fade(tone + air))


def make_reload():
    """Bolt back, bolt home: two metallic transients with a spring between them."""
    length = 0.55
    out = np.zeros(int(RATE * length))
    for at, freq, level in ((0.0, 2600.0, 1.0), (0.19, 1900.0, 0.9)):
        clip_len = 0.09
        n = int(RATE * clip_len)
        click = bandpass(noise(clip_len, int(freq)), freq, 6.0) * decay_env(clip_len, 0.006) * 3.0
        ring = sine(freq * 0.5, clip_len) * decay_env(clip_len, 0.015) * 0.12
        i = int(RATE * at)
        out[i:i + n] += (click + ring)[:len(out) - i] * level
    spring = bandpass(noise(length, 71), 3400.0, 20.0) * decay_env(length, 0.05) * 0.12
    write("Reload", fade(out + spring))


def bell(length, f0, ratios, taus, seed, strike=0.5):
    """A struck bell: inharmonic partials over a strike transient. Pairs are detuned
    a few cents so the tail beats instead of sitting still."""
    x = t(length)
    out = np.zeros(len(x))
    rng = np.random.default_rng(seed)
    for ratio, tau in zip(ratios, taus):
        for cents in (-3.0, 3.0):
            f = f0 * ratio * (2.0 ** (cents / 1200.0))
            out += sine(f, length, phase=rng.uniform(0, 6.28)) * decay_env(length, tau) / (1.0 + ratio)
    hit = highpass(noise(length, seed + 1), 1500.0) * decay_env(length, 0.01) * strike
    return fade(out + hit, 8.0)


def make_ward_lit():
    """Consecration: a low bell, and a shimmer that arrives after it and outlasts it."""
    length = 2.8
    x = t(length)
    b = bell(length, 220.0, (1.0, 2.02, 2.97, 4.12, 5.43), (1.6, 1.1, 0.8, 0.5, 0.3), 81, strike=0.25)
    shimmer = np.zeros(len(x))
    for f, tau in ((1760.0, 1.2), (2640.0, 0.9), (3520.0, 0.7)):
        shimmer += sine(f, length) * np.exp(-x / tau) * (1.0 - np.exp(-x / 0.35))
    write("WardLit", b + 0.18 * shimmer)


def make_hive_open():
    """A hive: a stone dragged aside, over a swell of something underneath it."""
    length = 3.6
    x = t(length)
    swell = (1.0 - np.exp(-x / 0.5)) * np.exp(-x / 2.4)
    # The sub is felt rather than heard, so it is kept as a bed under the grinding
    # instead of *being* the sound -- at full level it took all the energy below 50 Hz
    # and the stone disappeared entirely.
    sub = sine(np.linspace(58.0, 31.0, len(x)), length) * swell * 0.35
    rumble = lowpass(noise(length, 91), 220.0) * swell * 0.5
    # Grinding: bandpassed noise whose level flutters, which reads as stone on stone
    # rather than as a filtered hiss.
    flutter = 0.55 + 0.45 * np.abs(lowpass(noise(length, 92), 9.0))
    flutter /= np.max(flutter)
    grind = (bandpass(noise(length, 93), 520.0, 1.1)
             + 0.7 * bandpass(noise(length, 95), 1250.0, 1.3)) * flutter * swell * 1.1
    grit = highpass(noise(length, 94), 2500.0) * flutter * swell * 0.2
    write("HiveOpen", fade(mix((0.5, sub), (0.45, rumble), (1.0, grind), (0.2, grit)), 30.0))


def make_dawn():
    """The bell that ends the night. Struck once, left to ring out."""
    write("Dawn", bell(5.0, 165.0, (0.5, 1.0, 1.19, 1.5, 2.0, 2.5, 3.01),
                       (3.4, 3.0, 2.2, 1.7, 1.2, 0.8, 0.55), 101, strike=0.6))


def make_wind():
    """The bed. Brown noise through a slow band, gusting, joined end to start so it
    can repeat forever without a seam."""
    length = 12.0
    x = t(length)
    # Cascaded lowpasses on white noise, not a bandpassed random walk. The walk's
    # energy is all below 50 Hz, so banding it at 420 left so little behind that a
    # -26 dB hiss layer became the entire sound -- measured 50% energy at 12 kHz, i.e.
    # tape noise. Tilting white noise instead puts the weight where wind actually is,
    # a few hundred Hz, and the low shelf underneath gives it size.
    white = noise(length, 111)
    body = (lowpass(lowpass(white, 900.0), 520.0) * 2.2
            + lowpass(white, 160.0) * 1.6
            + bandpass(white, 1700.0, 0.8) * 0.35)
    hiss = highpass(noise(length, 112), 2600.0) * 0.02
    # Gusts: a few incommensurable slow sines, so the pattern never quite repeats
    # inside the loop.
    gust = np.ones(len(x)) * 0.55
    for f, a in ((0.037, 0.22), (0.061, 0.14), (0.113, 0.09)):
        gust += a * np.sin(2.0 * math.pi * f * x + f * 100.0)
    write("Wind", loop_join(mix((1.0, body * gust), (0.06, hiss * gust)), 1.2), rms_db=-26.0)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    print("writing", OUT)
    make_groans()
    make_zombie_hit()
    make_player_hurt()
    make_steps()
    make_pickup()
    make_reload()
    make_ward_lit()
    make_hive_open()
    make_dawn()
    make_wind()


if __name__ == "__main__":
    main()
