"""Synthesize the shared UI sound set (GamePlay::Sound::UiSoundBank) and install it as SoundFile assets under
Assets/Audio/UI.

    python tools/art/ui_sfx.py [--only NAME ...] [--preview out.png]

Design (docs/UIDesign.md の2系統に合わせる。写実寄りのゲームなので「ゲームっぽい電子音」にしない):

- 系統 A「手で触れる物」(掲示板・店・手帳・貼り紙・石版): 紙・木・鉄・石の物音。
- 系統 B「HUD」(アイテム袋・魔法陣・ロックオン・大砲・チュートリアル札): 革袋・布・鉄の留め具・低い空気のうなり。
  音階の付いたガラス音や鈴は使わない。目立たせたい出来事 (達成・開始) だけ低い青銅の鐘や太鼓にする。
- 高さ: 基音は 45〜500 Hz、高域は 1.6〜3.2 kHz で丸める (スペクトル重心がおおむね 1.5 kHz 以下)。
  紙の擦れも 250〜2500 Hz の帯域だけを使い、シャリシャリさせない。
- 音量の段: 何度も鳴るもの (カーソル・文字送り・袋の切替) は小さく短く (-24〜-18 dB / <150 ms)、
  操作の返事 (決定・戻る・頁) は中 (-17〜-10 dB)、出来事 (判子・開始・ボス・達成) は大きめ (-8〜-3 dB)。

The primitives, the mp3 encoder and the .meta template come from tools/art/magic_sfx.py (MPEG-1 Layer III,
192 kbps, 48 kHz, stereo; volume_ 255). write_sound() keeps an existing .meta, so re-running keeps the GUIDs.
"""
from __future__ import annotations

import argparse
import sys
import uuid
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import magic_sfx as m  # noqa: E402

OUT_DIR = m.REPO / 'Assets/Audio/UI'

# 低い青銅の鐘の基音 (D3 / D4)
D3, D4 = 146.83, 293.66


# ================================================================ building blocks
def paper(d, rng, lo=400, hi=2500, tau=0.02):
    """紙や布が擦れる短いノイズ (高域は入れない)"""
    return m.bp(m.white(d, rng), lo, hi) * m.exp_decay(d, tau, 0.0015)


def wood(f, d, tau=0.035, rng=None, click=0.35):
    """木を叩いた音: 低めの共鳴 2 本 + 叩いた瞬間の擦れ"""
    x = m.osc(f, d) * m.exp_decay(d, tau, 0.0008)
    x += 0.45 * m.osc(f * 2.31, d) * m.exp_decay(d, tau * 0.45, 0.0008)
    if rng is not None:
        x += click * m.bp(m.white(d, rng), f * 1.5, min(f * 7, 3000)) * m.exp_decay(d, tau * 0.25, 0.0005)
    return x


def iron(f, d, tau=0.05, rng=None):
    """鉄の金具・留め具が当たる音: 重く短く減衰する非整数倍音 + 当たりの擦れ"""
    partials = [(1.0, 1.0), (2.14, 0.5), (3.47, 0.25)]
    x = sum(m.osc(f * r, d) * g * m.exp_decay(d, tau / r, 0.0005) for r, g in partials)
    if rng is not None:
        x += 0.5 * m.bp(m.white(d, rng), 600, 2800) * m.exp_decay(d, 0.006, 0.0003)
    return x


def bronze_bell(f, d, tau=1.2):
    """低い青銅の鐘: hum (0.5) / prime / 短三度 / 五度 / オクターブの教会鐘の部分音"""
    partials = [(0.5, 0.55, 1.6), (1.0, 1.0, 1.0), (1.19, 0.45, 0.7), (1.5, 0.3, 0.5), (2.0, 0.35, 0.4), (2.61, 0.12, 0.25)]
    x = sum(m.osc(f * r, d) * g * m.exp_decay(d, tau * k, 0.002) for r, g, k in partials)
    return m.lp(x, 2200)


def drum(f, d, rng, tau=0.18):
    """皮を張った低い太鼓: 音程の落ちる胴鳴り + 皮の擦れ"""
    body = m.osc(m.curve(d, [(0, f * 1.6), (0.06, f)], 'exp'), d) * m.exp_decay(d, tau, 0.002)
    skin = m.lp(m.white(d, rng), 900) * m.exp_decay(d, 0.02, 0.001)
    return body + 0.35 * skin


