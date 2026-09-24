"""お金 / アイテムを拾った時のエフェクトと SE を作る (PickupItemBase.pickupParticle_ / pickupSound_)。

    python tools/art/pickup_fx.py --build-dir <tmp> [--install]

- Assets/Art/Effect/Pickup/{CoinPickupSparkle,ItemPickupSparkle}.efkefc   (+ _Source/Pickup/*.efkproj)
- Assets/Prefab/Particle/{CoinPickupSparkle,ItemPickupSparkle}.prefab     : scale 8 で一度だけ再生して消える
- Assets/Audio/Physics/{Pickup_Coin,Pickup_Item}.mp3                      : 常に書く (--install 不要)

単位は m (tools/art/magic_fx_lib.py と同じ)。原点は拾い物のあった位置 (地面近く)。
一度に何枚も拾うので、エフェクトは小さく短く (0.5 秒ほど)、SE も短くしてある。
お金の SE はリアル寄りの世界観に合わせ、硬貨の擦れる音だけで作る (チャイム的な音程は付けない)。
SE の部品・mp3 形式・.meta は tools/art/magic_sfx.py のもの。
"""
from __future__ import annotations

import argparse
import sys
import uuid
from pathlib import Path

import numpy as np

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.art import magic_fx_textures  # noqa: E402
from tools.art.magic_fx_lib import FIXED, YAXIS, N, emit_circle, emit_sphere, project  # noqa: E402
from tools.common.cereal_json import Num  # noqa: E402
from tools.effect import xmlio  # noqa: E402

import magic_sfx as m  # noqa: E402
from chest_open_effect import run  # noqa: E402
from game_over_prefab import Builder, asset_guid, new_prefab  # noqa: E402
from magic_spell_prefabs import DESTROY, METRE, save, set_field, set_scale  # noqa: E402

INSTALL_DIR = 'Assets/Art/Effect/Pickup'
PREFAB_DIR = REPO / 'Assets/Prefab/Particle'
SOUND_DIR = REPO / 'Assets/Audio/Physics'
END_FRAME = 45

WHITE = (255, 252, 235)
GOLD = (255, 214, 90)
MINT = (170, 255, 225)
TEAL = (70, 215, 200)


def a(rgb, alpha):
    return (*rgb, alpha)


# ---------------------------------------------------------------- エフェクト

def coin_pickup_sparkle():
    return project([
        # 拾った瞬間の小さな金の閃光
        N('Flash', tex='glow_core', at=(0, 0.35, 0), life=10, grow=(0.5, 1.4, 10, 0), color=a(WHITE, 255),
          fade_out=(8, 0, -20)),
        N('Glow', tex='glow', at=(0, 0.35, 0), life=16, grow=(0.9, 1.8, 10, 0), color=a(GOLD, 200),
          fade_out=(12, 0, -20)),
        # 足元に広がる細い輪
        N('Ripple', kind='ring', rot=(90, 0, 0), billboard=FIXED, at=(0, 0.05, 0), life=16,
          grow=(0.2, 1.0, 20, -10),
          ring={'outer': (0.9, 0), 'inner': (0.72, 0), 'center_ratio': 0.5, 'outer_color': a(GOLD, 0),
                'center_color': a(GOLD, 220), 'inner_color': a(GOLD, 0)}, fade_out=(12, 0, -20)),
        # 上へ散って落ちる十字のきらめき
        N('Sparkles', tex='cross', count=7, interval=0, life=(22, 34), at=(0, 0.35, 0),
          emit=emit_sphere((0.05, 0.2), upper=True), vel=(0, (0.035, 0.07), 0), gravity=(0, -0.003, 0),
          grow=((0.22, 0.34), 0.03, 0, 10), spin=(0, 0, (-5, 5)), color=a(GOLD, 255),
          color_spread=(0, 20, 30, 0), fade_out=(10, 0, -20)),
        # 少し遅れて周りで瞬く星
        N('Twinkle', tex='spark', count=4, interval=2, delay=4, life=(12, 18),
          at_rand=((-0.45, 0.45), (0.3, 1.0), (-0.45, 0.45)), grow=((0.25, 0.4), 0.03, 0, 20), spin=(0, 0, 4),
          color=a(WHITE, 255)),
    ], END_FRAME)


