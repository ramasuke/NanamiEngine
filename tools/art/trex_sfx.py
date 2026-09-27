"""ティラノサウルスの攻撃・咆哮の効果音を、攻撃アニメーション (Assets/Art/Models/Monster/T-Rex.mv1) の動きに
合わせて合成し、Assets/Audio/Enemy/Tyrannosaurus に SoundFile アセットとして入れる。

    python tools/art/trex_sfx.py [--only NAME ...] [--preview out.png]

どの音も「アニメーションの 0 秒から鳴らす」前提で、ファイルの時刻 = クリップの時刻 (30 fps) になっている。
- 攻撃 (PhysicsAttack): animationSound_ に入れる (攻撃開始の Tick で鳴る)。
- 咆哮 (PlayAnimation): animationSound_.sound_ に入れ、waitAnimationSound_secs_ は 0。
- 突進の激突 (ChargeRush): impactSound_ に入れる (激突の瞬間 = mama cave bite の 0 秒)。

TIMELINES の時刻は、エディタの AnimationView から顎 (jt_Jaw_C) と頭 (jt_Head_C) の骨をフレームごとに読み、
顎が開き始める / 閉じ切る / 頭が一番速く動く時刻を拾ったもの。クリップを差し替えたら測り直すこと。

音作り (tools/art/ui_sfx.py の方針に合わせて低く・写実寄り):
- 声: 低い声帯ピッチ (唸り 35〜60 Hz、咆哮 60〜110 Hz) の倍音を、顎の開きで動くフォルマントに通す (hyena_sfx.voice)。
  基音に同期した喉のざらつき (パルス状のノイズ) と 1 オクターブ下のうなりを重ねる。
- 顎を閉じる音: 骨と歯が噛み合う硬い打撃 (共振器) + 濡れた口の音 + 胸に響く低い衝撃。
- 頭を振る音: 大きな体が空気を押す低い風圧 (帯域ノイズを掃引)。
- 音量は A 特性の聞こえ方で揃える (ui_sfx.match_loudness)。基準は HyenaHowl (-12 dB)。
既存の .mp3.meta は guid を保つ。
"""
from __future__ import annotations

import argparse
import sys
import uuid
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import hyena_sfx as hv  # noqa: E402
import magic_sfx as m  # noqa: E402
import ui_sfx as ui  # noqa: E402

OUT_DIR = m.REPO / 'Assets/Audio/Enemy/Tyrannosaurus'
TAIL_SECS = 1.2


# ================================================================ 部品
def vocal(d, rng, f0_pts, amp_pts, open_pts, rough=0.8, jitter=0.035, flutter=0.0, breath=0.35, rasp=0.45, sub=0.35):
    """恐竜の声。f0/amp/open は [(t, v), ...]。open (0〜1) は顎の開きで、フォルマントを上げる。
    flutter は唸りのガラガラした揺れ (20〜30 Hz の振幅の揺らぎ)。"""
    n = m.n_of(d)
    f0 = m.curve(d, f0_pts, 'exp')
    opening = m.curve(d, open_pts)
    amp = m.curve(d, amp_pts)
    if flutter:
        amp = amp * np.clip(1 + flutter * hv._smooth_noise(n, rng, 26), 0, None)
    formants = [(260 + 420 * opening, 170, 1.0), (620 + 520 * opening, 240, 0.45), (1900 + 500 * opening, 500, 0.12)]
    voiced = hv.voice(f0, formants, amp, rng, jitter=jitter, rough=rough, breath=breath, tilt=0.85)

    phase = np.cumsum(2 * np.pi * f0 / m.SR)
    pulse = (0.5 + 0.5 * np.cos(phase)) ** 6
    throat = m.bp(rng.standard_normal(n), 220, 2000) * pulse * amp
    rumble = m.lp(np.sin(phase * 0.5) * amp, 150)
    return m.norm(voiced) + m.norm(throat) * rasp + m.norm(rumble) * sub


