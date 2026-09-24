"""SwordMan のダッシュ / ジャンプ攻撃の効果音を合成し、Assets/Audio/Physics に SoundFile アセットとして入れる。

    python tools/art/swordman_sfx.py [--only NAME ...] [--audition DIR] [--preview PATH]

--audition は試聴用に全候補 (<Name>_<Variant>.mp3) を DIR に書き出し、何もインストールしない。指定しなければ
各音の選ばれた候補 (CHOSEN) をインストールする。処理は magic_sfx.py と同じ (numpy + scipy, lameenc 192 kbps,
48 kHz ステレオ)。既存の .mp3.meta は guid を保つ。
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import magic_sfx as m  # noqa: E402
from magic_sfx import (SR, at, bp, curve, exp_decay, finish, hp, lp, norm, osc, reverb, sat, sweep,  # noqa: E402
                       trim_tail, white)

m.OUT_DIR = m.REPO / 'Assets/Audio/Physics'


# ================================================================ レイヤー
def _swish(d, rng, f0, f_peak, f1, t_peak, q=2.5, attack=0.03):
    env = curve(d, [(0, 0), (attack, 1), (t_peak + 0.04, 0.8), (d, 0)]) ** 1.4
    return sweep(white(d, rng), curve(d, [(0, f0), (t_peak, f_peak), (d, f1)], 'exp'), q) * env


def _shing(d, t0, freqs=(2900, 4350, 6100), tau=0.12):
    """金属の刃の響き (部分音は少し非整数倍)。"""
    ring = sum(osc(f, d) * (0.7 ** i) for i, f in enumerate(freqs))
    env = np.zeros(len(ring))
    at(env, exp_decay(d, tau, 0.002), t0)
    return ring * env


def _thud(d, f=(95, 45), tau=0.12, t0=0.0):
    out = np.zeros(m.n_of(d))
    body = osc(curve(d, [(0, f[0]), (0.15, f[1])], 'exp'), d) * exp_decay(d, tau, 0.002)
    return at(out, body, t0)


def _slice(d, rng, t0, tau=0.018, lo=2500, hi=10000):
    out = np.zeros(m.n_of(d))
    return at(out, bp(white(d, rng), lo, hi) * exp_decay(d, tau, 0.0008), t0)


def _flesh(d, rng, t0, tau=0.06):
    out = np.zeros(m.n_of(d))
    return at(out, sat(bp(white(d, rng), 180, 1400) * exp_decay(d, tau, 0.001), 2.5), t0)


def _rubble(d, rng, t0, count=60, span=0.35):
    times = [t0 + span * rng.random() ** 2 for _ in range(count)]
    return m.grains(d, rng, times, 500, 4500, (0.003, 0.02), (0.15, 1.0))


def _ground_slam(d, rng, t0, weight=1.0):
    boom = _thud(d, (70, 32), 0.22 * weight, t0)
    crack = np.zeros(m.n_of(d))
    at(crack, sat(bp(white(d, rng), 120, 2500) * exp_decay(d, 0.07 * weight, 0.001), 3.0), t0)
    return norm(boom) * 1.0 + norm(crack) * 0.6 + norm(_rubble(d, rng, t0 + 0.02)) * 0.3 * weight


# ================================================================ DashAttack: 踏み込み居合い (発生 0.53s / 全体 0.61s、ヒット判定の瞬間に鳴る)
def _whoosh_body(d, rng, t_peak, lo=90, hi=700):
    """風切りの胴鳴り: 重い刀身が空気を押す低い「ブォッ」"""
    env = curve(d, [(0, 0), (t_peak, 1), (d, 0)]) ** 2
    return bp(white(d, rng), lo, hi) * env


def dash_whiff_a(rng):
    """鋭い居合い(重め): 風切りを下げ、低い胴鳴りと踏み込みを足す"""
    d = 0.28
    swish = _swish(d, rng, 900, 5200, 1600, 0.06, 2.4, 0.015)
    step = _thud(d, (110, 50), 0.05)
    mix = norm(swish) * 0.8 + norm(_whoosh_body(d, rng, 0.06)) * 0.6 + norm(step) * 0.55
    return finish(reverb(sat(mix + _shing(d, 0.02, (2300, 3450), 0.05) * 0.1, 1.4), rng, 0.15, 0.1), -3)


def dash_whiff_b(rng):
    """重い薙ぎ払い: 低く太い風切り + 強い踏み込み"""
    d = 0.34
    swish = _swish(d, rng, 400, 2800, 700, 0.09, 1.8, 0.03)
    step = _thud(d, (100, 42), 0.07) + bp(white(d, rng), 150, 700) * exp_decay(d, 0.03, 0.001) * 0.6
    mix = norm(swish) + norm(_whoosh_body(d, rng, 0.09, 70, 500)) * 0.6 + norm(step) * 0.7
    return finish(reverb(sat(mix, 1.5), rng, 0.15, 0.1), -3)


def dash_whiff_c(rng):
    """二段の抜刀(重め): 鞘走り → 低めの本振り"""
    d = 0.36
    draw = np.zeros(m.n_of(d))
    at(draw, bp(white(0.1, rng), 2500, 7000) * curve(0.1, [(0, 0), (0.07, 1), (0.1, 0)]) ** 2, 0.0)
    swish = np.zeros(m.n_of(d))
    at(swish, _swish(0.26, rng, 800, 4200, 1200, 0.06, 2.2, 0.012), 0.09)
    body = np.zeros(m.n_of(d))
    at(body, _whoosh_body(0.26, rng, 0.06), 0.09)
    mix = norm(draw) * 0.3 + norm(swish) + norm(body) * 0.6 + norm(_thud(d, (105, 48), 0.05, 0.1)) * 0.5
    return finish(reverb(sat(mix, 1.4), rng, 0.15, 0.1), -3)


def dash_hit_a(rng):
    """斬り抜け(重め): 斬撃 + 太い肉の手応え + 低い衝撃"""
    d = 0.32
    swish = _swish(d, rng, 1200, 5000, 1800, 0.04, 2.5, 0.01)
    mix = (norm(_slice(d, rng, 0.02, 0.02, 1800, 8000)) * 0.6 + norm(_flesh(d, rng, 0.025, 0.06)) * 0.9
           + norm(_thud(d, (90, 40), 0.08, 0.025)) * 1.0 + norm(swish) * 0.3)
    return finish(reverb(sat(mix, 1.9), rng, 0.2, 0.12), -1)


def dash_hit_b(rng):
    """重い一閃: 斬撃 + 深い衝撃 + 短い刃鳴り"""
    d = 0.4
    mix = (norm(_slice(d, rng, 0.0, 0.025, 1500, 7000)) * 0.6 + norm(_flesh(d, rng, 0.005, 0.07)) * 0.8
           + norm(_thud(d, (75, 32), 0.12, 0.005)) * 1.1 + _shing(d, 0.01, (2100, 3200, 4600), 0.07) * 0.12)
    return finish(reverb(sat(mix, 2.2), rng, 0.25, 0.12), -1)


# ================================================================ JumpAttack: 空中で溜め → 真下へ急降下 → 着地で叩きつけ (岩が突き出る)
def plunge_a(rng):
    """急降下の風切り (降下開始時に鳴らす想定): 上がって落ちるピッチの長い風音"""
    d = 0.6
    swish = _swish(d, rng, 500, 2600, 400, 0.18, 1.6, 0.08)
    return finish(reverb(norm(swish), rng, 0.3, 0.15), -5)


def plunge_b(rng):
    """急降下: 振りかぶりの短い刃鳴り + 落下の風"""
    d = 0.6
    ring = _shing(d, 0.0, (3300, 4900), 0.1)
    wind = np.zeros(m.n_of(d))
    at(wind, _swish(0.5, rng, 700, 2000, 350, 0.25, 1.4, 0.15), 0.08)
    return finish(reverb(ring * 0.25 + norm(wind), rng, 0.35, 0.15), -5)


def slam_whiff_a(rng):
    """叩きつけ(空振り): 重い地響き + 岩の砕け"""
    d = 1.0
    return finish(reverb(sat(_ground_slam(d, rng, 0.0), 1.6), rng, 0.8, 0.22), -1, fade_out=0.15)


def slam_whiff_b(rng):
    """叩きつけ(空振り): 振り下ろしの風切り → 着地の衝撃(少し軽め)"""
    d = 1.0
    swish = _swish(d, rng, 900, 4500, 1500, 0.05, 2.2, 0.02) * curve(d, [(0, 1), (0.1, 1), (0.14, 0), (d, 0)])
    return finish(reverb(sat(norm(swish) * 0.5 + _ground_slam(d, rng, 0.08, 0.8), 1.5), rng, 0.7, 0.22), -1,
                  fade_out=0.15)


def slam_hit_a(rng):
    """叩きつけ(ヒット): 地響き + 斬撃 + 肉の手応え"""
    d = 1.0
    mix = (_ground_slam(d, rng, 0.0) + norm(_slice(d, rng, 0.0, 0.025)) * 0.6
           + norm(_flesh(d, rng, 0.005, 0.08)) * 0.7)
    return finish(reverb(sat(mix, 1.8), rng, 0.8, 0.22), -1, fade_out=0.15)


def slam_hit_b(rng):
    """叩きつけ(ヒット): 重い打撃を2発重ね(刃が入る → 地面に達する)"""
    d = 1.1
    mix = (norm(_flesh(d, rng, 0.0, 0.07)) * 0.8 + norm(_thud(d, (120, 60), 0.06)) * 0.6
           + _ground_slam(d, rng, 0.06, 1.1) + norm(_slice(d, rng, 0.0, 0.02)) * 0.5)
    return finish(reverb(sat(mix, 1.8), rng, 0.9, 0.22), -1, fade_out=0.15)


CANDIDATES = {
    'SwordDashAttack_Whiff': {'A': dash_whiff_a, 'B': dash_whiff_b, 'C': dash_whiff_c},
    'SwordDashAttack_Hit': {'A': dash_hit_a, 'B': dash_hit_b},
    'SwordJumpAttack_Plunge': {'A': plunge_a, 'B': plunge_b},
    'SwordJumpAttack_Whiff': {'A': slam_whiff_a, 'B': slam_whiff_b},
    'SwordJumpAttack_Hit': {'A': slam_hit_a, 'B': slam_hit_b},
}
CHOSEN = {name: 'A' for name in CANDIDATES} | {'SwordJumpAttack_Plunge': 'B'}


def render(name, variant):
    seed = 2000 + sorted(CANDIDATES).index(name) * 10 + sorted(CANDIDATES[name]).index(variant)
    return trim_tail(CANDIDATES[name][variant](np.random.default_rng(seed)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--only', nargs='*')
    ap.add_argument('--audition')
    ap.add_argument('--preview')
    args = ap.parse_args()
    rendered = {}
    for name in args.only or CANDIDATES:
        variants = CANDIDATES[name] if args.audition else [CHOSEN[name]]
        for variant in variants:
            stereo = render(name, variant)
            label = f'{name}_{variant}' if args.audition else name
            rendered[label] = stereo
            if args.audition:
                path = Path(args.audition) / f'{label}.mp3'
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(m.encode_mp3(stereo))
            else:
                path = m.write_sound(name, stereo)
            print(f'{label:28s} {stereo.shape[1] / SR:5.2f}s  {path.stat().st_size // 1024:3d} KB')
    if args.preview:
        m.render_preview(rendered, args.preview)


if __name__ == '__main__':
    main()
