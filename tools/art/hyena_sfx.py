"""Synthesize the hyena vocalisations and install them as SoundFile assets under Assets/Audio/Physics.

    python tools/art/hyena_sfx.py

HyenaHowl: 仲間を呼ぶ遠吠え (HyenaBehaviour の CallAllies.howlSound_、howlSeconds_ 約2.2秒)。
           狼寄りの "アオーーン": 唸り混じりの立ち上がりから長く伸ばしてビブラートで落ちる + 遠くで応える2頭目
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


def voice(f0, formants, amp, rng, jitter=0.02, rough=0.0, breath=0.12, tilt=0.6):
    """Harmonic voice. f0/amp are per-sample arrays, formants is [(freq_array, bandwidth, gain), ...].
    rough (scalar or per-sample) adds a subharmonic (f0/2) so the call sounds hoarse instead of like a whistle.
    tilt is the harmonic roll-off: higher = purer, more tonal."""
    n = len(f0)
    f0 = f0 * (1 + jitter * _smooth_noise(n, rng, 30))
    phase = np.cumsum(2 * np.pi * f0 / m.SR)
    out = np.zeros(n)
    for k in range(1, 48):
        fk = f0 * k
        env = sum(g / (1 + ((fk - ff) / (bw * 0.5)) ** 2) for ff, bw, g in formants)
        env = env * (fk < 9000) / k ** tilt
        out += env * np.sin(k * phase + rng.uniform(0, 2 * np.pi))
    sub = np.sin(phase * 0.5) * sum(g / (1 + ((f0 * 0.5 - ff) / (bw * 0.5)) ** 2) for ff, bw, g in formants)
    out += sub * rough
    out = m.norm(out)
    noise = m.bp(rng.standard_normal(n), 400, 3500) * 0.1
    return (out + m.norm(noise) * breath) * amp


def howl(d, rng, f_lo, f_hi, f_end):
    """アオーーン: a throaty scoop up to a long tonal hold with vibrato, then a falling tail."""
    n = m.n_of(d)
    f0 = m.curve(d, [(0, f_lo), (d * 0.16, f_hi * 0.94), (d * 0.55, f_hi), (d * 0.72, f_hi * 0.97), (d, f_end)], 'exp')
    vib_depth = m.curve(d, [(0, 0.0), (d * 0.3, 0.006), (d * 0.7, 0.016), (d, 0.022)])
    f0 = f0 * (1 + vib_depth * np.sin(2 * np.pi * 5.2 * m.tt(d)))
    # "ア" opening to "オ" and closing to "ウ" at the tail
    f1 = m.curve(d, [(0, 650), (d * 0.2, 520), (d * 0.75, 460), (d, 380)])
    f2 = m.curve(d, [(0, 1150), (d * 0.2, 950), (d * 0.75, 850), (d, 760)])
    f3 = np.full(n, 2600.0)
    amp = m.curve(d, [(0, 0), (d * 0.1, 0.75), (d * 0.3, 1.0), (d * 0.7, 0.9), (d, 0)]) ** 1.3
    amp *= 1 + 0.06 * _smooth_noise(n, rng, 8)
    # growly onset, clean hold, slight hoarseness again as the breath runs out
    rough = m.curve(d, [(0, 0.7), (d * 0.18, 0.1), (d * 0.75, 0.05), (d, 0.25)])
    return voice(f0, [(f1, 160, 1.0), (f2, 220, 0.35), (f3, 400, 0.08)], amp, rng,
                 jitter=0.006, rough=rough, breath=0.1, tilt=1.1)


def hyena_howl(rng):
    d = 2.2
    buf = np.zeros(m.n_of(d))
    m.at(buf, howl(2.05, rng, 260, 470, 300), 0.0, 1.0)
    # a second, distant voice answering underneath so it reads as a pack call
    m.at(buf, m.lp(howl(1.3, rng, 330, 560, 400), 3000), 0.75, 0.28)
    x = m.sat(buf, 1.2)
    return m.finish(m.reverb(x, rng, 1.6, 0.3, predelay=0.03, bright=4500), -2, fade_out=0.12)


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
