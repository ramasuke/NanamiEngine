"""FirstEventDragon が島に降り立つときの着地音を合成し、Assets/Audio/Enemy/FirstEventDragon に SoundFile アセットとして入れる。

    python tools/art/dragon_landing_sfx.py [--preview out.png]

FirstEventDragon.enemyBehaviourData の着地シーケンス (State0 の ToTouchDownAnimation の後) で PlaySE が鳴らす。
ファイルの IMPACT_SECS が後ろ足の接地で、BT はそこから逆算して鳴らし始める (Touchdown Sound Delay)。

接地の時刻は AnimationView で FlyingToLanding.mv1 の骨をフレームごとに読んだもの (2026-09-29):
- フレーム 16〜17 で右足 (Bip001-R-Foot / Toe0) が地面の高さに届き、骨盤の落下が急に止まる = 後ろ足の着地。
- フレーム 20〜22 で前足 (Bip001-L/R-Hand) が下り切る = 前足の着地 (後ろ足の約 0.25 秒後)。
アニメツリーの FlyingToLanding は speed_ 19.8 (フレーム/秒) なので、後ろ足はアニメ開始から約 0.86 秒。
クリップや速度を変えたら測り直し、BT の待ち時間も合わせること。

音作り (ui_sfx.py の方針どおり低く写実的に):
- 接地前: 翼が空気を叩きつける低い風圧 (帯域ノイズのうねり)
- 接地: 巨体の重さが乗る超低音の衝撃 + 地面の鈍い共振 + 土が潰れる音
- 前足: 少し軽い 2 発目
- 余韻: 飛び散る土砂・小石 + 地鳴り、屋外の残響
"""
from __future__ import annotations

import argparse
import math
import sys
import uuid
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import magic_sfx as m  # noqa: E402
import ui_sfx as ui  # noqa: E402

OUT_DIR = m.REPO / 'Assets/Audio/Enemy/FirstEventDragon'

# NOTE: BT の Touchdown Sound Delay (= 接地 0.86 秒 - IMPACT_SECS) と対になっている
IMPACT_SECS = 0.3
FRONT_FEET_SECS = IMPACT_SECS + 0.25


def downdraft(d, rng):
    """着地直前、翼で空気を下に押しつける低い風圧"""
    fc = m.curve(d, [(0, 180), (IMPACT_SECS - 0.05, 420), (IMPACT_SECS + 0.15, 200), (d, 150)], 'exp')
    x = m.sweep(m.brown(d, rng) + m.white(d, rng) * 0.3, fc, q=0.8)
    env = m.curve(d, [(0, 0), (IMPACT_SECS * 0.7, 0.8), (IMPACT_SECS, 1.0), (IMPACT_SECS + 0.35, 0.2), (d, 0)])
    return m.norm(x) * env


def slam(d, rng, start, weight=1.0):
    """巨体が地面に乗る衝撃: 下がっていく超低音 + 地面の鈍い共振 + 土が潰れる音"""
    n = m.n_of(d)
    body = np.zeros(n)
    rest = d - start
    boom = m.osc(m.curve(rest, [(0, 62), (0.35, 30)], 'exp'), rest) * m.exp_decay(rest, 0.32 * weight, 0.004)
    ground = ui.resonate(ui.excite(rest, rng, 0.012, 500), [(48, 0.35, 1.0), (86, 0.2, 0.6), (131, 0.12, 0.35)])
    crunch = m.sat(m.bp(m.white(rest, rng), 140, 1100) * m.exp_decay(rest, 0.07, 0.002), 2.5)
    m.at(body, m.norm(boom) * 1.0 + m.norm(ground) * 0.7 + m.norm(crunch) * 0.7, start)
    return body * weight


def debris(d, rng):
    """跳ね上がって落ちる土砂・小石 (接地から 1 秒ほど)"""
    rate = lambda t: 0.0 if t < IMPACT_SECS + 0.02 else 180 * math.exp(-(t - IMPACT_SECS) / 0.4)  # noqa: E731
    return m.grains(d, rng, m.poisson_times(d, rng, rate), 300, 2400, (0.006, 0.03), (0.2, 0.9))


def rumble(d, rng):
    """地鳴りの余韻"""
    env = m.curve(d, [(0, 0), (IMPACT_SECS, 0), (IMPACT_SECS + 0.06, 1.0), (d, 0)])
    return m.lp(m.brown(d, rng), 110) * env


def dragon_landing_impact(rng):
    d = 2.6
    x = (downdraft(d, rng) * 0.45
         + slam(d, rng, IMPACT_SECS, 1.0)
         + slam(d, rng, FRONT_FEET_SECS, 0.55)
         + m.norm(debris(d, rng)) * 0.45
         + m.norm(rumble(d, rng)) * 0.5)
    x = ui.muffle(m.sat(x, 1.4), 2600)
    return ui.open_air(x, rng, rt60=1.6, mix=0.3, bright=1400)


SOUNDS = {
    'DragonLanding_Impact': dragon_landing_impact,
}

# NOTE: TRex_ChargeImpact (-10 dB) と同じ段。島に響く一番大きい演出なので少し上げる
LOUDNESS = {
    'DragonLanding_Impact': -9.0,
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
    ap = argparse.ArgumentParser()
    ap.add_argument('--preview')
    args = ap.parse_args()
    rendered = {}
    for i, (name, recipe) in enumerate(SOUNDS.items()):
        stereo = m.trim_tail(recipe(np.random.default_rng(9100 + i)))
        stereo = ui.match_loudness(stereo, LOUDNESS[name], max_squash_db=5.0)
        rendered[name] = stereo
        path = write_sound(name, stereo)
        mono = stereo.mean(axis=0)
        f = np.fft.rfftfreq(len(mono), 1 / m.SR)
        power = np.abs(np.fft.rfft(mono)) ** 2
        print(f'{name:24s} {stereo.shape[1] / m.SR:5.2f}s  {path.stat().st_size // 1024:3d} KB  '
              f'A {ui.a_weighted_level(stereo):6.1f} dB  peak {20 * np.log10(np.max(np.abs(stereo))):5.1f} dBFS  '
              f'centroid {np.sum(f * power) / np.sum(power):5.0f} Hz')
    if args.preview:
        m.render_preview(rendered, args.preview)


if __name__ == '__main__':
    main()
