#!/usr/bin/env python3
#
# make_whistle.py -- synthesize the referee's whistles of rcssmonitor
#
# Usage: python3 tools/make_whistle.py [OUTPUT_DIR]
#
# Creates OUTPUT_DIR/whistle_short.wav and OUTPUT_DIR/whistle_long.wav
# (OUTPUT_DIR: resources/sounds by default): 44.1 kHz, 16 bits, mono.
# rcssmonitor plays them at the changes of the play mode; the two and three
# long whistles (the end of a half and of the game) repeat the long one
# (docs/plan/2026-11-22-00_v20-features.md §C).
#
# The whistle is a pealess one, as referees use today: a clear tone of about
# 3.8 kHz, steady in pitch, from several chambers that sound at once (the
# tones about 12.5 % and 25 % above the base, weaker), with only a slow
# change of the breath in loudness, a weak harmonic and a little breath
# noise. The short whistle is short (0.1 s) with a quick attack and release,
# to sound crisp. The random numbers have a fixed seed, so the same files are
# made every time. Only the standard library of Python 3 is used.

import math
import os
import random
import struct
import sys
import wave

RATE = 44100
SEED = 20261123

BASE_FREQUENCY = 3780.0  # Hz, the pitch of the whistle
CHAMBERS = ((1.0, 1.0), (1.125, 0.5), (1.25, 0.18))  # (frequency / base, amplitude)
HARMONIC = 0.03  # the second harmonic of the base tone
JITTER = 0.001  # the slight unsteadiness of the pitch (of the base frequency)
BREATH_RATE = 7.0  # Hz, how fast the breath changes the loudness
NOISE = 0.02  # the breath noise, low-passed
PEAK = 0.8  # of the full scale


def synthesize(duration, attack, release, breath_depth, rng):
    """the samples (floats) of a whistle of a duration (seconds)"""
    n = int(duration * RATE)
    samples = []
    phases = [0.0] * len(CHAMBERS)
    noise = 0.0
    jitter = 0.0
    breath_phase = rng.uniform(0.0, 2.0 * math.pi)
    for i in range(n):
        t = i / RATE
        # the pitch: steady, but for a slow and slight wandering
        jitter += rng.gauss(0.0, JITTER * 0.02)
        jitter *= 0.9995
        frequency = BASE_FREQUENCY * (1.0 + jitter)
        tone = 0.0
        for k, (ratio, amplitude) in enumerate(CHAMBERS):
            phases[k] += 2.0 * math.pi * frequency * ratio / RATE
            tone += amplitude * math.sin(phases[k])
        tone += HARMONIC * math.sin(2.0 * phases[0])
        # the breath: a slow change of the loudness
        tone *= 1.0 - breath_depth * (0.5 + 0.5 * math.sin(2.0 * math.pi * BREATH_RATE * t + breath_phase))
        noise = 0.85 * noise + 0.15 * rng.uniform(-1.0, 1.0)
        value = tone + NOISE * noise / 0.15
        # the envelope: a quick attack and a quick release
        if t < attack:
            value *= math.sin(0.5 * math.pi * t / attack)
        elif t > duration - release:
            value *= math.sin(0.5 * math.pi * max(0.0, duration - t) / release)
        samples.append(value)
    peak = max(abs(v) for v in samples)
    return [v * PEAK / peak for v in samples]


def write(path, samples):
    """write the samples to a WAV file (16 bits, mono)"""
    with wave.open(path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(b''.join(struct.pack('<h', int(round(v * 32767))) for v in samples))
    print('created %s (%.2f s, %d bytes)' % (path, len(samples) / RATE, os.path.getsize(path)))


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.normpath(os.path.join(
        os.path.dirname(os.path.abspath(__file__)), '..', 'resources', 'sounds'))
    os.makedirs(out, exist_ok=True)
    rng = random.Random(SEED)
    write(os.path.join(out, 'whistle_short.wav'), synthesize(0.10, 0.008, 0.010, 0.05, rng))
    write(os.path.join(out, 'whistle_long.wav'), synthesize(0.95, 0.025, 0.025, 0.10, rng))


if __name__ == '__main__':
    main()
