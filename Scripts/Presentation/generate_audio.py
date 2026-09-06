#!/usr/bin/env python3
"""Original Transmit mechanical/electromagnetic one-shots; Python stdlib only.

Run from any directory. Outputs are source WAVs, not Unreal-managed assets.
Use --verify to inspect existing outputs without regenerating them.
"""

import argparse
import array
import json
import math
from pathlib import Path
import random
import sys
import wave

RATE = 48000
TAU = math.tau
ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "ContentSource/Presentation/Audio"
DURATIONS = {
    "Capture": 0.30, "Transfer": 0.35, "Dock": 0.90,
    "Telegraph": 0.80, "Intercept": 0.45, "RamImpact1": 0.65,
    "RamImpact2": 1.30, "Complete": 1.80,
}


def exp_tail(t, start, decay):
    return math.exp(-(t - start) / decay) if t >= start else 0.0


def modal(t, start, base, decay, strength=1.0):
    """Inharmonic damped steel resonances; no pitched fantasy bell arpeggio."""
    if t < start:
        return 0.0
    age = t - start
    modes = ((1.0, 1.0), (1.431, 0.47), (2.137, 0.28), (3.731, 0.13))
    return strength * sum(
        weight * math.sin(TAU * base * ratio * age)
        * math.exp(-age * (1.0 + mode * 0.45) / decay)
        for mode, (ratio, weight) in enumerate(modes)
    )


def sweep(t, start, length, low, high):
    if not start <= t <= start + length:
        return 0.0
    age = t - start
    return math.sin(TAU * (low * age + (high - low) * age * age / (2 * length)))


def synthesize(name, duration, seed):
    rng = random.Random(seed)
    samples = []
    low_noise = previous_noise = 0.0
    for i in range(round(duration * RATE)):
        t = i / RATE
        n = rng.uniform(-1, 1)
        low_noise += 0.055 * (n - low_noise)
        grit = n - previous_noise
        previous_noise = n
        if name == "Capture":
            # Brief reverse pressure intake, then a dry mechanical latch.
            intake = min(t / 0.19, 1.0) ** 1.8 if t < 0.195 else 0.0
            y = intake * (0.30 * low_noise + 0.09 * sweep(t, 0, 0.195, 175, 760))
            y += modal(t, 0.195, 690, 0.019, 0.34)
            y += 0.38 * exp_tail(t, 0.195, 0.026) * math.sin(TAU * 125 * (t - 0.195))
            y += 0.06 * grit * exp_tail(t, 0.196, 0.007)
        elif name == "Transfer":
            y = 0.32 * sweep(t, 0, 0.19, 840, 135) * math.exp(-t / 0.075)
            y += 0.42 * low_noise * math.exp(-t / 0.092)
            y += modal(t, 0, 1070, 0.035, 0.18)
            y += 0.30 * math.sin(TAU * 72 * t) * math.exp(-t / 0.072)
        elif name == "Dock":
            # Three relays grow heavier and settle into a transformer body.
            y = 0.0
            for start, base, amp in ((0.04, 520, 0.18), (0.23, 650, 0.23), (0.44, 780, 0.30)):
                y += modal(t, start, base, 0.033, amp)
                y += amp * 0.33 * grit * exp_tail(t, start, 0.008)
            power = min(max((t - 0.07) / 0.5, 0.0), 1.0)
            power *= math.exp(-max(t - 0.51, 0.0) / 0.115)
            y += power * (0.15 * math.sin(TAU * 90 * t) + 0.06 * math.sin(TAU * 180 * t))
            y += 0.38 * exp_tail(t, 0.44, 0.065) * math.sin(TAU * 62 * (t - 0.44))
        elif name == "Telegraph":
            # Accelerating coil pulses: a physical charge, not an alarm siren.
            u = t / duration
            pulse = (0.5 + 0.5 * math.sin(TAU * (5 * t + 7 * t * t))) ** 5
            envelope = (0.2 + 0.8 * u) * min((duration - t) / 0.055, 1.0)
            y = envelope * (0.24 * sweep(t, 0, duration, 180, 510) * (0.25 + 0.75 * pulse)
                            + 0.28 * low_noise * pulse + 0.045 * math.sin(TAU * 60 * t))
        elif name == "Intercept":
            y = modal(t, 0, 430, 0.023, 0.34)
            y += 0.16 * grit * math.exp(-t / 0.012)
            y += 0.47 * sweep(t, 0, 0.4, 105, 43) * math.exp(-t / 0.084)
            y += 0.23 * low_noise * math.exp(-t / 0.036)
        elif name == "RamImpact1":
            y = modal(t, 0, 205, 0.075, 0.43) + modal(t, 0.009, 1320, 0.031, 0.14)
            y += 0.50 * sweep(t, 0, 0.27, 92, 41) * math.exp(-t / 0.085)
            y += (0.23 * grit + 0.60 * low_noise) * math.exp(-t / 0.024)
            for start, base in ((0.073, 1720), (0.118, 830), (0.167, 1140)):
                y += modal(t, start, base, 0.023, 0.052)
        elif name == "RamImpact2":
            # Lower body, delayed structural shear and a longer debris cascade.
            y = modal(t, 0, 142, 0.12, 0.43)
            y += 0.55 * sweep(t, 0, 0.65, 78, 31) * math.exp(-t / 0.14)
            y += (0.24 * grit + 0.70 * low_noise) * math.exp(-t / 0.035)
            y += modal(t, 0.055, 370, 0.076, 0.30)
            y += 0.24 * low_noise * exp_tail(t, 0.055, 0.11)
            for k, start in enumerate((0.13, 0.20, 0.29, 0.39, 0.51, 0.67, 0.84)):
                y += modal(t, start, 620 + (k * 347) % 1000, 0.025 + k * 0.005, 0.083 / (1 + k * 0.3))
        else:  # Complete: restrained open fifth with a late third, quiet contact.
            y = modal(t, 0.015, 580, 0.028, 0.075)
            for start, frequency, strength in ((0.0, 130.813, 0.22), (0.10, 196.0, 0.12), (0.23, 329.628, 0.055)):
                if t >= start:
                    age = t - start
                    env = (1 - math.exp(-age / 0.055)) * math.exp(-age / 0.34)
                    y += strength * env * (math.sin(TAU * frequency * age) + 0.12 * math.sin(TAU * frequency * 2 * age))
            y += 0.07 * low_noise * math.exp(-t / 0.12)
        samples.append(y)

    # Remove DC before the final envelope; zero-valued endpoints prevent seams.
    mean = sum(samples) / len(samples)
    fade_in = round(0.0015 * RATE)
    fade_out = round(min(0.060, duration * 0.15) * RATE)
    for i, value in enumerate(samples):
        attack = math.sin(min(i / fade_in, 1.0) * math.pi / 2) ** 2
        release = math.sin(min((len(samples) - 1 - i) / fade_out, 1.0) * math.pi / 2) ** 2
        samples[i] = (value - mean) * attack * release
    peak = max(map(abs, samples))
    # Consistent source headroom; runtime event mixing should set relative volume.
    gain = 10 ** (-3.0 / 20.0) / peak
    pcm = array.array("h", (round(value * gain * 32767) for value in samples))
    if sys.byteorder != "little":
        pcm.byteswap()
    return pcm.tobytes()


