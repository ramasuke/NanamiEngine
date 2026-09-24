"""共通の UI 効果音セット (GamePlay::Sound::UiSoundBank で鳴らす) を合成し、Assets/Audio/UI に SoundFile アセットとして
入れる。セットは Assets/Data/UiSound/UiSoundBank.uiSoundBank (UiSoundBankData) に登録され、各 UI コンポーネントは
`uiSounds_` フィールドからそれを参照する。

    python tools/art/ui_sfx.py [--only NAME ...] [--preview out.png]

Design (docs/UIDesign.md の2系統に合わせる。写実寄りの世界なので「ゲームっぽい電子音」「軽いクリック音」にしない):

- 系統 A「手で触れる物」(掲示板・店・手帳・貼り紙・石版): 厚い羊皮紙・木の卓・鉄・石が酒場で鳴る物音。
- 系統 B「HUD」(アイテム袋・魔法陣・ロックオン・大砲・チュートリアル札): 革袋・革のきしみ・鉄の歯止め・風のうなり。
  音階の付いたガラス音・鈴・ピコピコは使わない。出来事 (開始・ボス) だけ遠くの鐘や大太鼓にする。
- 重さ: 正弦波を直接鳴らさず、柔らかい打撃で共振器を叩くモーダル合成 (knock / iron / stone / drum) にし、
  胴鳴りと長めの減衰、部屋の初期反射 (tavern) を付けて「その場で物が鳴った」音にする。
- 何度も鳴る所には鳴らさない: 会話の文字送り・NPC に近づいたとき・ロックオン解除は無音。
  チュートリアル札は出てくるときだけ鳴らし、課題の達成は羽根ペンの控えめな音にする。
- 高さ: 基音は 45〜500 Hz、高域は 1.6〜3.2 kHz で丸める (スペクトル重心がおおむね 1.5 kHz 以下)。
  紙の擦れも 250〜2500 Hz の帯域だけを使い、シャリシャリさせない。
- 音量は波形のピークではなく聞こえ方 (A 特性・50 ms 窓の最大 RMS) で揃える。低い音はピークで揃えると
  ほとんど聞こえなくなるため。基準は店の購入音 Shop_Purchase (-14 dB)。LOUDNESS の段:
  出来事 (判子・開始・ボス) -14 dB、操作の返事 (決定・戻る・頁・開閉) -17 dB、
  何度も鳴るもの (カーソル・袋の切替) -21 dB、戦闘・会話・チュートリアル中のもの -19〜-23 dB。ピークは -1 dBFS を超えないよう柔らかく抑える。

プリミティブ・mp3 エンコーダ・.meta テンプレートは tools/art/magic_sfx.py のもの (MPEG-1 Layer III,
192 kbps, 48 kHz, ステレオ。volume_ 255)。write_sound() は既存の .meta を残すので、再実行しても GUID は変わらない。
"""
from __future__ import annotations

import argparse
import sys
import uuid
from pathlib import Path

import numpy as np
from scipy.signal import lfilter

sys.path.insert(0, str(Path(__file__).resolve().parent))
import magic_sfx as m  # noqa: E402

OUT_DIR = m.REPO / 'Assets/Audio/UI'

# 低い青銅の鐘の基音 (D3 / D4)
D3, D4 = 146.83, 293.66


# ================================================================ 部品
# NOTE: 正弦波をそのまま鳴らすと電子音に聞こえるので、音程のある成分は「柔らかい打撃 (雑音の短い塊) で共振器を
#       叩く」モーダル合成で作る。共振の Q は減衰時間から決める (tau = Q / (pi f))。
def excite(d, rng, soft=0.003, lp_hz=2500):
    """指の腹・拳・木槌で叩いたときの励起。soft が長いほど柔らかい (高域が出ない)"""
    n = max(8, m.n_of(soft))
    burst = rng.standard_normal(n) * np.hanning(n)
    return m.lp(m.pad(burst, m.n_of(d)), lp_hz)


def resonate(exc, modes):
    """modes = [(周波数, 減衰秒, 音量), ...] の共振器を並べて exc で鳴らす"""
    out = np.zeros_like(exc)
    for f, tau, g in modes:
        b, a = m._biquad('bp', f, max(0.7, np.pi * f * tau))
        out += lfilter(b, a, exc) * g
    return out


