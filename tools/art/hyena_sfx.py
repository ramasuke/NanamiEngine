"""ハイエナの鳴き声を合成し、Assets/Audio/Physics に SoundFile アセットとして入れる。

    python tools/art/hyena_sfx.py

HyenaHowl: 仲間を呼ぶ遠吠え (HyenaBehaviour の CallAllies.howlSound_、howlSeconds_ 約2.2秒)。
           狼寄りの "アオーーン": 唸り混じりの立ち上がりから長く伸ばしてビブラートで落ちる + 遠くで応える2頭目
声は加算合成: 揺らぎのある声帯ピッチの倍音を、動くフォルマント包絡に沿った振幅で重ね、息のノイズを少し足す。
プリミティブ・mp3 エンコーダ・.meta テンプレートは tools/art/magic_sfx.py のものを使うので、形式は他の SE と同じ
(MPEG-1 Layer III, 192 kbps, 48 kHz, ステレオ。volume_ は 255、音量はピークで決める)。
既存の .mp3.meta は guid を保つ。
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
    """[-1, 1] の帯域制限ランダム揺らぎ (ピッチのジッター / 振幅のシマー)。"""
    return m.norm(m.lp(rng.standard_normal(n), rate_hz, order=2))


def voice(f0, formants, amp, rng, jitter=0.02, rough=0.0, breath=0.12, tilt=0.6):
    """倍音で作る声。f0/amp はサンプルごとの配列、formants は [(freq_array, bandwidth, gain), ...]。
    rough (スカラーまたはサンプルごと) は低調波 (f0/2) を足し、口笛ではなくかすれた声にする。
    tilt は倍音の減衰: 大きいほど純粋で音程感が強い。"""
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
    """アオーーン: 喉を鳴らしてしゃくり上げ、ビブラートで長く伸ばしてから下がって終わる。"""
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
    # 唸る立ち上がり、澄んだ伸ばし、息が切れるとまた少しかすれる
    rough = m.curve(d, [(0, 0.7), (d * 0.18, 0.1), (d * 0.75, 0.05), (d, 0.25)])
    return voice(f0, [(f1, 160, 1.0), (f2, 220, 0.35), (f3, 400, 0.08)], amp, rng,
                 jitter=0.006, rough=rough, breath=0.1, tilt=1.1)


def hyena_howl(rng):
    d = 2.2
    buf = np.zeros(m.n_of(d))
    m.at(buf, howl(2.05, rng, 260, 470, 300), 0.0, 1.0)
    # 群れの呼び声に聞こえるよう、下で遠くの2頭目が応える
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