def growl(d, rng, level_pts, open_pts, f0=(42, 55)):
    """喉の奥の低い唸り (構え・噛んだ後)。"""
    f0_pts = [(0, f0[0]), (d * 0.5, f0[1]), (d, f0[0])]
    return vocal(d, rng, f0_pts, level_pts, open_pts, rough=1.0, jitter=0.06, flutter=0.55, breath=0.5, rasp=0.6, sub=0.5)


def snarl(d, rng, t_open, t_peak, t_close, level=1.0, f0=(48, 88)):
    """口を開けながらの威嚇: 閉じた唸りから、開くにつれてピッチと明るさが上がり、閉じると切れる。"""
    amp = [(0, 0.0), (t_open * 0.5, 0.35), (t_open, 0.55), (t_peak, 1.0), (t_close, 0.8), (d, 0.0)]
    opening = [(0, 0.05), (t_open, 0.1), (t_peak, 1.0), (t_close, 0.6), (d, 0.1)]
    f0_pts = [(0, f0[0]), (t_open, f0[0] * 1.1), (t_peak, f0[1]), (t_close, f0[1] * 0.85), (d, f0[0])]
    return vocal(d, rng, f0_pts, amp, opening, rough=0.9, jitter=0.05, flutter=0.35, breath=0.4, rasp=0.55) * level


def roar(d, rng, t_full, t_close, f0_peak=100, level=1.0):
    """咆哮: 太い声 + 1 オクターブ下のうなり + 少し上の割れた成分。t_full で全開、t_close から閉じる。"""
    amp = [(0, 0.0), (0.12, 0.7), (t_full, 1.0), (t_close, 0.85), (d, 0.0)]
    opening = [(0, 0.3), (t_full, 1.0), (t_close, 0.9), (d, 0.15)]
    f0_pts = [(0, f0_peak * 0.62), (t_full, f0_peak), (t_full + (t_close - t_full) * 0.6, f0_peak * 0.93),
              (t_close, f0_peak * 0.8), (d, f0_peak * 0.5)]
    body = vocal(d, rng, f0_pts, amp, opening, rough=0.55, jitter=0.045, flutter=0.15, breath=0.3, rasp=0.5, sub=0.45)
    low_pts = [(t, f * 0.5) for t, f in f0_pts]
    low = vocal(d, rng, low_pts, amp, opening, rough=1.0, jitter=0.07, flutter=0.4, breath=0.4, rasp=0.7, sub=0.6)
    high_pts = [(t, f * 2.05) for t, f in f0_pts]
    high = m.lp(vocal(d, rng, high_pts, amp, opening, rough=0.4, jitter=0.03, breath=0.25, rasp=0.3, sub=0.0), 2600)
    mix = m.norm(body) + m.norm(low) * 0.7 + m.norm(high) * 0.3
    return m.sat(m.norm(mix), 1.6) * level


def inhale(d, rng):
    """咆哮の前に大きく息を吸う音。"""
    fc = m.curve(d, [(0, 260), (d, 900)], 'exp')
    x = m.sweep(m.white(d, rng), fc, q=0.9)
    return x * m.curve(d, [(0, 0), (d * 0.8, 1), (d, 0)]) ** 1.5


def whoosh(d, rng, t_peak):
    """大きな頭が空気を押す低い風圧。"""
    fc = m.curve(d, [(0, 110), (t_peak, 420), (d, 140)], 'exp')
    x = m.sweep(m.white(d, rng), fc, q=0.8) + m.lp(m.white(d, rng), 180) * 0.6
    return x * m.curve(d, [(0, 0), (t_peak, 1), (d, 0)]) ** 2.5


