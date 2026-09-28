"""プレイヤーの回避 (前転) とジャスト回避の効果音を合成し、Assets/Audio/Physics に SoundFile アセットとして入れる。

    python tools/art/avoid_sfx.py [--only NAME ...] [--audition DIR] [--preview PATH]

- AvoidRolling: 回避の開始時 (avoidRollingSound_)。何度も鳴るので控えめに: 地面を蹴る → 外套・革の擦れ →
  肩から転がる低い着地。
- JustAvoidRolling: 回避の出だしの窓で攻撃を受け流した瞬間 (justAvoidRollingSound_)。左から右へかすめる
  風切りに、ゲームらしい「シャキーン」(刃の擦れ + 少し音程が上がる金属の響き + きらめきの粒) を重ねる。

回避は ui_sfx.py と同じ方針 (低い物理音・モーダル合成・A 特性で音量合わせ)。ジャスト回避だけは試聴で
「低く重い案」「明るい物理音の案」を退けて、はっきりゲームっぽい明るい音 (H) に決まった。
--audition は試聴用に全候補 (<Name>_<Variant>.mp3) を DIR に書き出し、何もインストールしない。
指定しなければ各音の選ばれた候補 (CHOSEN) をインストールする。既存の .mp3.meta は guid を保つ。
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import magic_sfx as m  # noqa: E402
import ui_sfx as u  # noqa: E402

m.OUT_DIR = m.REPO / 'Assets/Audio/Physics'

# NOTE: 空振り (ComboAttackWhiff_1) が -18 dB。回避は毎回鳴るので少し下げ、ジャスト回避は出来事なので上げる
LOUDNESS = {'AvoidRolling': -20.0, 'JustAvoidRolling': -16.0}


# ================================================================ 部品
def _kick_off(d, rng, t0=0.0, f=85):
    """土を蹴る踏み切り: 鈍い足音 + 土の粒"""
    thump = u.knock(d, rng, f, 0.05, 0.004, 0.8)
    grit = m.grains(d, rng, [t0 + 0.05 * rng.random() ** 2 for _ in range(18)], 300, 1800, (0.003, 0.012), (0.2, 0.8))
    out = np.zeros(m.n_of(d))
    m.at(out, m.norm(thump), t0)
    return out + m.norm(grit) * 0.35


def _cloth(d, rng, t0, dur, lo=180, hi=1100, rate=160):
    """外套と革の擦れ: 布が空気を切る「ばさっ」+ 革の粒"""
    swish = u.brush(dur, rng, lo, hi)
    grain = u.leather(dur, rng, lo, hi, rate)
    out = np.zeros(m.n_of(d))
    return m.at(out, m.norm(swish) * 0.8 + m.norm(grain) * 0.6, t0)


def _body_land(d, rng, t0, f=62, tau=0.12, grit=0.4):
    """肩・背中から地面に転がる重い着地 (胴の共振 + 土の擦れ)"""
    body = u.drum(d, rng, f, tau)
    scrape = m.bp(m.white(d, rng), 200, 1200) * m.curve(d, [(0, 0), (0.01, 1), (0.12, 0.3), (d, 0)])
    out = np.zeros(m.n_of(d))
    return m.at(out, m.norm(body) + m.norm(scrape) * grit, t0)


def _pass_by(d, rng, t0, dur, f_peak=900, lo=150, q=1.6, peak_at=0.2, muffle_hz=2200):
    """重い物が耳元をかすめる風切り。ピッチが上がって抜けていく (モノラル)"""
    t_peak = dur * peak_at
    fc = m.curve(dur, [(0, lo), (t_peak, f_peak), (dur, lo * 0.8)], 'exp')
    env = m.curve(dur, [(0, 0), (t_peak, 1), (dur, 0)]) ** 2.2
    body = m.lp(m.white(dur, rng), 450) * env
    out = np.zeros(m.n_of(d))
    # NOTE: 2 次の帯域通過だけだと高域が抜けてシャーッと明るくなるので、まとめて丸める
    swish = u.muffle(m.sweep(m.white(dur, rng), fc, q), muffle_hz)
    return m.at(out, m.norm(swish * env) + m.norm(body) * 0.5, t0)


def _pan(x, pan_curve):
    """pan_curve (-1 = 左, +1 = 右) で等パワーのパン。ステレオを返す"""
    theta = (np.clip(pan_curve, -1, 1) + 1) * np.pi / 4
    return np.stack([x * np.cos(theta), x * np.sin(theta)])


def _stereo_space(dry, rng, rt60, mix, bright=1800):
    """ステレオの乾いた音に左右別々の残響を付ける"""
    rl = m.reverb(dry[0], rng, rt60, mix, 0.02, bright)[0]
    rr = m.reverb(dry[1], rng, rt60, mix, 0.02, bright)[1]
    return np.stack([rl, rr])


def _mono_space(x, rng):
    return m.reverb(u.muffle(x, 3000), rng, rt60=0.45, mix=0.18, predelay=0.012, bright=2200)


# ================================================================ AvoidRolling (回避ステート 0.4s の出だしで鳴る)
def roll_a(rng):
    """標準: 踏み切り → 外套の擦れ → 肩から着地して一回転"""
    d = 0.55
    mix = (_kick_off(d, rng) * 0.7 + _cloth(d, rng, 0.03, 0.3) * 0.8
           + _body_land(d, rng, 0.17, 64, 0.1) * 0.9 + _body_land(d, rng, 0.31, 80, 0.06, 0.6) * 0.45)
    return _mono_space(m.sat(mix, 1.3), rng)


def roll_b(rng):
    """重め: 鎧の重みで地面をドスンと転がる (布は控えめ)"""
    d = 0.6
    mix = (_kick_off(d, rng, 0.0, 75) * 0.8 + _cloth(d, rng, 0.04, 0.25, 150, 800) * 0.5
           + _body_land(d, rng, 0.15, 52, 0.16, 0.5) * 1.0 + _body_land(d, rng, 0.3, 66, 0.08, 0.7) * 0.55)
    return _mono_space(m.sat(mix, 1.5), rng)


def roll_c(rng):
    """軽快: 外套が大きくはためく + 柔らかい土の上を転がる"""
    d = 0.55
    mix = (_kick_off(d, rng, 0.0, 95) * 0.5 + _cloth(d, rng, 0.0, 0.38, 220, 1400, 220) * 1.0
           + _body_land(d, rng, 0.2, 70, 0.07, 0.8) * 0.6)
    return _mono_space(mix, rng)


# ================================================================ JustAvoidRolling (攻撃を受け流した瞬間)
# NOTE: 被弾の判定が出た時点で鳴るので、頂点は出だし 0.06s 前後に置いて手応えを遅らせない
def _pass_by_stereo(d, rng, t0=0.0, dur=0.34, f_peak=900, muffle_hz=2200, lo=150):
    x = _pass_by(d, rng, t0, dur, f_peak, lo, muffle_hz=muffle_hz)
    return _pan(x, m.curve(d, [(0, -0.8), (t0 + dur * 0.2, 0.0), (t0 + dur, 0.8), (d, 0.8)]))


def just_a(rng):
    """かすめる風切り + 低い鉄の響き (刃が鎧の縁を掠めた感じ)"""
    d = 0.9
    whoosh = _pass_by_stereo(d, rng)
    ring = np.zeros(m.n_of(d))
    m.at(ring, m.norm(u.iron(0.8, rng, 170, 0.28, 0.002)), 0.05)
    dry = m.norm(whoosh) + _pan(ring, np.full(m.n_of(d), 0.0)) * 0.55
    return _stereo_space(dry, rng, 0.9, 0.22)


def just_b(rng):
    """かすめる風切り + 低い太鼓の「ドン」 (時間が止まったような重い手応え)"""
    d = 1.0
    whoosh = _pass_by_stereo(d, rng, 0.0, 0.34, 750)
    boom = np.zeros(m.n_of(d))
    m.at(boom, m.norm(u.drum(0.9, rng, 48, 0.35)), 0.05)
    dry = m.norm(whoosh) * 0.9 + _pan(boom, np.zeros(m.n_of(d))) * 0.8
    return _stereo_space(m.sat(dry, 1.3), rng, 1.1, 0.25, 1500)


def just_c(rng):
    """かすめる風切り + 刃が擦れる短いザッ + 低い鉄の響き + 小さな太鼓"""
    d = 0.95
    whoosh = _pass_by_stereo(d, rng, 0.0, 0.32, 1000)
    scrape = np.zeros(m.n_of(d))
    m.at(scrape, m.bp(m.white(0.12, rng), 500, 2400) * m.curve(0.12, [(0, 0), (0.008, 1), (0.12, 0)]) ** 1.5, 0.05)
    ring = np.zeros(m.n_of(d))
    m.at(ring, m.norm(u.iron(0.8, rng, 150, 0.3, 0.002)), 0.06)
    boom = np.zeros(m.n_of(d))
    m.at(boom, m.norm(u.drum(0.8, rng, 55, 0.22)), 0.06)
    center = m.norm(scrape) * 0.45 + m.norm(ring) * 0.5 + m.norm(boom) * 0.55
    dry = m.norm(whoosh) * 0.9 + _pan(center, np.full(m.n_of(d), 0.1))
    return _stereo_space(m.sat(dry, 1.3), rng, 0.9, 0.22)



# ---------------------------------------------------------------- 明るめ (試聴で「もっと明るく」と言われた後の候補)
def _blade_ring(d, rng, f=1450, tau=0.35):
    """刃がかすめて鳴る短い金属の響き: 非整数倍の部分音 (音階にはしない)"""
    modes = [(f, tau, 1.0), (f * 1.53, tau * 0.8, 0.7), (f * 2.21, tau * 0.55, 0.45), (f * 2.87, tau * 0.35, 0.25)]
    return m.lp(u.resonate(u.excite(d, rng, 0.0006, 9000), modes), 7000)


def _glint(d, rng, t0, lo=2000, hi=7000):
    """刃先が擦れる短い「シャッ」"""
    x = m.bp(m.white(0.1, rng), lo, hi) * m.curve(0.1, [(0, 0), (0.004, 1), (0.1, 0)]) ** 2
    return m.at(np.zeros(m.n_of(d)), x, t0)


def just_d(rng):
    """明るい風切り + 刃先のシャッ + 金属の響き"""
    d = 0.9
    whoosh = _pass_by_stereo(d, rng, 0.0, 0.3, 2600, 6500, 300)
    ring = m.at(np.zeros(m.n_of(d)), m.norm(_blade_ring(0.8, rng)), 0.05)
    center = m.norm(_glint(d, rng, 0.05)) * 0.5 + m.norm(ring) * 0.55
    dry = m.norm(whoosh) * 0.9 + _pan(center, np.full(m.n_of(d), 0.15))
    return _stereo_space(dry, rng, 0.8, 0.22, 5000)


def just_e(rng):
    """明るい風切り + 澄んだ長めの金属の響き (キィン)"""
    d = 1.1
    whoosh = _pass_by_stereo(d, rng, 0.0, 0.3, 2200, 5500, 250)
    ring = m.at(np.zeros(m.n_of(d)), m.norm(_blade_ring(1.0, rng, 1200, 0.6)), 0.05)
    dry = m.norm(whoosh) * 0.8 + _pan(m.norm(ring) * 0.75, np.full(m.n_of(d), 0.1))
    return _stereo_space(dry, rng, 1.1, 0.25, 6000)


def just_f(rng):
    """D に低い太鼓を足して重さを残す"""
    d = 0.95
    whoosh = _pass_by_stereo(d, rng, 0.0, 0.3, 2600, 6500, 300)
    ring = m.at(np.zeros(m.n_of(d)), m.norm(_blade_ring(0.8, rng, 1600, 0.3)), 0.05)
    boom = m.at(np.zeros(m.n_of(d)), m.norm(u.drum(0.8, rng, 55, 0.2)), 0.05)
    center = m.norm(_glint(d, rng, 0.05)) * 0.45 + m.norm(ring) * 0.5 + m.norm(boom) * 0.6
    dry = m.norm(whoosh) * 0.9 + _pan(center, np.full(m.n_of(d), 0.15))
    return _stereo_space(m.sat(dry, 1.2), rng, 0.9, 0.22, 5000)



# ---------------------------------------------------------------- ゲームらしい「シャキーン」 (試聴で決まった方向)
def _sha(d, rng, t0, dur=0.07):
    """「シャ」: 刃を抜くような明るい擦れ。帯域を上へ掃く"""
    fc = m.curve(dur, [(0, 2500), (dur, 9000)], 'exp')
    x = m.sweep(m.white(dur, rng), fc, 1.2) * m.curve(dur, [(0, 0), (0.006, 1), (dur, 0)]) ** 1.5
    return m.at(np.zeros(m.n_of(d)), x, t0)


def _kiin(d, rng, t0, f=2350, tau=0.7, glide=0.0):
    """「キーン」: 長く澄んだ金属の響き。部分音をわずかにずらしてうなりを付ける。glide > 0 で少しずつ上がる"""
    n = m.n_of(d - t0)
    ratios = [(1.0, 1.0, 1.0), (1.004, 1.0, 0.6), (1.51, 0.75, 0.55), (2.08, 0.55, 0.35), (2.76, 0.4, 0.2)]
    bend = m.curve(d - t0, [(0, 1.0), (d - t0, 1.0 + glide)], 'exp')
    ring = np.zeros(n)
    for r, tau_r, g in ratios:
        ring += m.osc(f * r * bend, d - t0) * m.exp_decay(d - t0, tau * tau_r, 0.001) * g
    return m.at(np.zeros(m.n_of(d)), ring, t0)


def _sparkle(d, rng, t0, count=14, span=0.35):
    """きらめきの細かい粒 (高い短い音をばらまく)"""
    return m.pings(d, rng, count, 3500, 8000, 0.03, t0, t0 + span, (0.2, 0.7))


def just_g(rng):
    """シャキーン: 刃の擦れ + 長い金属の響き"""
    d = 1.3
    center = m.norm(_sha(d, rng, 0.0)) * 0.7 + m.norm(_kiin(d, rng, 0.03)) * 0.8
    whoosh = _pass_by_stereo(d, rng, 0.0, 0.25, 3000, 8000, 400)
    dry = _pan(center, np.zeros(m.n_of(d))) + m.norm(whoosh) * 0.35
    return _stereo_space(dry, rng, 1.3, 0.3, 9000)


def just_h(rng):
    """シャキーン (派手): 音程が少し上がる響き + きらめきの粒"""
    d = 1.4
    center = (m.norm(_sha(d, rng, 0.0)) * 0.7 + m.norm(_kiin(d, rng, 0.03, 2100, 0.75, 0.06)) * 0.8
              + m.norm(_sparkle(d, rng, 0.04)) * 0.3)
    whoosh = _pass_by_stereo(d, rng, 0.0, 0.25, 3000, 8000, 400)
    dry = _pan(center, np.zeros(m.n_of(d))) + m.norm(whoosh) * 0.35
    return _stereo_space(dry, rng, 1.5, 0.35, 10000)


def just_i(rng):
    """シャキーン + 低い「ドゥン」: 明るさの下に重さを敷く"""
    d = 1.3
    boom = m.at(np.zeros(m.n_of(d)), m.norm(u.drum(1.0, rng, 50, 0.3)), 0.02)
    center = (m.norm(_sha(d, rng, 0.0)) * 0.65 + m.norm(_kiin(d, rng, 0.03, 2500, 0.65)) * 0.75
              + m.norm(boom) * 0.7)
    whoosh = _pass_by_stereo(d, rng, 0.0, 0.25, 3000, 8000, 400)
    dry = _pan(center, np.zeros(m.n_of(d))) + m.norm(whoosh) * 0.35
    return _stereo_space(dry, rng, 1.3, 0.3, 9000)


CANDIDATES = {
    'AvoidRolling': {'A': roll_a, 'B': roll_b, 'C': roll_c},
    'JustAvoidRolling': {'A': just_a, 'B': just_b, 'C': just_c, 'D': just_d, 'E': just_e, 'F': just_f,
                         'G': just_g, 'H': just_h, 'I': just_i},
}
CHOSEN = {'AvoidRolling': 'C', 'JustAvoidRolling': 'H'}


def render(name, variant):
    seed = 3000 + sorted(CANDIDATES).index(name) * 10 + sorted(CANDIDATES[name]).index(variant)
    stereo = CANDIDATES[name][variant](np.random.default_rng(seed))
    stereo = m.finish(stereo, -1, fade_out=0.08)
    return m.trim_tail(u.match_loudness(stereo, LOUDNESS[name]))


def centroid(stereo):
    x = stereo.mean(axis=0)
    spec = np.abs(np.fft.rfft(x))
    f = np.fft.rfftfreq(len(x), 1 / m.SR)
    return float((spec * f).sum() / (spec.sum() + 1e-12))


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
            print(f'{label:22s} {stereo.shape[1] / m.SR:5.2f}s  A={u.a_weighted_level(stereo):6.1f} dB  '
                  f'centroid={centroid(stereo):5.0f} Hz  {path.stat().st_size // 1024:3d} KB')
    if args.preview:
        m.render_preview(rendered, args.preview)


if __name__ == '__main__':
    main()
