"""SwordMan のダッシュ / ジャンプ / カウンター攻撃の効果音を合成し、Assets/Audio/Physics に SoundFile アセットとして入れる。

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
import ui_sfx as u  # noqa: E402
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


# ================================================================ CounterAttack: ジャスト回避からの振り下ろし (発生 0.27s、判定の瞬間に鳴る)
def _cleave(d, rng, t0, dur=0.22, f_top=3800, f_end=260):
    """振り下ろしの風切り: 上から叩き落とすので音程が一気に下がる"""
    env = curve(dur, [(0, 0), (0.025, 1), (dur * 0.5, 0.6), (dur, 0)]) ** 1.3
    swish = sweep(white(dur, rng), curve(dur, [(0, f_top), (dur, f_end)], 'exp'), 2.0) * env
    body = bp(white(dur, rng), 70, 520) * env
    return at(np.zeros(m.n_of(d)), norm(swish) + norm(body) * 0.7, t0)


def _sha(d, rng, t0, dur=0.06):
    """刃を走らせる明るい擦れ (avoid_sfx の「シャ」を短く)"""
    fc = curve(dur, [(0, 2200), (dur, 8000)], 'exp')
    x = sweep(white(dur, rng), fc, 1.2) * curve(dur, [(0, 0), (0.005, 1), (dur, 0)]) ** 1.5
    return at(np.zeros(m.n_of(d)), x, t0)


def _boom(d, t0, f=(58, 26), tau=0.35):
    """腹に来る低い衝撃。_thud より長く低い"""
    body = osc(curve(d, [(0, f[0]), (0.3, f[1])], 'exp'), d) * exp_decay(d, tau, 0.003)
    return at(np.zeros(m.n_of(d)), body, t0)


def _bronze(d, t0, f=196, tau=0.9):
    """低い青銅の響き (音階にしない非整数倍の部分音)"""
    ring = sum(osc(f * r, d) * exp_decay(d, tau * k, 0.002) * g
               for r, k, g in [(1.0, 1.0, 1.0), (2.32, 0.6, 0.5), (3.61, 0.4, 0.3), (5.12, 0.25, 0.15)])
    return at(np.zeros(m.n_of(d)), lp(ring, 2500), t0)


def counter_whiff_a(rng):
    """重い縦斬り: 叩き落とす低い風切り + 短い刃鳴り"""
    d = 0.5
    mix = norm(_cleave(d, rng, 0.0)) + norm(_thud(d, (90, 40), 0.07, 0.12)) * 0.5
    return finish(reverb(sat(mix + _shing(d, 0.01, (2100, 3150), 0.06) * 0.12, 1.5), rng, 0.3, 0.14), -3)


def counter_whiff_b(rng):
    """派手な一閃: 刃のシャッ → 叩き落とす風切り → 低い衝撃の余韻"""
    d = 0.8
    mix = (norm(_sha(d, rng, 0.0)) * 0.55 + norm(_cleave(d, rng, 0.02, 0.24, 4500, 300))
           + norm(_boom(d, 0.14, (70, 32), 0.2)) * 0.6 + _shing(d, 0.0, (2600, 3900, 5500), 0.09) * 0.15)
    return finish(reverb(sat(mix, 1.5), rng, 0.6, 0.2), -3)


def counter_whiff_c(rng):
    """振りかぶり → 振り下ろし: 短く吸い込む風 → 太い風切り → 地面を打つ"""
    d = 0.6
    inhale = np.zeros(m.n_of(d))
    at(inhale, _swish(0.12, rng, 300, 1400, 1800, 0.1, 1.6, 0.08), 0.0)
    mix = (norm(inhale) * 0.35 + norm(_cleave(d, rng, 0.09, 0.2, 3200, 220))
           + norm(_ground_slam(d, rng, 0.24, 0.5)) * 0.55)
    return finish(reverb(sat(mix, 1.5), rng, 0.4, 0.16), -3)


def counter_hit_a(rng):
    """重い一刀両断: 斬撃 + 太い肉の手応え + 腹に来る衝撃 (明るい音なし)"""
    d = 1.0
    mix = (norm(_slice(d, rng, 0.0, 0.025, 1500, 7000)) * 0.6 + norm(_flesh(d, rng, 0.004, 0.09)) * 0.9
           + norm(_thud(d, (95, 40), 0.1, 0.004)) * 0.8 + norm(_boom(d, 0.004)) * 1.1
           + norm(_cleave(d, rng, 0.0, 0.12)) * 0.25)
    return finish(reverb(sat(mix, 2.0), rng, 0.8, 0.2), -1, fade_out=0.15)


def counter_hit_b(rng):
    """ズバァン (派手): 刃のシャッ + 斬撃 + 深い衝撃 + 長い刃鳴り"""
    d = 1.3
    mix = (norm(_sha(d, rng, 0.0)) * 0.45 + norm(_slice(d, rng, 0.01, 0.03, 1500, 8000)) * 0.6
           + norm(_flesh(d, rng, 0.012, 0.08)) * 0.8 + norm(_boom(d, 0.012, (62, 27), 0.4)) * 1.1
           + _shing(d, 0.012, (1850, 2780, 3960), 0.35) * 0.22)
    return finish(reverb(sat(mix, 1.9), rng, 1.1, 0.24), -1, fade_out=0.2)


def counter_hit_c(rng):
    """ズドン + 低い鐘: 重い斬撃に青銅の響きを重ね、決まった感を出す"""
    d = 1.5
    mix = (norm(_slice(d, rng, 0.0, 0.025, 1500, 7000)) * 0.55 + norm(_flesh(d, rng, 0.004, 0.08)) * 0.8
           + norm(_boom(d, 0.004, (60, 28), 0.35)) * 1.1 + norm(_bronze(d, 0.01)) * 0.5
           + norm(_rubble(d, rng, 0.03, 30, 0.25)) * 0.2)
    return finish(reverb(sat(mix, 1.8), rng, 1.2, 0.22), -1, fade_out=0.25)


def counter_hit_d(rng):
    """B と C の間: シャッ + 斬撃 + 衝撃 + 低い鐘と短い刃鳴り"""
    d = 1.4
    mix = (norm(_sha(d, rng, 0.0)) * 0.4 + norm(_slice(d, rng, 0.01, 0.028, 1500, 7500)) * 0.55
           + norm(_flesh(d, rng, 0.012, 0.08)) * 0.8 + norm(_boom(d, 0.012, (60, 27), 0.38)) * 1.1
           + norm(_bronze(d, 0.015, 220, 0.8)) * 0.35 + _shing(d, 0.012, (2100, 3150), 0.18) * 0.14)
    return finish(reverb(sat(mix, 1.9), rng, 1.1, 0.22), -1, fade_out=0.2)


# ---------------------------------------------------------------- 斬れ味重視 + エフェクトに同期 (tools/art/counter_attack_effect.py)
# CounterSlash: 0f 縦の一閃 / 3f (0.05s) 刃が地面を打つ (閃光・衝撃波・石片) / 火花 18〜32f / 砂煙 ~1s
# CounterImpact (ヒット時のみ): 0f 閃光 + 1 本目の斬撃 / 2f (0.033s) 交差する 2 本目 / 火花 ~0.5s / 残り火 ~1s
SLASH_GROUND_T = 3 / 60
IMPACT_CROSS_T = 2 / 60


def _cut(d, rng, t0, dur=0.1, f0=9000, f1=1600, q=1.3, tau=0.035):
    """刃が走る「ズバッ」: 立ち上がりの鋭いノイズを高い方から一気に掃き下ろす"""
    fc = curve(dur, [(0, f0), (dur, f1)], 'exp')
    x = sweep(white(dur, rng), fc, q) * exp_decay(dur, tau, 0.0006)
    return at(np.zeros(m.n_of(d)), x, t0)


def _tear(d, rng, t0, dur=0.09, lo=350, hi=3200):
    """肉と布を裂く手応え: 粒の荒い帯域ノイズ"""
    grain = np.abs(rng.standard_normal(m.n_of(dur)))
    grain = lp(grain, 180) * 3
    x = bp(white(dur, rng), lo, hi) * np.clip(grain, 0, 1.5) * exp_decay(dur, 0.035, 0.001)
    return at(np.zeros(m.n_of(d)), sat(x / (np.max(np.abs(x)) + 1e-9), 2.0), t0)


def _air_cut(d, rng, t0, dur=0.14):
    """刃が空気を裂く短く鋭い「ヒュッ」(重い風切りより高く速い)"""
    env = curve(dur, [(0, 0), (0.018, 1), (dur, 0)]) ** 2
    x = sweep(white(dur, rng), curve(dur, [(0, 2500), (0.02, 6500), (dur, 1400)], 'exp'), 2.6) * env
    return at(np.zeros(m.n_of(d)), x, t0)


def _ground_hit(d, rng, t0, weight=1.0):
    """刃が地面を打つ: 短い地響き + 石の割れ + 石片"""
    crack = np.zeros(m.n_of(d))
    at(crack, sat(bp(white(0.2, rng), 200, 3000) * exp_decay(0.2, 0.03 * weight, 0.0008), 3.0), t0)
    return (norm(_thud(d, (85, 36), 0.09 * weight, t0)) + norm(crack) * 0.55
            + norm(_rubble(d, rng, t0 + 0.02, 30, 0.3)) * 0.25 * weight)


def _spark_crackle(d, rng, t0, span=0.45, count=40):
    """散る火花のパチパチ (最初に密で、だんだん疎に)"""
    times = [t0 + span * rng.random() ** 2.2 for _ in range(count)]
    return m.grains(d, rng, times, 2500, 8000, (0.002, 0.007), (0.1, 0.6))


def _dust_tail(d, rng, t0, dur=0.8):
    """砂煙の低いざわめき"""
    x = lp(m.brown(dur, rng), 350) * curve(dur, [(0, 0), (0.05, 1), (dur, 0)]) ** 2
    return at(np.zeros(m.n_of(d)), x, t0)


def counter_whiff_d(rng):
    """斬れ味: 鋭い空気の裂け + 刃鳴り → 3f 後に地面を打つ + 火花 + 砂煙"""
    d = 1.0
    mix = (norm(_air_cut(d, rng, 0.0)) * 0.9 + norm(_cut(d, rng, 0.0, 0.08, 7000, 2000, 1.5, 0.025)) * 0.35
           + _shing(d, 0.0, (2400, 3600, 5100), 0.1) * 0.14
           + _ground_hit(d, rng, SLASH_GROUND_T, 0.8) * 0.7 + norm(_spark_crackle(d, rng, SLASH_GROUND_T)) * 0.2
           + norm(_dust_tail(d, rng, SLASH_GROUND_T + 0.03)) * 0.25)
    return finish(reverb(sat(mix, 1.4), rng, 0.6, 0.16), -3, fade_out=0.15)


def counter_whiff_e(rng):
    """D を重く: 空気の裂けに太い胴鳴りを足し、地面の衝撃を強く"""
    d = 1.0
    mix = (norm(_air_cut(d, rng, 0.0)) * 0.8 + norm(_cleave(d, rng, 0.0, 0.12, 3000, 300)) * 0.4
           + _shing(d, 0.0, (2200, 3300), 0.08) * 0.12
           + _ground_hit(d, rng, SLASH_GROUND_T, 1.1) * 0.85 + norm(_spark_crackle(d, rng, SLASH_GROUND_T)) * 0.15
           + norm(_dust_tail(d, rng, SLASH_GROUND_T + 0.03)) * 0.3)
    return finish(reverb(sat(mix, 1.5), rng, 0.6, 0.16), -3, fade_out=0.15)


def counter_hit_e(rng):
    """ズバッ・ザンッ: 1 本目の斬撃 + 裂く手応え → 2f 後に交差する 2 本目 → 地面 + 火花 + 残り火"""
    d = 1.3
    mix = (norm(_cut(d, rng, 0.0)) * 0.9 + norm(_tear(d, rng, 0.004)) * 0.6
           + norm(_cut(d, rng, IMPACT_CROSS_T, 0.09, 8000, 1800, 1.4, 0.03)) * 0.6
           + _shing(d, 0.0, (2300, 3450, 4900), 0.16) * 0.16 + norm(_thud(d, (100, 45), 0.06, 0.004)) * 0.55
           + _ground_hit(d, rng, SLASH_GROUND_T, 0.7) * 0.4 + norm(_spark_crackle(d, rng, 0.01, 0.5, 55)) * 0.22
           + norm(_dust_tail(d, rng, SLASH_GROUND_T + 0.03)) * 0.15)
    return finish(reverb(sat(mix, 1.6), rng, 0.8, 0.18), -1, fade_out=0.2)


def counter_hit_f(rng):
    """E をより鋭く: 二つの斬撃を強く、低音を控えめに、刃鳴りを長めに"""
    d = 1.4
    mix = (norm(_cut(d, rng, 0.0, 0.11, 10000, 2000, 1.2, 0.04)) + norm(_tear(d, rng, 0.004, 0.07, 500, 4000)) * 0.45
           + norm(_cut(d, rng, IMPACT_CROSS_T, 0.1, 9500, 2200, 1.3, 0.035)) * 0.8
           + _shing(d, 0.004, (2600, 3900, 5500), 0.3) * 0.2 + norm(_thud(d, (110, 50), 0.05, 0.004)) * 0.35
           + _ground_hit(d, rng, SLASH_GROUND_T, 0.6) * 0.3 + norm(_spark_crackle(d, rng, 0.01, 0.5, 55)) * 0.25)
    return finish(reverb(sat(mix, 1.5), rng, 0.9, 0.2), -1, fade_out=0.2)


def counter_hit_g(rng):
    """重い一太刀: 鋭い斬撃は 1 本、裂く手応えと衝撃を太く (交差の 2 本目は薄く)"""
    d = 1.3
    mix = (norm(_cut(d, rng, 0.0, 0.12, 8500, 1400, 1.3, 0.045)) * 0.9 + norm(_tear(d, rng, 0.004, 0.11)) * 0.8
           + norm(_cut(d, rng, IMPACT_CROSS_T, 0.08, 7000, 1800, 1.4, 0.025)) * 0.3
           + _shing(d, 0.0, (2100, 3150), 0.12) * 0.12 + norm(_thud(d, (90, 38), 0.1, 0.004)) * 0.8
           + norm(_boom(d, 0.004, (60, 28), 0.25)) * 0.5
           + _ground_hit(d, rng, SLASH_GROUND_T, 0.9) * 0.45 + norm(_spark_crackle(d, rng, 0.01, 0.45, 40)) * 0.18
           + norm(_dust_tail(d, rng, SLASH_GROUND_T + 0.03)) * 0.2)
    return finish(reverb(sat(mix, 1.7), rng, 0.8, 0.18), -1, fade_out=0.2)


CANDIDATES = {
    'SwordDashAttack_Whiff': {'A': dash_whiff_a, 'B': dash_whiff_b, 'C': dash_whiff_c},
    'SwordDashAttack_Hit': {'A': dash_hit_a, 'B': dash_hit_b},
    'SwordJumpAttack_Plunge': {'A': plunge_a, 'B': plunge_b},
    'SwordJumpAttack_Whiff': {'A': slam_whiff_a, 'B': slam_whiff_b},
    'SwordJumpAttack_Hit': {'A': slam_hit_a, 'B': slam_hit_b},
    'SwordCounterAttack_Whiff': {'A': counter_whiff_a, 'B': counter_whiff_b, 'C': counter_whiff_c,
                                 'D': counter_whiff_d, 'E': counter_whiff_e},
    'SwordCounterAttack_Hit': {'A': counter_hit_a, 'B': counter_hit_b, 'C': counter_hit_c, 'D': counter_hit_d,
                               'E': counter_hit_e, 'F': counter_hit_f, 'G': counter_hit_g},
}
CHOSEN = {name: 'A' for name in CANDIDATES} | {'SwordJumpAttack_Plunge': 'B', 'SwordCounterAttack_Whiff': 'D',
                                               'SwordCounterAttack_Hit': 'E'}


# NOTE: ダッシュ攻撃のヒットが A=-12.4 dB。カウンターは見せ場なので少し上げる (ダッシュ / ジャンプはピーク基準のまま)
LOUDNESS = {'SwordCounterAttack_Whiff': -13.0, 'SwordCounterAttack_Hit': -11.0}

# NOTE: 音を足しても既存の音の乱数が変わらないよう、名前の並びは追記だけにする
SEED_ORDER = ['SwordDashAttack_Hit', 'SwordDashAttack_Whiff', 'SwordJumpAttack_Hit', 'SwordJumpAttack_Plunge',
              'SwordJumpAttack_Whiff', 'SwordCounterAttack_Hit', 'SwordCounterAttack_Whiff']


def render(name, variant):
    seed = 2000 + SEED_ORDER.index(name) * 10 + sorted(CANDIDATES[name]).index(variant)
    stereo = CANDIDATES[name][variant](np.random.default_rng(seed))
    if name in LOUDNESS:
        stereo = u.match_loudness(stereo, LOUDNESS[name])
    return trim_tail(stereo)


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