def jaw_snap(d, rng, weight=1.0):
    """顎を閉じる: 歯と骨がぶつかる硬い打撃 (2 列の歯で少しずれる) + 濡れた口の音 + 胸に響く低い衝撃。"""
    bone_modes = [(210, 0.05), (480, 0.03), (870, 0.02), (1450, 0.012)]
    bone = np.zeros(m.n_of(d))
    for delay, gain in ((0.0, 1.0), (0.011, 0.55)):
        hit = ui.resonate(ui.excite(d, rng, 0.0012, 3500), [(f, tau * weight, 1.0 / (i + 1)) for i, (f, tau) in enumerate(bone_modes)])
        m.at(bone, m.norm(hit), delay, gain)
    click = m.bp(m.white(d, rng), 1400, 3800) * m.exp_decay(d, 0.006, 0.0005)
    wet = m.sat(m.bp(m.white(d, rng), 200, 1100) * m.exp_decay(d, 0.045, 0.002), 2.5)
    thump = m.osc(m.curve(d, [(0, 95), (0.12, 42)], 'exp'), d) * m.exp_decay(d, 0.09 * weight, 0.002)
    return m.sat(m.norm(bone) + m.norm(click) * 0.2 + m.norm(wet) * 0.45 + m.norm(thump) * 0.9 * weight, 1.8)


def huff(d, rng):
    """噛んだ後に鼻から荒く吐く息 + 短い喉の音。"""
    breath = m.bp(m.white(d, rng), 120, 850) * m.exp_decay(d, 0.22, 0.04)
    grunt = vocal(0.35, rng, [(0, 48), (0.35, 36)], [(0, 0), (0.04, 1), (0.35, 0)], [(0, 0.2), (0.35, 0.05)],
                  rough=1.0, jitter=0.07, flutter=0.4, breath=0.5)
    return m.norm(breath) + m.norm(m.pad(grunt, m.n_of(d))) * 0.55


def grunt(d, rng, f0=62):
    """力を込めて頭を振るときの短い「グオッ」。"""
    return vocal(d, rng, [(0, f0 * 0.8), (d * 0.3, f0), (d, f0 * 0.6)], [(0, 0), (0.05, 1), (d * 0.5, 0.7), (d, 0)],
                 [(0, 0.3), (d * 0.3, 0.7), (d, 0.2)], rough=0.9, jitter=0.05, flutter=0.3, breath=0.4, rasp=0.6)


def stomp(d, rng):
    """重い足を踏み下ろす: 地面の低い衝撃 + 土の擦れ。"""
    boom = ui.drum(d, rng, 48, 0.28)
    dirt = m.lp(m.grains(d, rng, list(0.01 + 0.12 * rng.random(24) ** 2), 200, 1600, (0.004, 0.02)), 1800)
    return m.norm(boom) + m.norm(dirt) * 0.35


def crash(d, rng):
    """突進で頭から石にぶつかる: 深い衝撃 + 石の割れ + 崩れる破片。"""
    boom = m.osc(m.curve(d, [(0, 80), (0.3, 30)], 'exp'), d) * m.exp_decay(d, 0.35, 0.002)
    rock = ui.stone(d, rng, 95, 0.12, grit=0.8)
    crack = m.sat(m.bp(m.white(d, rng), 150, 2600) * m.exp_decay(d, 0.09, 0.001), 3.0)
    debris = m.grains(d, rng, [0.03 + 0.9 * rng.random() ** 2 for _ in range(70)], 300, 2500, (0.004, 0.025), (0.1, 0.8))
    hit = m.sat(m.norm(boom) * 1.1 + m.norm(rock) * 0.9 + m.norm(crack) * 0.8, 2.0)
    return hit + m.norm(m.lp(debris, 2500)) * 0.3