def item_pickup_sparkle():
    return project([
        # ふわっと膨らんで消える柔らかい光
        N('Puff', tex='glow', at=(0, 0.5, 0), life=20, grow=(0.6, 2.0, 10, 0), color=a(MINT, 190),
          fade_in=2, fade_out=(14, 0, -20)),
        N('Core', tex='glow_core', at=(0, 0.5, 0), life=12, grow=(0.4, 1.0, 10, 0), color=a(WHITE, 255),
          fade_out=(8, 0, -20)),
        # 足元の輪
        N('Ripple', kind='ring', rot=(90, 0, 0), billboard=FIXED, at=(0, 0.05, 0), life=18,
          grow=(0.2, 1.1, 20, -10),
          ring={'outer': (1.0, 0), 'inner': (0.82, 0), 'center_ratio': 0.5, 'outer_color': a(TEAL, 0),
                'center_color': a(TEAL, 210), 'inner_color': a(TEAL, 0)}, fade_out=(14, 0, -20)),
        # 吸い込まれるように上へ昇る光の筋と粒
        N('Streaks', tex='streak', billboard=YAXIS, count=6, interval=1, life=(14, 20),
          at_rand=((-0.35, 0.35), 0.1, (-0.35, 0.35)), vel=(0, (0.05, 0.08), 0),
          size=((0.05, 0.08), (0.5, 0.8), 1), color=a(WHITE, 220), fade_in=2, fade_out=(10, 0, -20)),
        N('Motes', tex='glow', count=10, interval=1, life=(24, 34), at=(0, 0.2, 0), emit=emit_circle((0.15, 0.5)),
          vel=(0, (0.025, 0.045), 0), grow=((0.12, 0.2), 0.03, 0, 0), color=a(TEAL, 235),
          color_spread=(20, 20, 20, 0), fade_in=3, fade_out=(14, 0, -20)),
    ], END_FRAME)


EFFECTS = {
    'CoinPickupSparkle': coin_pickup_sparkle,
    'ItemPickupSparkle': item_pickup_sparkle,
}


def build_effect(build: Path, name: str, recipe, install: bool):
    build.mkdir(parents=True, exist_ok=True)
    magic_fx_textures.write_all(build / 'Texture')
    proj = build / f'{name}.efkproj'
    xmlio.write(proj, recipe())
    run(['tools.effect', 'validate', str(proj)])
    run(['tools.effect', 'compile', str(proj)])
    print(f'compiled {proj.with_suffix(".efkefc")}')
    if install:
        print(run(['tools.effect', 'install', str(proj.with_suffix('.efkefc')), '--project', str(proj),
                   '--dest', f'{INSTALL_DIR}/{name}.efkefc']))


def build_prefab(name: str):
    prefab = new_prefab(name)
    set_scale(prefab.root, METRE)
    comp = Builder(prefab).component(prefab.root, 'ParticleSystem')
    comp.data.pop('isRoop_', None)     # アーカイブしていたのはバージョン 0 だけ
    set_field(comp, 'particleFile_', asset_guid(REPO / INSTALL_DIR / f'{name}.efkefc.meta'))
    comp.data['playingDuration_secs_'] = Num.of_float(round(END_FRAME / 60.0 + 0.1, 2))
    comp.data['playMode_'] = Num.of_int(DESTROY)
    return save(prefab, PREFAB_DIR, name)


# ---------------------------------------------------------------- サウンド

def _bell(f, d, tau):
    """澄んだ金属音。硬貨より倍音を減らして明るく"""
    partials = [(1.0, 1.0), (2.0, 0.35), (3.01, 0.2), (4.17, 0.08)]
    return sum(m.osc(f * ratio, d) * gain * m.exp_decay(d, tau / (1 + ratio * 0.3), 0.001) for ratio, gain in partials)