def knock(d, rng, f=120, tau=0.1, soft=0.003, body=0.5):
    """厚い木の台を拳や指で叩く音: 板の非整数倍の共振 + 台の胴鳴り"""
    modes = [(f, tau, 1.0), (f * 2.3, tau * 0.6, 0.55), (f * 3.8, tau * 0.35, 0.3), (f * 5.6, tau * 0.2, 0.15)]
    x = m.norm(resonate(excite(d, rng, soft), modes))
    thump = m.norm(resonate(excite(d, rng, soft * 2, 400), [(f * 0.55, tau * 0.7, 1.0)]))
    return x + body * thump


def iron(d, rng, f=260, tau=0.06, soft=0.0012):
    """鉄の金具・留め具: 重く短い非整数倍音 (高域は丸める)"""
    modes = [(f, tau, 1.0), (f * 2.14, tau * 0.7, 0.6), (f * 3.47, tau * 0.5, 0.35), (f * 5.2, tau * 0.3, 0.2)]
    return m.lp(resonate(excite(d, rng, soft, 3500), modes), 3200)


def bronze_bell(d, rng, f=D3, tau=1.4):
    """遠くの青銅の鐘: 教会鐘の部分音 (hum / prime / 短三度 / 五度 / オクターブ)"""
    modes = [(f * 0.5, tau * 1.6, 0.6), (f, tau, 1.0), (f * 1.19, tau * 0.7, 0.45), (f * 1.5, tau * 0.5, 0.3),
             (f * 2.0, tau * 0.4, 0.3), (f * 2.61, tau * 0.25, 0.12)]
    return m.lp(resonate(excite(d, rng, 0.004, 2000), modes), 1800)


def drum(d, rng, f=60, tau=0.3):
    """皮を張った低い太鼓: 皮の共振 + 胴の空気 + 皮の擦れ"""
    modes = [(f, tau, 1.0), (f * 1.59, tau * 0.6, 0.5), (f * 2.14, tau * 0.4, 0.3)]
    x = m.norm(resonate(excite(d, rng, 0.006, 600), modes))
    skin = m.lp(m.white(d, rng), 700) * m.exp_decay(d, 0.03, 0.002)
    return x + 0.25 * m.norm(skin)


def parchment(d, rng, rate=90, lo=250, hi=1500):
    """厚い羊皮紙・帳面が擦れる音: ゆっくりした粗い粒 + 空気が動く低い「ふわっ」"""
    times = m.poisson_times(d, rng, lambda t: rate)
    crinkle = m.grains(d, rng, times, lo, hi, (0.006, 0.025), (0.2, 1.0))
    whump = m.lp(m.white(d, rng), 260)
    env = m.curve(d, [(0, 0), (d * 0.3, 1), (d, 0)])
    return (m.norm(crinkle) * 0.8 + m.norm(whump) * 0.5) * env


def brush(d, rng, lo=300, hi=1400):
    """指先で紙の表面をなでる/紙を滑らせる音"""
    fc = m.curve(d, [(0, lo), (d * 0.4, hi), (d, lo)], 'exp')
    x = m.sweep(m.white(d, rng), fc, q=0.9)
    return x * m.curve(d, [(0, 0), (d * 0.35, 1), (d, 0)])


def leather(d, rng, lo=200, hi=1200, rate=140):
    """革袋・外套・籠手の擦れ"""
    times = m.poisson_times(d, rng, lambda t: rate)
    x = m.grains(d, rng, times, lo, hi, (0.006, 0.02), (0.3, 1.0))
    return x * m.curve(d, [(0, 0), (d * 0.3, 1), (d, 0)])


def stone(d, rng, f=110, tau=0.08, grit=0.5):
    """石の板が当たる鈍い音"""
    modes = [(f, tau, 1.0), (f * 2.7, tau * 0.5, 0.5), (f * 4.9, tau * 0.3, 0.25)]
    x = m.norm(resonate(excite(d, rng, 0.002, 2000), modes))
    return x + grit * m.norm(m.bp(m.white(d, rng), 300, 1500) * m.exp_decay(d, tau * 0.4, 0.001))