def leather(d, rng, lo=250, hi=1600, rate=180):
    """革袋・外套の擦れ (ざらつきを細かい粒で出す)"""
    times = m.poisson_times(d, rng, lambda t: rate)
    x = m.grains(d, rng, times, lo, hi, (0.004, 0.015), (0.3, 1.0))
    return x * m.curve(d, [(0, 0), (d * 0.25, 1), (d, 0)])


def stone(d, rng, f=140, tau=0.06):
    """石の板が当たる鈍い音"""
    body = m.osc(m.curve(d, [(0, f * 1.3), (0.03, f)], 'exp'), d) * m.exp_decay(d, tau, 0.001)
    grit = m.bp(m.white(d, rng), 300, 1800) * m.exp_decay(d, tau * 0.5, 0.0008)
    return body + 0.55 * grit


def muffle(x, f=2800):
    """高域をまとめて丸める (4 次のローパス)"""
    return m.lp(x, f, 4)


def room(x, rng, rt60=0.28, mix=0.12, bright=5000):
    return m.reverb(x, rng, rt60=rt60, mix=mix, predelay=0.005, bright=bright)


def air(x, rng, rt60=0.6, mix=0.18, bright=2500):
    return m.reverb(x, rng, rt60=rt60, mix=mix, predelay=0.012, bright=bright)


# ================================================================ 系統 A
def ui_cursor(rng):
    """カーソル移動・ホバー: 指先で木の札に触れる小さく鈍い「トッ」"""
    d = 0.08
    x = wood(240, d, 0.014, rng, 0.25) + 0.3 * paper(d, rng, 500, 1800, 0.008)
    return m.finish(room(muffle(x, 2200), rng, 0.18, 0.06, 2500), -20.0, fade_out=0.02)


def ui_confirm(rng):
    """決定: 拳の腹で木の台を「ゴッ」と叩く (低い胴鳴り + 短い当たり)"""
    d = 0.3
    buf = np.zeros(m.n_of(d))
    m.at(buf, m.norm(wood(165, 0.25, 0.05, rng, 0.3)), 0.0, 1.0)
    m.at(buf, m.norm(m.osc(82, 0.2) * m.exp_decay(0.2, 0.05, 0.002)), 0.0, 0.45)
    return m.finish(room(muffle(buf, 2600), rng, 0.3, 0.12, 2500), -10.0)


def ui_cancel(rng):
    """戻る: 紙を手前へ引き戻す擦れ + こもった軽い叩き"""
    d = 0.26
    buf = np.zeros(m.n_of(d))
    slide = m.sweep(m.white(0.15, rng), m.curve(0.15, [(0, 1600), (0.15, 450)], 'exp'), q=1.0)
    slide *= m.curve(0.15, [(0, 0), (0.03, 1), (0.15, 0)])
    m.at(buf, m.norm(slide), 0.0, 0.5)
    m.at(buf, m.norm(wood(130, 0.14, 0.035, rng, 0.15)), 0.07, 0.8)
    return m.finish(room(muffle(buf, 2200), rng, 0.25, 0.1, 2200), -13.0)


def ui_open(rng):
    """画面を開く: 厚い羊皮紙を広げる「ばさっ」+ 木の台に置く重い音"""
    d = 0.55
    buf = np.zeros(m.n_of(d))
    rustle_d = 0.28
    times = m.poisson_times(rustle_d, rng, lambda t: 220 * (1 - t / rustle_d) + 40)
    rustle = m.grains(rustle_d, rng, times, 400, 2200, (0.004, 0.015), (0.2, 1.0))
    rustle += 0.6 * m.bp(m.white(rustle_d, rng), 250, 1200) * m.curve(rustle_d, [(0, 0), (0.06, 1), (rustle_d, 0)])
    m.at(buf, m.norm(rustle), 0.0, 0.6)
    m.at(buf, m.norm(wood(110, 0.25, 0.06, rng, 0.2)), 0.22, 0.85)
    return m.finish(room(muffle(buf, 2600), rng, 0.32, 0.12, 2500), -11.0)


def ui_close(rng):
    """画面を閉じる: 紙を畳む短い擦れ + 小さな置き音 (開くより軽い)"""
    d = 0.3
    buf = np.zeros(m.n_of(d))
    fold = m.bp(m.white(0.16, rng), 350, 1800) * m.curve(0.16, [(0, 0), (0.02, 1), (0.08, 0.5), (0.16, 0)])
    m.at(buf, m.norm(fold), 0.0, 0.6)
    m.at(buf, m.norm(wood(140, 0.15, 0.03, rng, 0.15)), 0.12, 0.6)
    return m.finish(room(muffle(buf, 2200), rng, 0.25, 0.1, 2200), -14.0)