# ================================================================ 時間軸
# イベント: (種類, 開始秒, 長さ秒, 音量, 引数) 。開始秒 = アニメーションの時刻。
def render(length, events, rng, rt60=0.9, mix=0.22):
    d = length + TAIL_SECS
    buf = np.zeros(m.n_of(d))
    for kind, start, dur, gain, *args in events:
        kw = args[0] if args else {}
        layer = LAYERS[kind](dur, rng, **kw)
        m.at(buf, m.norm(layer), start, gain)
    buf = m.lp(buf, 3200, 4)
    return m.reverb(buf, rng, rt60=rt60, mix=mix, predelay=0.02, bright=1800)


LAYERS = {
    'growl': lambda d, rng, level=None, opening=None: growl(d, rng, level or [(0, 0), (d * 0.3, 1), (d, 0)],
                                                            opening or [(0, 0.05), (d, 0.1)]),
    'snarl': lambda d, rng, **kw: snarl(d, rng, **kw),
    'roar': lambda d, rng, **kw: roar(d, rng, **kw),
    'inhale': lambda d, rng: inhale(d, rng),
    'whoosh': lambda d, rng, t_peak=None: whoosh(d, rng, d * 0.6 if t_peak is None else t_peak),
    'snap': lambda d, rng, weight=1.0: jaw_snap(d, rng, weight),
    'huff': lambda d, rng: huff(d, rng),
    'grunt': lambda d, rng, f0=62: grunt(d, rng, f0),
    'stomp': lambda d, rng: stomp(d, rng),
    'crash': lambda d, rng: crash(d, rng),
}


def bite(length, snarl_start, t_open, t_peak, t_close, whoosh_peak, snap_t, level=1.0, huff_t=None, tail=None):
    """噛みつきの型: 構えの唸り -> 口を開けて威嚇 -> 頭を突き出す風圧 -> 顎を閉じる -> 鼻息 (-> 低い唸りの余韻)"""
    d = snap_t + 0.08 - snarl_start
    ev = [
        ('snarl', snarl_start, d, 0.55 * level,
         dict(t_open=t_open - snarl_start, t_peak=t_peak - snarl_start, t_close=t_close - snarl_start)),
        ('whoosh', whoosh_peak - 0.3, 0.5, 0.45, dict(t_peak=0.3)),
        ('snap', snap_t - 0.004, 0.4, 1.0, dict(weight=level)),
        ('huff', huff_t or snap_t + 0.2, 0.7, 0.3),
    ]
    if tail:
        ev.append(('growl', tail[0], tail[1] - tail[0], 0.3))
    return length, ev