def muffle(x, f=2800):
    """高域をまとめて丸める (4 次のローパス)"""
    return m.lp(x, f, 4)


def tavern(x, rng, rt60=0.55, mix=0.2, bright=2200):
    """木造の酒場くらいの部屋: 初期反射 (壁・卓) + 残響。乾いた音を「その場で鳴った」音にする"""
    y = x.copy()
    for delay, gain in ((0.007, 0.35), (0.013, 0.25), (0.021, 0.18), (0.034, 0.12)):
        m.at(y, m.lp(x, 2500) * gain, delay)
    return m.reverb(y, rng, rt60=rt60, mix=mix, predelay=0.01, bright=bright)


def open_air(x, rng, rt60=1.2, mix=0.25, bright=1600):
    """屋外で遠くまで響く音 (鐘・太鼓・地鳴り)"""
    return m.reverb(x, rng, rt60=rt60, mix=mix, predelay=0.03, bright=bright)


def mix_at(d, parts):
    """parts = [(信号, 開始秒, 音量)] を 1 本にまとめる (各信号は正規化してから重ねる)"""
    buf = np.zeros(m.n_of(d))
    for x, start, gain in parts:
        m.at(buf, m.norm(x), start, gain)
    return buf


# ================================================================ 系統 A
def ui_cursor(rng):
    """カーソル移動・ホバー: 指先で厚い紙をなで、下の卓にかすかに触れる"""
    d = 0.16
    x = mix_at(d, [(brush(0.1, rng, 300, 1200), 0.0, 0.7), (knock(0.12, rng, 170, 0.04, 0.004, 0.2), 0.03, 0.5)])
    return m.finish(tavern(muffle(x, 2000), rng, 0.4, 0.14), -20.0, fade_out=0.03)


def ui_confirm(rng):
    """決定: 拳の腹で厚い卓を「ゴン」と 1 回叩く"""
    d = 0.35
    x = knock(d, rng, 118, 0.12, 0.003, 0.6)
    return m.finish(tavern(muffle(x, 2600), rng), -10.0)


def ui_cancel(rng):
    """戻る: 紙を手前へ引き戻し、指で軽く押さえる"""
    d = 0.35
    x = mix_at(d, [(brush(0.2, rng, 700, 250), 0.0, 0.7), (knock(0.2, rng, 100, 0.07, 0.005, 0.4), 0.13, 0.6)])
    return m.finish(tavern(muffle(x, 2000), rng), -13.0)


def ui_open(rng):
    """画面を開く: 厚い羊皮紙を広げ、卓に置く「ばさり…トン」"""
    d = 0.6
    x = mix_at(d, [(parchment(0.35, rng, 90), 0.0, 0.75), (knock(0.3, rng, 92, 0.1, 0.006, 0.7), 0.3, 0.7)])
    return m.finish(tavern(muffle(x, 2400), rng), -11.0)


def ui_close(rng):
    """画面を閉じる: 羊皮紙を畳んで脇へ置く"""
    d = 0.45
    x = mix_at(d, [(parchment(0.22, rng, 80, 250, 1300), 0.0, 0.7), (knock(0.25, rng, 105, 0.08, 0.006, 0.6), 0.18, 0.55)])
    return m.finish(tavern(muffle(x, 2200), rng), -14.0)


def ui_tab(rng):
    """タブ・頁の切り替え: 厚い帳面を 1 枚めくって置く"""
    d = 0.42
    flip = brush(0.3, rng, 250, 1300) + 0.4 * m.lp(m.white(0.3, rng), 250) * m.curve(0.3, [(0, 0), (0.15, 1), (0.3, 0)])
    x = mix_at(d, [(flip, 0.0, 0.8), (parchment(0.08, rng, 120, 300, 1200), 0.24, 0.35)])
    return m.finish(tavern(muffle(x, 2200), rng, 0.45, 0.16), -14.0)


def ui_stamp(rng):
    """判子を押す: 重い木の判子を紙越しに卓へ「ドン」と押し付け、少しねじって離す"""
    d = 0.6
    body = knock(0.45, rng, 72, 0.14, 0.004, 0.9)
    crush = m.lp(m.white(0.1, rng), 700) * m.exp_decay(0.1, 0.02, 0.001)
    x = mix_at(d, [(body, 0.0, 1.0), (crush, 0.0, 0.45), (brush(0.1, rng, 300, 900), 0.28, 0.2)])
    return m.finish(tavern(x, rng, 0.6, 0.22, 1800), -5.0)