def ui_tab(rng):
    """タブ・頁の切り替え: 厚い帳面を 1 枚めくる (低い帯域の擦れ)"""
    d = 0.24
    fc = m.curve(d, [(0, 500), (0.08, 1500), (d, 700)], 'exp')
    flip = m.sweep(m.white(d, rng), fc, q=1.2) * m.curve(d, [(0, 0), (0.025, 0.6), (0.07, 1), (0.14, 0.3), (d, 0)])
    buf = m.norm(flip)
    m.at(buf, m.norm(paper(0.05, rng, 600, 2000, 0.008)) * 0.35, 0.12)
    return m.finish(room(muffle(buf, 2400), rng, 0.22, 0.08, 2200), -14.0)


def ui_stamp(rng):
    """判子を押す: 判子を紙越しに厚い板へ「ドン」(受注・雇用・出発・地図の到達印)"""
    d = 0.5
    buf = np.zeros(m.n_of(d))
    body = m.osc(m.curve(0.35, [(0, 120), (0.05, 68)], 'exp'), 0.35) * m.exp_decay(0.35, 0.08, 0.0015)
    thwack = m.lp(m.white(0.08, rng), 1100) * m.exp_decay(0.08, 0.012, 0.0005)
    m.at(buf, m.norm(body), 0.0, 1.0)
    m.at(buf, m.norm(thwack), 0.0, 0.5)
    m.at(buf, m.norm(wood(180, 0.12, 0.025)), 0.004, 0.3)
    m.at(buf, m.norm(paper(0.05, rng, 400, 1500, 0.012)), 0.2, 0.15)
    return m.finish(room(buf, rng, 0.3, 0.12, 2000), -5.0)


def ui_refuse(rng):
    """断り: 帳場の台を指で 2 回叩く鈍い音"""
    d = 0.3
    buf = np.zeros(m.n_of(d))
    for start, gain in ((0.0, 1.0), (0.1, 0.75)):
        knock = m.osc(120, 0.1) * m.exp_decay(0.1, 0.03, 0.001) * 0.7
        knock += m.bp(m.white(0.1, rng), 120, 700) * m.exp_decay(0.1, 0.018, 0.001)
        m.at(buf, m.norm(knock), start, gain)
    return m.finish(room(buf, rng, 0.22, 0.08, 1800), -11.0)


def ui_digit(rng):
    """ルームコードの数字: 鉄のダイヤル錠が 1 目盛り回る鈍い「カチ」"""
    d = 0.1
    x = iron(430, d, 0.025, rng)
    return m.finish(room(muffle(x, 2800), rng, 0.15, 0.05, 2500), -19.0, fade_out=0.02)


def ui_game_start(rng):
    """タイトルで開始: 重い扉の鉄の閂が外れる音 + 遠くで鳴る低い青銅の鐘 1 打"""
    d = 3.0
    buf = np.zeros(m.n_of(d))
    m.at(buf, m.norm(iron(160, 0.3, 0.08, rng)), 0.0, 0.6)
    m.at(buf, m.norm(wood(75, 0.5, 0.12, rng, 0.2)), 0.02, 0.7)
    m.at(buf, m.norm(bronze_bell(D3, 2.8, 1.1)), 0.12, 0.9)
    return m.finish(m.reverb(buf, rng, rt60=1.8, mix=0.28, predelay=0.02, bright=1800), -5.0, fade_out=0.6)


def ui_stone_cursor(rng):
    """ゲームオーバーの石版上のカーソル: 小石が石に触れる低い「コツ」"""
    d = 0.1
    x = stone(d, rng, 260, 0.014)
    return m.finish(m.reverb(muffle(x, 1800), rng, rt60=0.5, mix=0.12, predelay=0.01, bright=1800), -18.0, fade_out=0.02)


def ui_stone_confirm(rng):
    """ゲームオーバーの決定: 石版を押し込む重い擦れ + 低い「ゴン」"""
    d = 0.9
    buf = np.zeros(m.n_of(d))
    grind = m.bp(m.brown(0.25, rng) + 0.3 * m.white(0.25, rng), 120, 900)
    grind *= m.curve(0.25, [(0, 0), (0.05, 1), (0.25, 0)])
    m.at(buf, m.norm(grind), 0.0, 0.5)
    m.at(buf, m.norm(stone(0.6, rng, 70, 0.14)), 0.16, 1.0)
    return m.finish(m.reverb(buf, rng, rt60=1.0, mix=0.2, predelay=0.012, bright=1500), -7.0, fade_out=0.15)