# NOTE: 数値は顎と頭の骨を AnimationView で測ったもの (モジュール先頭の説明を参照)
TIMELINES = {
    # bite close left: 口は 0.70〜0.93 で開き 1.03〜1.13 で閉じる。頭は 0.93 で一番速い
    'TRex_BiteCloseLeft': bite(2.67, 0.15, 0.70, 0.93, 1.03, 0.93, 1.12, tail=(1.6, 2.5)),
    # bite 90 right: bite close left と同じ顎の動き
    'TRex_Bite90Right': bite(2.67, 0.15, 0.70, 0.93, 1.03, 0.93, 1.12, tail=(1.6, 2.5)),
    # bite close right: 0.67〜0.93 で開き、1.13 で閉じ切る
    'TRex_BiteCloseRight': bite(2.2, 0.2, 0.67, 0.93, 1.03, 1.05, 1.12, tail=(1.55, 2.1)),
    # chase bite / chase bite left: 0.27〜0.53 で大きく開き、0.63〜0.73 で閉じる
    'TRex_ChaseBite': bite(1.73, 0.05, 0.27, 0.53, 0.63, 0.68, 0.73, huff_t=0.95),
    'TRex_ChaseBiteLeft': bite(1.73, 0.05, 0.27, 0.53, 0.63, 0.68, 0.73, huff_t=0.95),
    # chase bite right: 0.67〜0.9 で開き、1.12 までに閉じる (ゆっくり閉じるので軽め)
    'TRex_ChaseBiteRight': bite(2.67, 0.3, 0.67, 0.9, 0.95, 1.1, 1.12, level=0.85, huff_t=1.45, tail=(1.8, 2.5)),
    # walk left/right bite: 0.07〜0.83 でゆっくり大きく開き、0.93〜1.13 で閉じる
    'TRex_WalkLeftBite': bite(2.7, 0.05, 0.3, 0.83, 0.93, 0.95, 1.12, tail=(1.55, 2.5)),
    'TRex_WalkRightBite': bite(2.7, 0.05, 0.3, 0.83, 0.93, 0.95, 1.12, tail=(1.55, 2.5)),
    # bite 90 left: 0.07〜0.33 で吠えるように開いて 0.9 までに閉じ、1.07〜1.33 で開き直して 1.53〜1.7 で噛む。
    # 2.07〜2.6 でもう一度開いて 2.63〜2.8 で軽く噛む
    'TRex_Bite90Left': (2.93, [
        ('snarl', 0.02, 0.95, 0.75, dict(t_open=0.05, t_peak=0.33, t_close=0.6, f0=(50, 92))),
        ('snarl', 0.95, 0.8, 0.7, dict(t_open=0.12, t_peak=0.38, t_close=0.55)),
        ('whoosh', 1.25, 0.55, 0.6, dict(t_peak=0.28)),
        ('snap', 1.676, 0.4, 1.0),
        ('huff', 1.9, 0.6, 0.35),
        ('growl', 2.05, 0.7, 0.4, dict(opening=[(0, 0.1), (0.25, 0.6), (0.55, 0.5), (0.7, 0.1)])),
        ('snap', 2.776, 0.35, 0.45, dict(weight=0.55)),
    ]),
    # chase head butt left: 顎は chase bite right と同じ動き (1.12 で軽く閉じる)。1.33〜1.9 で頭を振り上げ、1.5 が当たり
    'TRex_ChaseHeadButtLeft': (2.67, [
        ('snarl', 0.3, 0.9, 0.6, dict(t_open=0.37, t_peak=0.6, t_close=0.65)),
        ('snap', 1.116, 0.3, 0.35, dict(weight=0.5)),
        ('grunt', 1.33, 0.45, 0.8, dict(f0=64)),
        ('whoosh', 1.2, 0.8, 0.9, dict(t_peak=0.4)),
        ('huff', 2.0, 0.6, 0.4),
    ]),
    # run head butt right: 0.7〜1.03 で 98 度まで口を開け (吠える)、1.1〜1.23 で閉じる。頭は 1.13 で一番速い
    'TRex_RunHeadButtRight': (2.13, [
        ('growl', 0.1, 0.6, 0.35),
        ('snarl', 0.6, 0.66, 1.0, dict(t_open=0.1, t_peak=0.43, t_close=0.5, f0=(55, 100))),
        ('whoosh', 0.85, 0.5, 0.7, dict(t_peak=0.28)),
        ('snap', 1.216, 0.45, 1.0, dict(weight=1.2)),
        ('huff', 1.45, 0.6, 0.45),
        ('snap', 1.926, 0.3, 0.3, dict(weight=0.4)),
    ]),
    # head butt left: 0.1〜0.43 で頭を下げて構え、0.73 で小さく振り、0.9〜1.3 で口を開けて吠えながら 1.13 / 1.53 で
    # 大きく振る。1.33〜1.63 で口を閉じる
    'TRex_HeadButtLeft': (2.53, [
        ('growl', 0.05, 0.75, 0.45, dict(opening=[(0, 0.05), (0.75, 0.2)])),
        ('whoosh', 0.45, 0.45, 0.4, dict(t_peak=0.28)),
        ('snarl', 0.8, 0.9, 0.85, dict(t_open=0.1, t_peak=0.35, t_close=0.55, f0=(52, 86))),
        ('whoosh', 0.85, 0.5, 0.75, dict(t_peak=0.28)),
        ('whoosh', 1.2, 0.6, 1.0, dict(t_peak=0.33)),
        ('snap', 1.616, 0.35, 0.55, dict(weight=0.6)),
        ('huff', 1.9, 0.6, 0.4),
    ]),
    # roar / roar left / roar right: 0.1〜0.67 で息を吸い、0.67〜1.1 で口を開け、2.6〜3.1 で閉じる
    'TRex_Roar': (4.3, [
        ('inhale', 0.15, 0.55, 0.35),
        ('roar', 0.68, 2.5, 1.0, dict(t_full=0.4, t_close=1.95, f0_peak=100)),
        ('growl', 3.0, 0.9, 0.3),
    ]),
    'TRex_RoarLeft': (4.3, [
        ('inhale', 0.15, 0.55, 0.35),
        ('roar', 0.68, 2.5, 1.0, dict(t_full=0.4, t_close=1.95, f0_peak=96)),
        ('growl', 3.0, 0.9, 0.3),
    ]),
    'TRex_RoarRight': (4.3, [
        ('inhale', 0.15, 0.55, 0.35),
        ('roar', 0.68, 2.5, 1.0, dict(t_full=0.4, t_close=1.95, f0_peak=104)),
        ('growl', 3.0, 0.9, 0.3),
    ]),
    # Big roar step: 0.45〜1.05 で息を吸い、1.07〜1.5 で 100 度まで開けて 3.1 まで吠え続け、3.1〜3.43 で閉じる。
    # 2.23 で右足を踏み込む
    'TRex_BigRoarStep': (4.5, [
        ('inhale', 0.4, 0.65, 0.4),
        ('roar', 1.07, 2.4, 1.0, dict(t_full=0.45, t_close=2.0, f0_peak=92)),
        ('stomp', 2.22, 0.8, 0.7),
        ('growl', 3.35, 0.9, 0.3),
    ]),
    # 突進で石にぶつかった瞬間から (mama cave bite): 痛がって吠え、0.67〜1.0 で口を開けて 1.6 / 2.33 でもがいて噛む
    'TRex_ChargeImpact': (2.7, [
        ('crash', 0.0, 1.4, 1.0),
        ('snarl', 0.12, 0.75, 0.4, dict(t_open=0.05, t_peak=0.25, t_close=0.5, f0=(70, 95))),
        ('growl', 0.7, 0.95, 0.35, dict(opening=[(0, 0.2), (0.3, 0.7), (0.95, 0.2)])),
        ('snap', 1.6, 0.35, 0.28, dict(weight=0.6)),
        ('growl', 1.8, 0.55, 0.35),
        ('snap', 2.33, 0.35, 0.22, dict(weight=0.5)),
    ]),
}

# NOTE: 攻撃・咆哮は HyenaHowl (-12 dB) と同じ段。咆哮はさらに大きく、噛む前の唸りを含む攻撃音は少し控える
LOUDNESS = {
    'TRex_Roar': -10.0,
    'TRex_RoarLeft': -10.0,
    'TRex_RoarRight': -10.0,
    'TRex_BigRoarStep': -9.0,
    'TRex_ChargeImpact': -10.0,
}
LOUD_ATTACK = -12.0

SOUNDS = list(TIMELINES)


def make(name):
    rng = np.random.default_rng(7000 + SOUNDS.index(name))
    length, events = TIMELINES[name]
    roaring = any(e[0] == 'roar' for e in events)
    stereo = render(length, events, rng, rt60=1.5 if roaring else 0.9, mix=0.28 if roaring else 0.2)
    stereo = m.trim_tail(stereo)
    return ui.match_loudness(stereo, LOUDNESS.get(name, LOUD_ATTACK))


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
    ap.add_argument('--only', nargs='*')
    ap.add_argument('--preview')
    args = ap.parse_args()
    rendered = {}
    for name in args.only or SOUNDS:
        stereo = make(name)
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