def ui_refuse(rng):
    """断り: 帳場の台を指の関節で 2 回、低く叩く"""
    d = 0.4
    x = mix_at(d, [(knock(0.18, rng, 98, 0.06, 0.003, 0.5), 0.0, 1.0), (knock(0.18, rng, 96, 0.06, 0.003, 0.5), 0.12, 0.75)])
    return m.finish(tavern(muffle(x, 1800), rng), -11.0)


def ui_digit(rng):
    """ルームコードの数字: 鉄のダイヤル錠が 1 目盛り重く回る「ゴリッ」"""
    d = 0.14
    x = mix_at(d, [(iron(0.12, rng, 300, 0.03), 0.0, 0.8), (m.bp(m.white(0.03, rng), 300, 1500), 0.0, 0.3)])
    return m.finish(tavern(muffle(x, 2600), rng, 0.3, 0.1), -19.0, fade_out=0.02)


def ui_game_start(rng):
    """タイトルで開始: 重い扉の鉄の閂を外し、扉が開く。遠くで青銅の鐘が 1 つ"""
    d = 3.4
    latch = iron(0.4, rng, 150, 0.12, 0.002)
    door = knock(0.6, rng, 62, 0.2, 0.008, 1.0)
    creak = m.sweep(m.white(0.6, rng), m.curve(0.6, [(0, 180), (0.6, 320)], 'exp'), q=6.0) * m.curve(0.6, [(0, 0), (0.2, 1), (0.6, 0)])
    near = tavern(mix_at(1.0, [(latch, 0.0, 0.8), (door, 0.12, 0.9), (creak, 0.18, 0.35)]), rng, 0.8, 0.25, 1800)
    bell = open_air(bronze_bell(3.0, rng, D3, 1.2), rng, 1.8, 0.35, 1400)
    out = np.zeros((2, m.n_of(d) + near.shape[1]))
    out[:, :near.shape[1]] += near / (np.max(np.abs(near)) + 1e-9)
    b = bell[:, : out.shape[1] - m.n_of(0.45)]
    out[:, m.n_of(0.45):m.n_of(0.45) + b.shape[1]] += 0.55 * b / (np.max(np.abs(bell)) + 1e-9)
    return m.finish(out, -5.0, fade_out=0.6)


def ui_stone_cursor(rng):
    """ゲームオーバーの石版上のカーソル: 小石が石に触れる低い「コツ」"""
    d = 0.14
    x = stone(d, rng, 240, 0.03, 0.3)
    return m.finish(open_air(muffle(x, 1800), rng, 0.8, 0.14, 1600), -18.0, fade_out=0.02)


def ui_stone_confirm(rng):
    """ゲームオーバーの決定: 石版を押し込む重い擦れ + 低い「ゴン」"""
    d = 1.0
    grind = m.bp(m.brown(0.3, rng) + 0.3 * m.white(0.3, rng), 100, 800) * m.curve(0.3, [(0, 0), (0.06, 1), (0.3, 0)])
    x = mix_at(d, [(grind, 0.0, 0.5), (stone(0.7, rng, 62, 0.2, 0.4), 0.18, 1.0)])
    return m.finish(open_air(x, rng, 1.1, 0.22, 1400), -7.0, fade_out=0.15)


def ui_hoof_tick(rng):
    """アセット更新の進み: 早馬の蹄が土を踏む「ドッ」"""
    d = 0.16
    x = knock(d, rng, 150, 0.03, 0.004, 0.6) + 0.5 * m.norm(m.lp(m.white(d, rng), 600) * m.exp_decay(d, 0.02, 0.001))
    return m.finish(open_air(muffle(x, 1500), rng, 0.5, 0.1, 1500), -19.0, fade_out=0.02)