def ui_hoof_tick(rng):
    """アセット更新の進み: 早馬の蹄が土を踏む「ドッ」"""
    d = 0.12
    x = wood(190, d, 0.02, rng, 0.3) + 0.5 * m.lp(m.white(d, rng), 700) * m.exp_decay(d, 0.015, 0.001)
    return m.finish(room(muffle(x, 1600), rng, 0.2, 0.06, 1800), -19.0, fade_out=0.02)


def ui_loading_done(rng):
    """ロード完了: 木箱の蓋を閉め、鉄の留め金を掛ける「ドッ・カチャ」"""
    d = 0.6
    buf = np.zeros(m.n_of(d))
    m.at(buf, m.norm(wood(95, 0.3, 0.07, rng, 0.25)), 0.0, 1.0)
    m.at(buf, m.norm(iron(380, 0.15, 0.03, rng)), 0.14, 0.45)
    return m.finish(room(muffle(buf, 2600), rng, 0.35, 0.14, 2200), -12.0, fade_out=0.1)


# ================================================================ 系統 B (HUD)
def hud_select(rng):
    """アイテム袋の切り替え: 腰の革袋の中で瓶や道具が軽く当たる"""
    d = 0.14
    buf = np.zeros(m.n_of(d))
    m.at(buf, m.norm(leather(0.1, rng, 250, 1400, 220)), 0.0, 0.5)
    m.at(buf, m.norm(wood(290, 0.08, 0.015)), 0.02, 0.7)
    return m.finish(room(muffle(buf, 2200), rng, 0.15, 0.05, 2200), -20.0, fade_out=0.03)


def hud_palette_open(rng):
    """魔法陣パレットを開く: 手元に魔力が集まる低いうなり (空気の渦 + 地を這う低音)"""
    d = 0.6
    buf = np.zeros(m.n_of(d))
    swirl = m.sweep(m.white(d, rng), m.curve(d, [(0, 180), (0.3, 700), (d, 350)], 'exp'), q=2.0)
    swirl *= m.curve(d, [(0, 0), (0.22, 1), (d, 0)])
    hum = m.osc(73.4, d) * m.curve(d, [(0, 0), (0.2, 1), (d, 0)])
    m.at(buf, m.norm(swirl), 0.0, 0.8)
    m.at(buf, m.norm(hum), 0.0, 0.35)
    return m.finish(air(muffle(buf, 1800), rng, 0.5, 0.15, 1800), -15.0, fade_out=0.1)


def hud_page_shift(rng):
    """魔法陣パレットの頁送り: 魔法陣が回る短い低い風切り"""
    d = 0.26
    swish = m.sweep(m.white(d, rng), m.curve(d, [(0, 300), (0.1, 900), (d, 250)], 'exp'), q=1.8)
    swish *= m.curve(d, [(0, 0), (0.08, 1), (d, 0)])
    return m.finish(air(muffle(swish, 1600), rng, 0.35, 0.12, 1600), -17.0, fade_out=0.05)


def hud_lock_on(rng):
    """ロックオン: 構え直す布擦れ + 剣の鍔や籠手の金具が鳴る短い「チャッ」"""
    d = 0.3
    buf = np.zeros(m.n_of(d))
    m.at(buf, m.norm(leather(0.08, rng, 300, 1500, 250)), 0.0, 0.35)
    m.at(buf, m.norm(iron(520, 0.2, 0.035, rng)), 0.03, 0.9)
    m.at(buf, m.norm(wood(150, 0.1, 0.02)), 0.03, 0.35)
    return m.finish(room(muffle(buf, 3200), rng, 0.25, 0.08, 2600), -15.0, fade_out=0.04)


def hud_lock_off(rng):
    """ロックオン解除: 構えを解く布擦れ (金具は鳴らさない)"""
    x = leather(0.16, rng, 250, 1200, 200)
    return m.finish(room(muffle(x, 1800), rng, 0.2, 0.06, 2000), -20.0, fade_out=0.04)


def hud_ready(rng):
    """大砲の装填完了: 鉄の歯止めが 2 段「ガチッ、ガチャン」と噛み合う"""
    d = 0.5
    buf = np.zeros(m.n_of(d))
    m.at(buf, m.norm(iron(210, 0.15, 0.035, rng)), 0.0, 0.6)
    m.at(buf, m.norm(iron(170, 0.3, 0.06, rng) + 0.6 * wood(85, 0.3, 0.06)), 0.11, 1.0)
    return m.finish(room(muffle(buf, 2800), rng, 0.3, 0.12, 2200), -11.0, fade_out=0.08)