def _coin_hit(f, rng, d=0.12):
    """硬貨どうしが触れる1回分。非整数倍の振動モードが短く減衰するだけで、音程感は出さない"""
    modes = [(1.0, 1.0), (1.69, 0.7), (2.47, 0.5), (3.31, 0.3), (4.23, 0.18)]
    out = np.zeros(m.n_of(d))
    for ratio, gain in modes:
        fr = f * ratio * rng.uniform(0.985, 1.015)
        if fr > m.SR * 0.45:
            continue
        out += m.osc(fr, d) * gain * m.exp_decay(d, rng.uniform(0.025, 0.05) / ratio ** 0.5, 0.0003)
    tick = m.hp(m.white(0.006, rng) * m.exp_decay(0.006, 0.0012, 0.0002), 2500)
    m.at(out, m.norm(tick), 0.0, 0.6)
    return m.norm(out)


def pickup_coin(rng):
    """数枚の硬貨が擦れ合って革袋に収まる「ジャリッ」。ベルやチャイムの音程は入れない"""
    d = 0.32
    buf = np.zeros(m.n_of(d))
    # 手で掴んで袋に入れる時の鈍い擦れと当たり
    rustle = m.bp(m.white(0.1, rng), 400, 2500) * m.exp_decay(0.1, 0.03, 0.008)
    m.at(buf, m.norm(rustle), 0.0, 0.18)
    thump = m.lp(m.white(0.05, rng), 350) * m.exp_decay(0.05, 0.012, 0.001)
    m.at(buf, m.norm(thump), 0.02, 0.35)
    # 硬貨の触れ合い。間隔も高さも不揃いにして、後ほど小さく
    t = 0.008
    for i in range(5):
        m.at(buf, _coin_hit(rng.uniform(2600, 4200), rng), t, 0.55 * 0.72 ** i * rng.uniform(0.8, 1.0))
        t += rng.uniform(0.018, 0.045)
    buf = m.lp(buf, 9000)
    stereo = m.reverb(buf, rng, rt60=0.18, mix=0.06, predelay=0.003, bright=6000)
    return m.finish(stereo, -8.0, fade_out=0.05)


def pickup_item(rng):
    """「ポン」と軽くはじけてから、上がっていく小さなチャイム3音"""
    d = 0.55
    buf = np.zeros(m.n_of(d))
    pop = m.chirp(520, 260, 0.07) * m.exp_decay(0.07, 0.025, 0.001)
    pop += m.bp(m.white(0.07, rng), 300, 1600) * m.exp_decay(0.07, 0.012, 0.001) * 0.4
    m.at(buf, m.norm(pop), 0.0, 0.8)
    for i, f in enumerate((1047, 1319, 1568)):   # C6 E6 G6
        t = 0.05 + 0.055 * i
        m.at(buf, m.norm(_bell(f, d - t, 0.14)), t, 0.55 + 0.1 * i)
    stereo = m.reverb(buf, rng, rt60=0.35, mix=0.15, predelay=0.006, bright=8000)
    return m.finish(stereo, -4.0, fade_out=0.08)


SOUNDS = {
    'Pickup_Coin': pickup_coin,
    'Pickup_Item': pickup_item,
}


def write_sound(name, stereo):
    SOUND_DIR.mkdir(parents=True, exist_ok=True)
    path = SOUND_DIR / f'{name}.mp3'
    path.write_bytes(m.encode_mp3(stereo))
    meta = Path(str(path) + '.meta')
    if not meta.exists():
        content = str(path.relative_to(m.REPO)).replace('/', '\\').replace('\\', '\\\\')
        text = m.META.format(path=content, guid=str(uuid.uuid4()).upper()).replace('\n', '\r\n')
        meta.write_bytes(text.encode('utf-8'))
    return path


def build_sounds():
    for i, (name, recipe) in enumerate(SOUNDS.items()):
        stereo = m.trim_tail(recipe(np.random.default_rng(3000 + i)))
        path = write_sound(name, stereo)
        print(f'{name:16s} {stereo.shape[1] / m.SR:5.2f}s  {path.stat().st_size // 1024:3d} KB')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build-dir', required=True)
    ap.add_argument('--install', action='store_true', help='install the effects and write the prefabs')
    args = ap.parse_args()
    build = Path(args.build_dir).resolve()
    for name, recipe in EFFECTS.items():
        build_effect(build / name, name, recipe, args.install)
        if args.install:
            build_prefab(name)
    build_sounds()


if __name__ == '__main__':
    main()