def ui_loading_done(rng):
    """ロード完了: 木箱の蓋を閉め、鉄の留め金を掛ける「ドッ・カチャ」"""
    d = 0.6
    x = mix_at(d, [(knock(0.35, rng, 82, 0.12, 0.006, 0.8), 0.0, 1.0), (iron(0.2, rng, 330, 0.04), 0.16, 0.45)])
    return m.finish(tavern(muffle(x, 2400), rng), -12.0, fade_out=0.1)


# ================================================================ 系統 B (HUD)
def hud_select(rng):
    """アイテム袋の切り替え: 腰の革袋の中で瓶と道具が鈍く当たる"""
    d = 0.2
    x = mix_at(d, [(leather(0.14, rng, 200, 1100, 160), 0.0, 0.6), (knock(0.12, rng, 210, 0.035, 0.003, 0.2), 0.03, 0.6)])
    return m.finish(tavern(muffle(x, 2000), rng, 0.3, 0.08), -20.0, fade_out=0.03)


def hud_palette_open(rng):
    """魔法陣パレットを開く: 手元に風が巻いて集まる低いうなり"""
    d = 0.7
    swirl = m.sweep(m.white(d, rng), m.curve(d, [(0, 150), (0.35, 600), (d, 280)], 'exp'), q=1.6)
    swirl *= m.curve(d, [(0, 0), (0.28, 1), (d, 0)])
    rumble = m.lp(m.brown(d, rng), 160) * m.curve(d, [(0, 0), (0.25, 1), (d, 0)])
    x = m.norm(swirl) + 0.5 * m.norm(rumble)
    return m.finish(open_air(muffle(x, 1600), rng, 0.8, 0.2, 1400), -15.0, fade_out=0.1)


def hud_page_shift(rng):
    """魔法陣パレットの頁送り: 陣が回る短く低い風切り"""
    d = 0.3
    swish = m.sweep(m.white(d, rng), m.curve(d, [(0, 250), (0.12, 750), (d, 220)], 'exp'), q=1.5)
    swish *= m.curve(d, [(0, 0), (0.1, 1), (d, 0)])
    return m.finish(open_air(muffle(swish, 1400), rng, 0.5, 0.15, 1400), -17.0, fade_out=0.05)


def hud_lock_on(rng):
    """ロックオン: 柄を握り直す革のきしみ (金属音は鳴らさず、控えめに)"""
    d = 0.2
    creak = m.sweep(m.white(0.12, rng), 420, q=5.0) * m.curve(0.12, [(0, 0), (0.04, 1), (0.12, 0)])
    x = mix_at(d, [(leather(0.12, rng, 200, 1000, 180), 0.0, 0.6), (creak, 0.02, 0.5)])
    return m.finish(tavern(muffle(x, 1600), rng, 0.25, 0.06), -20.0, fade_out=0.04)


def hud_lock_off(rng):
    """(今は鳴らしていない) ロックオン解除: 構えを解く布擦れ"""
    x = leather(0.16, rng, 200, 1000, 140)
    return m.finish(tavern(muffle(x, 1600), rng, 0.25, 0.06), -20.0, fade_out=0.04)


def hud_ready(rng):
    """大砲の装填完了: 鉄の歯止めが 2 段「ガチッ、ガチャン」と噛み合い、砲架の木が鳴る"""
    d = 0.6
    x = mix_at(d, [(iron(0.2, rng, 190, 0.05), 0.0, 0.6), (iron(0.35, rng, 150, 0.09), 0.12, 1.0),
                   (knock(0.35, rng, 70, 0.12, 0.006, 0.8), 0.12, 0.7)])
    return m.finish(open_air(muffle(x, 2600), rng, 0.7, 0.16, 1800), -11.0, fade_out=0.08)


def hud_notice(rng):
    """チュートリアル札が出る: 札を板に差し込む紙の擦れ + 軽い木の音"""
    d = 0.4
    x = mix_at(d, [(brush(0.18, rng, 250, 1100), 0.0, 0.6), (knock(0.2, rng, 140, 0.06, 0.004, 0.4), 0.14, 0.6)])
    return m.finish(tavern(muffle(x, 2000), rng, 0.4, 0.12), -15.0, fade_out=0.05)