def hud_notice(rng):
    """チュートリアル札が出る: 札を板に差し込む紙の擦れ + 軽い木の音"""
    d = 0.3
    buf = np.zeros(m.n_of(d))
    slide = m.bp(m.white(0.14, rng), 350, 1600) * m.curve(0.14, [(0, 0), (0.04, 1), (0.14, 0)])
    m.at(buf, m.norm(slide), 0.0, 0.55)
    m.at(buf, m.norm(wood(200, 0.12, 0.03, rng, 0.2)), 0.1, 0.75)
    return m.finish(room(muffle(buf, 2200), rng, 0.25, 0.08, 2200), -15.0, fade_out=0.05)


def hud_clear(rng):
    """チュートリアルの課題を達成: 低い太鼓 1 打 + 青銅の鐘 (短め)"""
    d = 1.6
    buf = np.zeros(m.n_of(d))
    m.at(buf, m.norm(drum(70, 0.6, rng, 0.2)), 0.0, 0.8)
    m.at(buf, m.norm(bronze_bell(D4, 1.5, 0.55)), 0.02, 0.6)
    return m.finish(m.reverb(buf, rng, rt60=1.1, mix=0.22, predelay=0.015, bright=1800), -8.0, fade_out=0.3)


def hud_boss_appear(rng):
    """ボスのゲージが出る: 地の底から膨らむ低音 + 大太鼓 2 打"""
    d = 2.6
    buf = np.zeros(m.n_of(d))
    drone = m.osc(D3 / 4, d, 'saw', 12) + 0.6 * m.osc(D3 / 4 * 1.5, d, 'saw', 12)
    drone = m.lp(drone, 320) * m.curve(d, [(0, 0), (1.2, 1), (1.9, 0.7), (d, 0)])
    rumble = m.lp(m.brown(d, rng), 200) * m.curve(d, [(0, 0), (1.0, 1), (d, 0)])
    m.at(buf, m.norm(drone), 0.0, 0.6)
    m.at(buf, m.norm(rumble), 0.0, 0.5)
    m.at(buf, m.norm(drum(48, 1.0, rng, 0.35)), 0.0, 1.0)
    m.at(buf, m.norm(drum(44, 1.0, rng, 0.4)), 0.55, 0.85)
    return m.finish(m.reverb(buf, rng, rt60=1.6, mix=0.25, predelay=0.02, bright=1200), -3.0, fade_out=0.5)


def hud_interact(rng):
    """話しかけられる印が出る: 外套が擦れる気配 + こもった木の「トッ」(電子音にせず、低く控えめに)"""
    d = 0.2
    buf = np.zeros(m.n_of(d))
    cloth = m.bp(m.white(0.07, rng), 500, 2200) * m.curve(0.07, [(0, 0), (0.025, 1), (0.07, 0)])
    m.at(buf, m.norm(cloth), 0.0, 0.3)
    m.at(buf, m.norm(wood(D4, 0.16, 0.028, rng, 0.2)), 0.03, 0.8)
    return m.finish(room(m.lp(buf, 2400), rng, 0.22, 0.07), -17.0, fade_out=0.04)


# ================================================================ 会話
def chat_open(rng):
    """会話の吹き出しが開く: 紙を軽く広げる擦れ + 柔らかい木の音 (系統 A の open より小さく短い)"""
    d = 0.24
    buf = np.zeros(m.n_of(d))
    m.at(buf, m.norm(m.bp(m.white(0.12, rng), 400, 1800) * m.curve(0.12, [(0, 0), (0.03, 1), (0.12, 0)])), 0.0, 0.5)
    m.at(buf, m.norm(wood(210, 0.12, 0.03)), 0.05, 0.7)
    return m.finish(room(muffle(buf, 2200), rng, 0.2, 0.08, 2000), -17.0)


def chat_blip(rng):
    """文字送り: 羽根ペンが紙を擦る極小の音 (何十回も鳴るので最も小さく、低く丸めた)"""
    d = 0.05
    x = wood(380, d, 0.008) + 0.5 * paper(d, rng, 500, 1800, 0.005)
    return m.finish(muffle(x, 2000), -24.0, fade_in=0.001, fade_out=0.012)


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
        rendered[name] = stereo
        path = write_sound(name, stereo)
        print(f'{name:18s} {stereo.shape[1] / m.SR:5.2f}s  {path.stat().st_size // 1024:3d} KB')
    if args.preview:
        m.render_preview(rendered, args.preview)


if __name__ == '__main__':
    main()
