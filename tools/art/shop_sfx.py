"""店 (露店) の効果音を合成し、Assets/Audio/UI に SoundFile アセットとして入れる。

    python tools/art/shop_sfx.py

Shop_Purchase: 買えたとき (ShopPresenter.purchaseSound_)。硬貨が数枚、革袋に落ちて鳴る
Shop_Refuse:   お金が足りない / これ以上持てないとき (ShopPresenter.refuseSound_)。帳場の台を指で2回叩く鈍い音
プリミティブ・mp3 エンコーダ・.meta テンプレートは tools/art/magic_sfx.py のものを使うので、形式は他の SE と同じ
(MPEG-1 Layer III, 192 kbps, 48 kHz, ステレオ。volume_ は 255、音量は波形のピークで決める)。
"""
from __future__ import annotations

import sys
import uuid
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import magic_sfx as m  # noqa: E402

OUT_DIR = m.REPO / 'Assets/Audio/UI'


def coin_clink(f, tau):
    """硬貨同士が当たる音。整数倍でない部分音を重ねて金属らしくする"""
    d = tau * 6
    partials = [(1.0, 1.0), (2.76, 0.55), (5.40, 0.3), (8.93, 0.15)]
    x = sum(m.osc(f * ratio, d) * gain * m.exp_decay(d, tau / (1 + ratio * 0.25), 0.0008) for ratio, gain in partials)
    return x


def shop_purchase(rng):
    d = 0.75
    buf = np.zeros(m.n_of(d))
    # 袋の口に落ちる革の鈍い音
    thud = m.lp(m.white(0.12, rng) * m.exp_decay(0.12, 0.035, 0.003), 380)
    m.at(buf, m.norm(thud), 0.035, 0.55)
    t = 0.0
    for i in range(6):
        f = 2600 + 1500 * rng.random()
        m.at(buf, m.norm(coin_clink(f, 0.05 + 0.06 * rng.random())), t, 0.85 * (0.82 ** i))
        t += 0.045 + 0.05 * rng.random() * (1 - i / 7)
    stereo = m.reverb(buf, rng, rt60=0.35, mix=0.14, predelay=0.006, bright=7000)
    return m.finish(stereo, -3.0, fade_out=0.08)


def shop_refuse(rng):
    d = 0.32
    buf = np.zeros(m.n_of(d))
    for start, gain in ((0.0, 1.0), (0.11, 0.8)):
        knock = m.osc(210, 0.1) * m.exp_decay(0.1, 0.03, 0.001) * 0.7
        knock += m.bp(m.white(0.1, rng), 180, 900) * m.exp_decay(0.1, 0.02, 0.001)
        m.at(buf, m.norm(knock), start, gain)
    stereo = m.reverb(buf, rng, rt60=0.25, mix=0.1, predelay=0.004, bright=3000)
    return m.finish(stereo, -6.0, fade_out=0.05)


SOUNDS = {
    'Shop_Purchase': shop_purchase,
    'Shop_Refuse': shop_refuse,
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
        stereo = m.trim_tail(recipe(np.random.default_rng(2000 + i)))
        path = write_sound(name, stereo)
        print(f'{name:16s} {stereo.shape[1] / m.SR:5.2f}s  {path.stat().st_size // 1024:3d} KB')


if __name__ == '__main__':
    main()