def hud_clear(rng):
    """チュートリアルの課題を達成: 羽根ペンで札にさっと印を付ける (次々に鳴るので控えめに)"""
    d = 0.3
    stroke1 = m.bp(m.white(0.06, rng), 500, 2000) * m.curve(0.06, [(0, 0), (0.01, 1), (0.06, 0)])
    stroke2 = m.bp(m.white(0.1, rng), 450, 1800) * m.curve(0.1, [(0, 0), (0.015, 1), (0.1, 0)])
    x = mix_at(d, [(stroke1, 0.0, 0.7), (stroke2, 0.07, 1.0), (knock(0.1, rng, 180, 0.03, 0.004, 0.2), 0.0, 0.25)])
    return m.finish(tavern(muffle(x, 2200), rng, 0.3, 0.08), -18.0, fade_out=0.04)


def hud_boss_appear(rng):
    """ボスのゲージが出る: 地の底から膨らむ地鳴り + 遠くの大太鼓 2 打"""
    d = 2.8
    rumble = m.lp(m.brown(d, rng), 140) * m.curve(d, [(0, 0), (1.2, 1), (1.9, 0.7), (d, 0)])
    growl = m.sweep(m.brown(d, rng), m.curve(d, [(0, 60), (1.3, 110), (d, 70)], 'exp'), q=3.0) * m.curve(d, [(0, 0), (1.3, 1), (d, 0)])
    x = mix_at(d, [(rumble, 0.0, 0.7), (growl, 0.0, 0.45), (drum(1.2, rng, 52, 0.4), 0.0, 1.0), (drum(1.2, rng, 47, 0.45), 0.6, 0.85)])
    return m.finish(open_air(x, rng, 1.8, 0.28, 1000), -3.0, fade_out=0.5)


def hud_interact(rng):
    """(今は鳴らしていない) 話しかけられる印が出る: 外套が擦れる気配"""
    x = leather(0.16, rng, 200, 1000, 120)
    return m.finish(tavern(muffle(x, 1600), rng, 0.25, 0.06), -20.0, fade_out=0.04)


# ================================================================ 会話
def chat_open(rng):
    """会話が始まる: 手紙を開くような紙の擦れ (話し声の邪魔をしないよう小さく)"""
    d = 0.3
    x = parchment(0.22, rng, 70, 250, 1200)
    return m.finish(tavern(muffle(x, 1800), rng, 0.35, 0.1), -17.0, fade_out=0.05)


def chat_blip(rng):
    """(今は鳴らしていない) 文字送り: 羽根ペンが紙を擦る極小の音"""
    d = 0.06
    x = m.bp(m.white(d, rng), 500, 1600) * m.curve(d, [(0, 0), (0.01, 1), (d, 0)])
    return m.finish(muffle(x, 1800), -24.0, fade_in=0.001, fade_out=0.012)


# 聞こえ方の目標 (dB, A 特性)。ここに無い音は LOUD_RESPONSE
LOUD_EVENT, LOUD_RESPONSE, LOUD_FREQUENT = -14.0, -17.0, -21.0
LOUDNESS = {
    'Ui_Stamp': LOUD_EVENT,
    'Ui_GameStart': LOUD_EVENT,
    'Ui_StoneConfirm': LOUD_EVENT,
    'Hud_BossAppear': LOUD_EVENT,
    'Ui_Cursor': LOUD_FREQUENT,
    'Ui_Digit': LOUD_FREQUENT,
    'Ui_StoneCursor': LOUD_FREQUENT,
    'Ui_HoofTick': LOUD_FREQUENT,
    'Hud_Select': LOUD_FREQUENT,
    'Hud_LockOff': LOUD_FREQUENT,
    'Hud_Interact': LOUD_FREQUENT,
    'Chat_Blip': LOUD_FREQUENT,
    # 戦闘中や会話・チュートリアルの間に何度も鳴るので、さらに一段控える
    'Hud_LockOn': -23.0,
    'Hud_Clear': -22.0,
    'Chat_Open': -21.0,
    'Hud_Notice': -19.0,
}
PEAK_CEILING = 10 ** (-1.0 / 20)


