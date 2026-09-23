"""Synthesize the hyena vocalisations and install them as SoundFile assets under Assets/Audio/Physics.

    python tools/art/hyena_sfx.py

HyenaHowl: 仲間を呼ぶ遠吠え (HyenaBehaviour の CallAllies.howlSound_、howlSeconds_ 約2.2秒)。
           ブチハイエナの "フーーウップ" と尻上がりに跳ね上がる whoop を2回 + 締めのキキキッという笑い声
The voice is additive: harmonics of a jittered glottal pitch whose amplitudes follow a moving formant envelope, plus
a little breath noise. The primitives, the mp3 encoder and the .meta template come from tools/art/magic_sfx.py, so the
format matches the other SE (MPEG-1 Layer III, 192 kbps, 48 kHz, stereo; volume_ 255 and loudness set by the peak).
An existing .mp3.meta keeps its guid.
"""
from __future__ import annotations

import sys
import uuid
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import magic_sfx as m  # noqa: E402

OUT_DIR = m.REPO / 'Assets/Audio/Physics'


def _smooth_noise(n, rng, rate_hz):
    """Band-limited random wobble in [-1, 1] (pitch jitter / amplitude shimmer)."""
    return m.norm(m.lp(rng.standard_normal(n), rate_hz, order=2))


def voice(f0, formants, amp, rng, jitter=0.02, rough=0.0, breath=0.12):
    """Harmonic voice. f0/amp are per-sample arrays, formants is [(freq_array, bandwidth, gain), ...].
    rough adds a subharmonic (f0/2) so the call sounds hoarse instead of like a whistle."""
    n = len(f0)
    f0 = f0 * (1 + jitter * _smooth_noise(n, rng, 30))
    phase = np.cumsum(2 * np.pi * f0 / m.SR)
    out = np.zeros(n)
    for k in range(1, 48):
        fk = f0 * k
        env = sum(g / (1 + ((fk - ff) / (bw * 0.5)) ** 2) for ff, bw, g in formants)
        env = env * (fk < 9000) / k ** 0.6
        out += env * np.sin(k * phase + rng.uniform(0, 2 * np.pi))
    if rough:
        sub = np.sin(phase * 0.5) * sum(g / (1 + ((f0 * 0.5 - ff) / (bw * 0.5)) ** 2) for ff, bw, g in formants)
        out += sub * rough
    out = m.norm(out)
    noise = m.bp(rng.standard_normal(n), 400, 3500) * 0.1
    return (out + m.norm(noise) * breath) * amp


def whoop(d, rng, f_lo, f_hi):
    """フーーウップ: low hoot that holds, then leaps up by more than an octave and breaks off."""
    rise = d * 0.62
    f0 = m.curve(d, [(0, f_lo * 0.92), (d * 0.12, f_lo), (rise, f_lo * 1.15), (d * 0.88, f_hi), (d, f_hi * 0.9)], 'exp')
    # "ウ" (F1 low, F2 low) opening towards "オ" as it climbs
    f1 = m.curve(d, [(0, 330), (rise, 380), (d, 560)])
    f2 = m.curve(d, [(0, 780), (rise, 850), (d, 1100)])
    f3 = m.curve(d, [(0, 2400), (d, 2700)])
    amp = m.curve(d, [(0, 0), (d * 0.08, 0.55), (rise, 0.7), (d * 0.85, 1.0), (d, 0)]) ** 1.2
    amp *= 1 + 0.08 * _smooth_noise(len(amp), rng, 12)
    return voice(f0, [(f1, 140, 1.0), (f2, 180, 0.45), (f3, 300, 0.12)], amp, rng, jitter=0.012, rough=0.15)


def giggle_note(d, rng, f):
    """笑い声の1音: a short nasal "ヒッ" that bends up and falls."""
    f0 = m.curve(d, [(0, f * 0.9), (d * 0.35, f * 1.08), (d, f * 0.8)], 'exp')
    f1 = np.full(len(f0), 620.0)
    f2 = np.full(len(f0), 1900.0)
    f3 = np.full(len(f0), 2900.0)
    amp = m.curve(d, [(0, 0), (d * 0.12, 1.0), (d * 0.5, 0.7), (d, 0)])
    return voice(f0, [(f1, 160, 1.0), (f2, 220, 0.6), (f3, 350, 0.25)], amp, rng, jitter=0.03, rough=0.45, breath=0.25)


def hyena_howl(rng):
    d = 2.2
    buf = np.zeros(m.n_of(d))
    m.at(buf, whoop(0.78, rng, 210, 620), 0.0, 0.9)
    m.at(buf, whoop(0.82, rng, 235, 700), 0.86, 1.0)
    # キキキッ: a falling, speeding-up cackle to finish
    t, period, f = 1.72, 0.085, 820
    for i in range(6):
        m.at(buf, giggle_note(0.065, rng, f), t, 0.55 * 0.88 ** i)
        t += period
        period *= 0.93
        f *= 0.95
    x = m.sat(buf, 1.3)
    return m.finish(m.reverb(x, rng, 0.9, 0.22, predelay=0.025, bright=5000), -2, fade_out=0.08)


SOUNDS = {
    'HyenaHowl': hyena_howl,
}


def write_sound(name, stereo):
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUT_DIR / f'{name}.mp3'
    path.write_bytes(m.encode_mp3(stereo))
    meta = Path(str(path) + '.meta')
    if not meta.exists():
        content = str(path.relative_to(m.REPO)).replace('/', '\\').replace('\\', '\\\\')
        text = m.META.format(path=content, guid=str(uuid.uuid4()).upper()).replace('\n', '\r\n')
        meta.write_bytes(text.encode('utf-8'))
    return path


def main():
    for i, (name, recipe) in enumerate(SOUNDS.items()):
        stereo = m.trim_tail(recipe(np.random.default_rng(4000 + i)))
        path = write_sound(name, stereo)
        print(f'{name:22s} {stereo.shape[1] / m.SR:5.2f}s  {path.stat().st_size // 1024:3d} KB')


if __name__ == '__main__':
    main()
