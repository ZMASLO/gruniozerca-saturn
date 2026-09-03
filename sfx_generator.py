#!/usr/bin/env python3
"""
SFX Generator for Gruniożerca Saturn
Generates 4 sound effects as raw PCM (32kHz, mono, 16-bit signed)
Compatible with SCSP / Jo Engine on Sega Saturn.

Outputs: CATCH.PCM, MISS.PCM, CHGCOL.PCM, GAMEOVR.PCM
Also writes WAV versions for local testing.
"""

import wave, struct, math, os
import numpy as np

SAMPLE_RATE = 32000
MAX_AMP     = 0.6        # safety headroom (0dBFS = 1.0, leave 40% margin)
OUT_DIR     = "JoEngine-src/Projects/gruniozerca/cd"

def write_raw_pcm(filename: str, samples: np.ndarray):
    """Write 16-bit signed mono PCM raw (no WAV header)."""
    samples = np.clip(samples, -1.0, 1.0) * 32767
    samples = samples.astype(np.int16)
    samples.tofile(filename)
    print(f"  → {filename}: {len(samples)} samples, {len(samples)/SAMPLE_RATE:.3f}s, peak={np.max(np.abs(samples)):.0f}")

def write_wav(filename: str, samples: np.ndarray):
    """Write WAV for local listening."""
    samples = np.clip(samples, -1.0, 1.0) * 32767
    samples = samples.astype(np.int16)
    with wave.open(filename, 'w') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SAMPLE_RATE)
        w.writeframes(samples.tobytes())
    print(f"  → {filename}: {len(samples)} samples")

def gen_catch() -> np.ndarray:
    """Happy ding: harmonic ping with rising frequency + fast decay."""
    dur = 0.20
    n = int(SAMPLE_RATE * dur)
    t = np.arange(n, dtype=np.float64) / SAMPLE_RATE
    # Fundamental: 800→1600 Hz sweep
    freq_sweep = 800 + 800 * (t / dur)
    # Harmonics 2x and 3x
    s = (np.sin(2 * np.pi * freq_sweep * t) +
         0.5 * np.sin(2 * np.pi * freq_sweep * 2 * t) +
         0.3 * np.sin(2 * np.pi * freq_sweep * 3 * t))
    # Envelope: fast attack, exponential decay
    env = np.exp(-t * 25) * MAX_AMP
    return s * env

def gen_miss() -> np.ndarray:
    """Buzz/bump: square wave + noise burst, low pitch."""
    dur = 0.30
    n = int(SAMPLE_RATE * dur)
    t = np.arange(n, dtype=np.float64) / SAMPLE_RATE
    # Square wave at 200 Hz (rich harmonics)
    sq = np.sign(np.sin(2 * np.pi * 200 * t))
    # Noise burst
    noise = np.random.uniform(-1, 1, n)
    noise_env = np.exp(-t * 20) * 0.4
    # Envelope
    env = np.exp(-t * 12) * MAX_AMP
    s = (sq + noise * 0.3) * env
    return s

def gen_color_change() -> np.ndarray:
    """Quick chirp: sine sweep up fast."""
    dur = 0.10
    n = int(SAMPLE_RATE * dur)
    t = np.arange(n, dtype=np.float64) / SAMPLE_RATE
    # Sweep 400→2000 Hz
    freq_start, freq_end = 400, 2000
    freq = freq_start + (freq_end - freq_start) * (t / dur)
    s = np.sin(2 * np.pi * freq * t)
    env = np.exp(-t * 40) * MAX_AMP
    return s * env

def gen_game_over() -> np.ndarray:
    """Sad cascade: 4 descending tones with decay + sustain."""
    dur = 1.0
    n = int(SAMPLE_RATE * dur)
    t = np.arange(n, dtype=np.float64) / SAMPLE_RATE
    notes = [400, 320, 260, 200]  # descending notes
    note_len = n // len(notes)
    s = np.zeros(n, dtype=np.float64)
    for i, freq in enumerate(notes):
        start = i * note_len
        end = start + note_len
        tn = t[start:end] - t[start]
        tone = np.sin(2 * np.pi * freq * tn)
        # Each note: fast attack, slow decay
        env_note = np.exp(-tn * 6) * MAX_AMP
        s[start:end] += tone * env_note
    return s

def main():
    print("=== Gruniożerca Saturn — SFX Generator ===\n")
    os.makedirs(OUT_DIR, exist_ok=True)

    print("[CATCH]")
    catch = gen_catch()
    write_raw_pcm(f"{OUT_DIR}/CATCH.PCM", catch)
    write_wav(f"{OUT_DIR}/CATCH.WAV", catch)

    print("[MISS]")
    miss = gen_miss()
    write_raw_pcm(f"{OUT_DIR}/MISS.PCM", miss)
    write_wav(f"{OUT_DIR}/MISS.WAV", miss)

    print("[CHGCOL]")
    chgcol = gen_color_change()
    write_raw_pcm(f"{OUT_DIR}/CHGCOL.PCM", chgcol)
    write_wav(f"{OUT_DIR}/CHGCOL.WAV", chgcol)

    print("[GAMEOVR]")
    gameover = gen_game_over()
    write_raw_pcm(f"{OUT_DIR}/GAMEOVR.PCM", gameover)
    write_wav(f"{OUT_DIR}/GAMEOVR.WAV", gameover)

    print("\nDone. All SFX written to cd/.")

if __name__ == "__main__":
    main()