def a_weighted_level(stereo):
    """A 特性をかけた 50 ms 窓 RMS の最大値 (dBFS)"""
    x = stereo.mean(axis=0)
    f = np.fft.rfftfreq(len(x), 1 / m.SR)
    f2 = f * f
    ra = (12194.0 ** 2 * f2 ** 2) / ((f2 + 20.6 ** 2) * np.sqrt((f2 + 107.7 ** 2) * (f2 + 737.9 ** 2)) * (f2 + 12194.0 ** 2))
    y = np.fft.irfft(np.fft.rfft(x) * ra * 10 ** (2.0 / 20), len(x))
    win = min(len(y), m.n_of(0.05))
    rms = np.sqrt(np.convolve(y * y, np.ones(win) / win, 'valid')).max()
    return 20 * np.log10(rms + 1e-12)


def headroom_over(stereo, target_db):
    """target_db まで上げたとき、ピークが天井を何 dB 超えるか"""
    gain = target_db - a_weighted_level(stereo)
    return 20 * np.log10(np.max(np.abs(stereo)) / PEAK_CEILING) + gain


def match_loudness(stereo, target_db, max_squash_db=3.0):
    """聞こえ方を target_db に合わせる。ピークが天井を超える分は tanh で柔らかく潰す
    NOTE: 潰しすぎると歪むので、先に聞こえにくい超低音 (ピークを食うだけの成分) を必要な分だけ削る"""
    for cutoff in (50, 70, 90, 110, 140):
        if headroom_over(stereo, target_db) <= max_squash_db:
            break
        stereo = np.stack([m.hp(ch, cutoff, 2) for ch in stereo])
    # NOTE: それでも潰れすぎる長い音 (鐘・低いうなり) は目標の方を下げる。持続音は短い音より大きく聞こえる
    target_db -= max(0.0, headroom_over(stereo, target_db) - max_squash_db)
    for _ in range(4):
        stereo = stereo * 10 ** ((target_db - a_weighted_level(stereo)) / 20)
        peak = np.max(np.abs(stereo))
        if peak > PEAK_CEILING:
            stereo = PEAK_CEILING * np.tanh(stereo / PEAK_CEILING)
        if abs(a_weighted_level(stereo) - target_db) < 0.3:
            break
    return stereo


SOUNDS = {
    'Ui_Cursor': ui_cursor,
    'Ui_Confirm': ui_confirm,
    'Ui_Cancel': ui_cancel,
    'Ui_Open': ui_open,
    'Ui_Close': ui_close,
    'Ui_Tab': ui_tab,
    'Ui_Stamp': ui_stamp,
    'Ui_Refuse': ui_refuse,
    'Ui_Digit': ui_digit,
    'Ui_GameStart': ui_game_start,
    'Ui_StoneCursor': ui_stone_cursor,
    'Ui_StoneConfirm': ui_stone_confirm,
    'Ui_HoofTick': ui_hoof_tick,
    'Ui_LoadingDone': ui_loading_done,
    'Hud_Select': hud_select,
    'Hud_PaletteOpen': hud_palette_open,
    'Hud_PageShift': hud_page_shift,
    'Hud_LockOn': hud_lock_on,
    'Hud_LockOff': hud_lock_off,
    'Hud_Ready': hud_ready,
    'Hud_Notice': hud_notice,
    'Hud_Clear': hud_clear,
    'Hud_BossAppear': hud_boss_appear,
    'Hud_Interact': hud_interact,
    'Chat_Open': chat_open,
    'Chat_Blip': chat_blip,
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
    ap.add_argument('--only', nargs='*')
    ap.add_argument('--preview')
    args = ap.parse_args()
    rendered = {}
    for name in args.only or SOUNDS:
        rng = np.random.default_rng(3000 + list(SOUNDS).index(name))
        stereo = m.trim_tail(SOUNDS[name](rng))
        stereo = match_loudness(stereo, LOUDNESS.get(name, LOUD_RESPONSE))
        rendered[name] = stereo
        path = write_sound(name, stereo)
        print(f'{name:18s} {stereo.shape[1] / m.SR:5.2f}s  {path.stat().st_size // 1024:3d} KB  '
              f'A {a_weighted_level(stereo):6.1f} dB  peak {20 * np.log10(np.max(np.abs(stereo))):5.1f} dBFS')
    if args.preview:
        m.render_preview(rendered, args.preview)


if __name__ == '__main__':
    main()