def verify(path, expected_duration):
    with wave.open(str(path), "rb") as stream:
        assert (stream.getnchannels(), stream.getsampwidth(), stream.getframerate()) == (1, 2, RATE)
        assert stream.getnframes() == round(expected_duration * RATE)
        pcm = array.array("h", stream.readframes(stream.getnframes()))
    if sys.byteorder != "little":
        pcm.byteswap()
    values = [value / 32768 for value in pcm]
    peak = max(map(abs, values))
    dc = sum(values) / len(values)
    rms = math.sqrt(sum(value * value for value in values) / len(values))
    edge = max(abs(values[1] - values[0]), abs(values[-1] - values[-2]))
    assert peak < 0.71 and abs(dc) < 0.001, (path, peak, dc)
    assert pcm[0] == pcm[-1] == 0 and edge < 0.001, (path, edge)
    return {"file": path.name, "seconds": expected_duration,
            "peak_dbfs": round(20 * math.log10(peak), 3),
            "rms_dbfs": round(20 * math.log10(rms), 3),
            "dc": round(dc, 8), "endpoint_step": round(edge, 8),
            "clipped_samples": sum(abs(value) >= 32767 for value in pcm)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    results = []
    for seed, (name, duration) in enumerate(DURATIONS.items(), 21001):
        path = OUTPUT / ("S_Transmit_" + name + ".wav")
        if not args.verify:
            with wave.open(str(path), "wb") as stream:
                stream.setnchannels(1)
                stream.setsampwidth(2)
                stream.setframerate(RATE)
                stream.writeframes(synthesize(name, duration, seed))
        results.append(verify(path, duration))
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
