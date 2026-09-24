"""アイテムを使ったときの演出 (パーティクル + SE) を組んで、ItemData に結び付ける。

    python tools/art/item_use_fx.py --build-dir <tmp> [--install]

- Assets/Art/Effect/Item/ItemUse_<Kind>.efkefc   (+ _Source/Item/ItemUse_<Kind>.efkproj)
- Assets/Prefab/Particle/ItemUse<Kind>.prefab    : scale 8 で一度だけ再生して消える
- Assets/Audio/Item/ItemUse_<Kind>.mp3
- Assets/Data/Item/*.itemData.meta               : useParticle_ / useSound_ を埋める (ItemData version 4)

プレハブは使い手の足元に出て、PlayItemUseCue が消えるまで使い手の位置へ付いて行かせる。
単位は m (tools/art/magic_fx_lib.py と同じ。原点は足元、アバターは身長 2 m)。
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

import numpy as np

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.art import magic_fx_textures  # noqa: E402
from tools.art.magic_fx_lib import ALWAYS, FIXED, ON_CREATE, YAXIS, N, emit_circle, emit_sphere, project  # noqa: E402
from tools.common.cereal_json import Num  # noqa: E402
from tools.effect import xmlio  # noqa: E402

import magic_sfx as m  # noqa: E402
from game_over_prefab import Builder, asset_guid, new_prefab  # noqa: E402
from magic_spell_prefabs import DESTROY, METRE, save, set_field, set_scale  # noqa: E402

INSTALL_DIR = 'Assets/Art/Effect/Item'
PREFAB_DIR = REPO / 'Assets/Prefab/Particle'
SOUND_DIR = REPO / 'Assets/Audio/Item'
ITEM_DIR = REPO / 'Assets/Data/Item'
ITEM_DATA_VERSION = 4


def a(rgb, alpha):
    return (*rgb, alpha)


# ================================================================ エフェクト
MINT = (120, 255, 150)
LEAF = (60, 220, 110)
PALE = (225, 255, 230)


def heal():
    """回復: 足元に緑の輪が広がり、体を包む光と十字の粒が昇っていく"""
    return [
        # 足元の輪 (体に付いて行く)
        N('Ripple', kind='ring', rot=(90, 0, 0), billboard=FIXED, bind=ALWAYS, at=(0, 0.05, 0), life=36,
          grow=(0.2, 1.0, 20, -10),
          ring={'outer': (1.3, 0), 'inner': (1.05, 0), 'center_ratio': 0.4, 'outer_color': a(MINT, 0),
                'center_color': a(MINT, 230), 'inner_color': a(MINT, 0)}, fade_out=(26, 0, -20)),
        N('FloorGlow', tex='glow', rot=(90, 0, 0), billboard=FIXED, bind=ALWAYS, at=(0, 0.04, 0), life=60,
          size=2.4, color=a(LEAF, 150), fade_in=6, fade_out=(40, 0, -20)),
        # 体を包む光
        N('BodyGlow', tex='glow', billboard=YAXIS, bind=ALWAYS, at=(0, 1.0, 0), life=50, size=(1.6, 2.6, 1),
          color=a(MINT, 120), fade_in=8, fade_out=(34, 0, -20)),
        N('Flash', tex='glow_core', bind=ALWAYS, at=(0, 1.1, 0), life=14, grow=(0.8, 2.2, 20, 0),
          color=a(PALE, 200), fade_out=(12, 0, -20)),
        # 周りから昇る光の粒 (その場に残って昇る)
        N('Motes', tex='glow', bind=ON_CREATE, count=36, interval=1, life=(40, 60), at=(0, 0.1, 0),
          emit=emit_circle((0.3, 0.75)), vel=(0, (0.02, 0.045), 0), grow=((0.12, 0.22), 0.03, 0, 0),
          color=a(MINT, 235), color_spread=(30, 0, 40, 0), fade_in=4, fade_out=(24, 0, -20)),
        # 十字がふわっと昇る
        N('Crosses', tex='cross', bind=ON_CREATE, count=7, interval=5, delay=4, life=(40, 52),
          at_rand=((-0.6, 0.6), (0.4, 1.4), (-0.6, 0.6)), vel=(0, (0.012, 0.022), 0), size=(0.22, 0.34),
          color=a(PALE, 255), fade_in=6, fade_out=(20, 0, -20)),
        # 頭の上で瞬く
        N('Twinkle', tex='spark', bind=ALWAYS, count=8, interval=4, delay=8, life=(16, 24),
          at_rand=((-0.7, 0.7), (1.2, 2.3), (-0.7, 0.7)), grow=((0.3, 0.5), 0.04, 0, 20), spin=(0, 0, 4),
          color=a(PALE, 255)),
    ], 100


EMBER = (255, 170, 60)
GOLD = (255, 220, 120)
HOT = (255, 250, 220)


def stamina():
    """スタミナ: 暖色の閃光と火の粉が弾け、上向きの矢印が昇って力がみなぎる"""
    return [
        N('Flash', tex='glow_core', bind=ALWAYS, at=(0, 1.0, 0), life=12, grow=(0.9, 2.6, 20, 0),
          color=a(HOT, 220), fade_out=(10, 0, -20)),
        N('Ripple', kind='ring', rot=(90, 0, 0), billboard=FIXED, bind=ALWAYS, at=(0, 0.05, 0), life=26,
          grow=(0.15, 1.0, 20, -10),
          ring={'outer': (1.4, 0), 'inner': (1.15, 0), 'center_ratio': 0.4, 'outer_color': a(EMBER, 0),
                'center_color': a(EMBER, 230), 'inner_color': a(EMBER, 0)}, fade_out=(20, 0, -20)),
        N('BodyGlow', tex='glow', billboard=YAXIS, bind=ALWAYS, at=(0, 1.0, 0), life=44, size=(1.5, 2.5, 1),
          color=a(EMBER, 120), fade_in=4, fade_out=(30, 0, -20)),
        # 上へ抜ける筋
        N('Streaks', tex='streak', billboard=YAXIS, bind=ON_CREATE, count=12, interval=1, life=(18, 26),
          at_rand=((-0.55, 0.55), (0.0, 0.6), (-0.55, 0.55)), vel=(0, (0.07, 0.11), 0),
          size=((0.06, 0.1), (0.7, 1.1), 1), color=a(GOLD, 220), fade_in=3, fade_out=(14, 0, -20)),
        # 上向きの矢印が昇る
        N('Chevrons', tex='chevron', bind=ON_CREATE, count=6, interval=4, delay=2, life=(30, 38),
          at_rand=((-0.5, 0.5), (0.3, 0.9), (-0.5, 0.5)), vel=(0, (0.025, 0.035), 0), size=(0.3, 0.42),
          color=a(GOLD, 255), fade_in=4, fade_out=(16, 0, -20)),
        # 弾けて落ちる火の粉
        N('Embers', tex='spark', bind=ON_CREATE, count=26, life=(28, 44), at=(0, 1.0, 0),
          emit=emit_sphere((0.1, 0.35)), vel=(0, (0.02, 0.06), 0), gravity=(0, -0.003, 0),
          grow=((0.12, 0.2), 0.03, 0, 20), spin=(0, 0, (-6, 6)), color=a(EMBER, 255),
          color_spread=(0, 40, 40, 0), fade_out=(12, 0, -20)),
    ], 80


PUFF = (255, 240, 200)


def place():
    """設置物: 手元でぽんと光って、小さな火花が散る"""
    return [
        N('Pop', tex='glow_core', bind=ALWAYS, at=(0, 0.9, 0), life=12, grow=(0.5, 1.6, 20, 0),
          color=a(PUFF, 200), fade_out=(10, 0, -20)),
        N('Ripple', kind='ring', rot=(90, 0, 0), billboard=FIXED, bind=ALWAYS, at=(0, 0.05, 0), life=20,
          grow=(0.2, 1.0, 20, -10),
          ring={'outer': (1.0, 0), 'inner': (0.82, 0), 'center_ratio': 0.4, 'outer_color': a(PUFF, 0),
                'center_color': a(PUFF, 170), 'inner_color': a(PUFF, 0)}, fade_out=(16, 0, -20)),
        N('Sparks', tex='spark', bind=ON_CREATE, count=12, life=(18, 28), at=(0, 0.9, 0),
          emit=emit_sphere((0.05, 0.2)), vel=(0, (0.015, 0.04), 0), gravity=(0, -0.002, 0),
          grow=((0.1, 0.16), 0.03, 0, 20), color=a(GOLD, 255), color_spread=(0, 30, 40, 0)),
    ], 40


EFFECTS = {'Heal': heal, 'Stamina': stamina, 'Place': place}


# ================================================================ サウンド
def heal_sound(rng):
    d = 1.3
    bells = np.zeros(m.n_of(d))
    for i, f in enumerate((784.0, 987.8, 1174.7, 1568.0)):
        m.at(bells, m.fm_bell(f, d - i * 0.07, 1.41, 1.6, 0.45), i * 0.07, 1.0 - i * 0.12)
    shimmer = m.pings(d, rng, 16, 3000, 7500, 0.12, 0.1, 0.9) * 0.3
    air = m.bp(m.white(d, rng), 2500, 8000) * m.curve(d, [(0, 0), (0.25, 1), (d, 0)]) * 0.06
    return m.finish(m.reverb(m.norm(bells) * 0.7 + shimmer + air, rng, 1.2, 0.35), -6, fade_out=0.3)


def stamina_sound(rng):
    d = 0.9
    # かぶりつく歯ごたえ
    bite = m.lp(m.white(0.09, rng) * m.exp_decay(0.09, 0.025, 0.002), 1800)
    bite += m.osc(m.curve(0.09, [(0, 180), (0.09, 90)], 'exp'), 0.09) * m.exp_decay(0.09, 0.03) * 0.6
    buf = np.zeros(m.n_of(d))
    m.at(buf, m.norm(bite), 0.0, 0.8)
    # 力が湧き上がる
    swell = m.lp(m._chord((196.0, 293.7, 392.0), d - 0.12, 'saw', 0.004), 2200)
    swell *= m.curve(d - 0.12, [(0, 0), (0.25, 1), (d - 0.12, 0)]) ** 1.5
    rise = m.sweep(m.white(d - 0.12, rng), m.curve(d - 0.12, [(0, 500), (0.35, 4000)], 'exp'), 1.5)
    rise *= m.curve(d - 0.12, [(0, 0), (0.2, 1), (d - 0.12, 0)]) ** 2
    m.at(buf, m.norm(swell) * 0.5 + m.norm(rise) * 0.25, 0.12)
    return m.finish(m.reverb(buf, rng, 0.8, 0.25), -5, fade_out=0.2)


def place_sound(rng):
    d = 0.35
    thud = m.osc(m.curve(d, [(0, 140), (0.08, 70)], 'exp'), d) * m.exp_decay(d, 0.06, 0.002)
    knock = m.bp(m.white(d, rng), 200, 1400) * m.exp_decay(d, 0.03, 0.001)
    return m.finish(m.reverb(m.norm(thud) * 0.7 + m.norm(knock) * 0.4, rng, 0.3, 0.12), -6, fade_out=0.08)


SOUNDS = {'Heal': heal_sound, 'Stamina': stamina_sound, 'Place': place_sound}

# アイテム -> 演出の種類
ITEMS = {'Herb': 'Heal', 'HealPotion': 'Heal', 'RoastedMeat': 'Stamina', 'LargeBarrelBomb': 'Place'}


# ================================================================ ビルド
def run(cmd):
    res = subprocess.run([sys.executable, '-m', *cmd], cwd=REPO, capture_output=True, text=True)
    if res.returncode != 0:
        raise SystemExit(f"FAILED: {' '.join(cmd)}\n{res.stdout}\n{res.stderr}")
    return res.stdout


def build_effects(build: Path, install: bool):
    build.mkdir(parents=True, exist_ok=True)
    magic_fx_textures.write_all(build / 'Texture')
    for kind, recipe in EFFECTS.items():
        name = f'ItemUse_{kind}'
        children, end_frame = recipe()
        proj = build / f'{name}.efkproj'
        xmlio.write(proj, project(children, end_frame))
        run(['tools.effect', 'validate', str(proj)])
        run(['tools.effect', 'compile', str(proj)])
        print(f'compiled {proj.with_suffix(".efkefc")}')
        if install:
            print(run(['tools.effect', 'install', str(proj.with_suffix('.efkefc')), '--project', str(proj),
                       '--dest', f'{INSTALL_DIR}/{name}.efkefc']))


def build_prefab(kind):
    name = f'ItemUse{kind}'
    end_frame = EFFECTS[kind]()[1]
    prefab = new_prefab(name)
    set_scale(prefab.root, METRE)
    comp = Builder(prefab).component(prefab.root, 'ParticleSystem')
    comp.data.pop('isRoop_', None)     # アーカイブしていたのはバージョン 0 だけ
    set_field(comp, 'particleFile_', asset_guid(REPO / INSTALL_DIR / f'ItemUse_{kind}.efkefc.meta'))
    comp.data['playingDuration_secs_'] = Num.of_float(round(end_frame / 60.0 + 0.1, 2))
    comp.data['playMode_'] = Num.of_int(DESTROY)
    return save(prefab, PREFAB_DIR, name)


def build_sounds():
    m.OUT_DIR = SOUND_DIR
    guids = {}
    for i, (kind, recipe) in enumerate(SOUNDS.items()):
        stereo = m.trim_tail(recipe(np.random.default_rng(3000 + i)))
        path = m.write_sound(f'ItemUse_{kind}', stereo)
        guids[kind] = asset_guid(Path(str(path) + '.meta'))
        print(f'{path.relative_to(REPO)}  {stereo.shape[1] / m.SR:4.2f}s')
    return guids


def _set_guid_field(text, key, guid):
    """"key": { ... "value_": "<guid>" } の guid を差し替える"""
    pattern = re.compile(r'("' + key + r'"\s*:\s*\{.*?"value_"\s*:\s*")[0-9A-Fa-f-]{36}(")', re.S)
    text, n = pattern.subn(lambda mt: mt.group(1) + guid + mt.group(2), text, count=1)
    if n != 1:
        raise SystemExit(f'{key} not found')
    return text


# NOTE: pickupPrefab_ で同じ型が先に出ているので cereal_class_version は書かない (書くと位置読みの value0 がずれて読めない)
PARTICLE_FIELD = '''
                "useParticle_": {{
                    "value0": {{
                        "polymorphic_id": 1073741824,
                        "ptr_wrapper": {{
                            "id": {id},
                            "data": {{
                                "value0": {{
                                    "value_": "{guid}"
                                }}
                            }}
                        }}
                    }}
                }}'''


def wire_items(particle_guids, sound_guids):
    for item, kind in ITEMS.items():
        path = ITEM_DIR / f'{item}.itemData.meta'
        raw = path.read_bytes()
        bom = raw.startswith(b'\xef\xbb\xbf')
        text = raw.decode('utf-8-sig').replace('\r\n', '\n')
        text = _set_guid_field(text, 'useSound_', sound_guids[kind])
        if '"useParticle_"' in text:
            text = _set_guid_field(text, 'useParticle_', particle_guids[kind])
        else:
            text = re.sub(r'("cereal_class_version": )\d+', rf'\g<1>{ITEM_DATA_VERSION}', text, count=1)
            ids = [int(v) for v in re.findall(r'"id": (\d+)', text)]
            new_id = max(v for v in ids if v >= 2147483648) + 1
            end = text.rindex('\n            }\n        }\n    }\n}')
            text = text[:end] + ',' + PARTICLE_FIELD.format(id=new_id, guid=particle_guids[kind]) + text[end:]
        path.write_bytes((b'\xef\xbb\xbf' if bom else b'') + text.replace('\n', '\r\n').encode('utf-8'))
        print(f'wired {path.relative_to(REPO)} -> {kind}')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build-dir', required=True)
    ap.add_argument('--install', action='store_true', help='install the effects, write the prefabs / sounds, wire the items')
    args = ap.parse_args()
    build_effects(Path(args.build_dir).resolve(), args.install)
    if args.install:
        particle_guids = {kind: build_prefab(kind) for kind in EFFECTS}
        sound_guids = build_sounds()
        wire_items(particle_guids, sound_guids)


if __name__ == '__main__':
    main()
