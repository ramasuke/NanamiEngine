"""ドラゴンが吐く炎のエフェクトを組んで、再生用プレハブとドラゴンの BT をつなぎ替える。

    python tools/art/dragon_fire_effects.py --build-dir <tmp> [--only NAME ...] [--install]

- Assets/Art/Effect/DragonFire/<Name>.efkefc   (+ _Source/DragonFire/<Name>.efkproj)
- テクスチャは tools/art/dragon_fire_textures.py (温度のグラデーションを焼き込んだ炎)

DragonFireball       戦闘中の火球 (FirstEventDragonFireBall.prefab, scale 10)。単位 = 当たり判定球の半径と同じ座標
DragonFireballGiant  序章で島や飛行船を焼く大火球 (FireBallParticle.prefab, scale 60)
DragonFireImpact     火球が落ちた瞬間。閃光・膨れ上がる火の玉・地を走る炎の輪・燃えながら飛ぶ破片・黒煙・焦げ跡
DragonFireBurning    着弾点で燃え続ける炎 (ループ。GenerateParticle の lifeTime_ で消す)

Impact / Burning はメートル単位でスケール 8 のプレハブ (Assets/Prefab/Particle/) から再生する。
--install は既存の 2 つの火球プレハブの particleFile_ を差し替え、FirstEventDragon の BT が出していた
IslandFireImpact / IslandBurning を新しいプレハブにつなぎ替える (AncientDragon の大砲の着弾はそのまま)。
"""
from __future__ import annotations

import argparse
import math
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.art import dragon_fire_textures  # noqa: E402
from tools.art.magic_fx_lib import (  # noqa: E402
    ADD, ALWAYS, BLEND, FIXED, ON_CREATE, YAXIS, G, N, emit_circle, emit_sphere, project,
)
from tools.common.cereal_json import Num  # noqa: E402
from tools.effect import xmlio  # noqa: E402
from tools.effect.model import Elem  # noqa: E402

from game_over_prefab import Builder, asset_guid, new_prefab  # noqa: E402
from magic_spell_prefabs import DESTROY, LOOP, LOOP_SECS, METRE, save, set_field, set_scale  # noqa: E402

INSTALL_DIR = 'Assets/Art/Effect/DragonFire'
PREFAB_DIR = REPO / 'Assets/Prefab/Particle'
FIREBALL_PREFABS = {
    'DragonFireball': REPO / 'Assets/Prefab/Npc/Enemy/FirstEventDragonFireBall.prefab',
    'DragonFireballGiant': REPO / 'Assets/Prefab/Particle/FireBallParticle.prefab',
}
DRAGON_TREE = REPO / 'Assets/Data/EnemyBehaviour/FirstEventDragon.enemyBehaviourData'
# FirstEventDragon の BT で置き換えるプレハブ -> 新しいプレハブ名
TREE_SWAPS = {
    'Assets/Prefab/Particle/IslandFireImpact.prefab': 'DragonFireImpact',
    'Assets/Prefab/Particle/IslandBurning.prefab': 'DragonFireBurning',
}
LIVE = 3600      # 単一インスタンスのレイヤー用の「エフェクト再生中ずっと」

WHITE = (255, 255, 255)
SMOKE = (42, 36, 32)
SMOKE_END = (86, 80, 74)


def a(rgb, alpha):
    return (*rgb, alpha)


def sheet(tex, frame_length, *, loop=False, start=(0, 0)):
    """フリップブックの UVAnimation。``start`` は開始コマのランダム範囲 (StartSheet)"""
    cols, rows = dragon_fire_textures.SHEETS[tex]
    w, h = {'fire_flip': (128, 128), 'fire_puff': (128, 128), 'smoke_puff': (256, 256)}[tex]
    return {'start': {'x': 0, 'y': 0}, 'size': {'x': w, 'y': h}, 'frame_length': frame_length,
            'frame_count_x': cols, 'frame_count_y': rows, 'loop_type': 1 if loop else 0, '_start': start}


def F(name, tex, *, sheet_=None, **kw):
    """テクスチャ付きノード。``sheet_`` (sheet() の戻り値) があればフリップブックにする"""
    uv = dict(sheet_) if sheet_ else None
    start = uv.pop('_start') if uv else None
    node = N(name, tex=tex, uv_anim=uv, **kw)
    if start and start != (0, 0):
        rc = node.child('RendererCommonValues')
        anim = rc.child('UVAnimation')
        lo, hi = start
        anim.children.append(Elem('StartSheet', children=[
            Elem('Center', text=str((lo + hi) // 2)), Elem('Max', text=str(hi)), Elem('Min', text=str(lo))]))
    return node


def smoke_sheet():
    return sheet('smoke_puff', 10000, start=(0, 3))


def flat(name, tex, size, *, at, blend=ADD, **kw):
    """地面に寝かせた板"""
    return F(name, tex, blend=blend, rot=(90, 0, 0), billboard=FIXED, at=at, size=size, **kw)


# ================================================================ 火球
def fireball(R, tail):
    """半径 R の火球。``tail`` = 尾を作る炎のかたまりの寿命 (フレーム)。
    尾・煙・火の粉は生成時だけ親に付いて (ON_CREATE) その場に残るので、飛ぶと自然に尾を引く"""
    puff_len = max(1, math.ceil(tail / 14))
    return project([
        F('Smoke', 'smoke_puff', blend=BLEND, sheet_=smoke_sheet(), infinite=True, interval=2, delay=6,
          life=(tail * 2, tail * 3), bind=ON_CREATE, emit=emit_sphere(R * 0.4, effects_rotation=False),
          vel=((-0.004 * R, 0.004 * R), (0.006 * R, 0.012 * R), (-0.004 * R, 0.004 * R)),
          rot_rand=(0, 0, (-180, 180)), spin=(0, 0, (-1, 1)),
          grow=((R * 1.0, R * 1.4), (R * 2.6, R * 3.4), 20, -10),
          color=a(SMOKE, 150), color_to=a(SMOKE_END, 0), fade_in=6),
        F('Halo', 'hot_glow', life=LIVE, bind=ALWAYS, size=R * 6.0, color=(255, 120, 50, 70), fade_in=5),
        F('Trail', 'fire_puff', sheet_=sheet('fire_puff', puff_len, start=(0, 2)), infinite=True, interval=1,
          life=(int(tail * 0.8), int(tail * 1.2)), bind=ON_CREATE, emit=emit_sphere(R * 0.35, effects_rotation=False),
          rot_rand=(0, 0, (-180, 180)), spin=(0, 0, (-3, 3)),
          grow=((R * 1.3, R * 1.6), (R * 2.2, R * 2.8), 20, -10),
          color=a(WHITE, 235), color_to=(255, 150, 110, 0), ease=(0, 10)),
        F('Licks', 'fire_flip', sheet_=sheet('fire_flip', 1, loop=True, start=(0, 31)), infinite=True, interval=2,
          life=(12, 18), bind=ON_CREATE, emit=emit_sphere(R * 0.5, upper=True, effects_rotation=False),
          vel=(0, (R * 0.02, R * 0.035), 0), rot_rand=(0, 0, (-20, 20)),
          size=((R * 0.9, R * 1.3), (R * 1.6, R * 2.2), 1), color=a(WHITE, 220), fade_in=3, fade_out=(6, 0, -10)),
        F('Body', 'fire_puff', sheet_=sheet('fire_puff', 1, start=(0, 3)), infinite=True, interval=1, life=(10, 14),
          bind=ALWAYS, emit=emit_sphere(R * 0.25, effects_rotation=False), rot_rand=(0, 0, (-180, 180)),
          spin=(0, 0, (-4, 4)), grow=((R * 1.6, R * 1.9), (R * 2.0, R * 2.3), 0, 0), color=a(WHITE, 255),
          fade_in=2, fade_out=(5, 0, -10)),
        F('Core', 'hot_glow', life=LIVE, bind=ALWAYS, size=R * 2.2, color=a(WHITE, 255), fade_in=3),
        F('Flicker', 'hot_glow', infinite=True, interval=2, life=6, bind=ALWAYS, size=(R * 1.6, R * 2.4),
          color=(255, 230, 190, 160), fade_in=2, fade_out=(3, 0, 0)),
        F('Embers', 'ember', infinite=True, interval=1, life=(30, 55), bind=ON_CREATE,
          emit=emit_sphere(R * 0.8, effects_rotation=False),
          vel=((-0.02 * R, 0.02 * R), (0.005 * R, 0.03 * R), (-0.02 * R, 0.02 * R)), gravity=(0, 0.0004 * R, 0),
          size=(R * 0.07, R * 0.14), color=(255, 220, 170, 255), fade_out=(12, 0, -20)),
        F('Sparks', 'ember', infinite=True, interval=2, life=(10, 18), bind=ON_CREATE,
          emit=emit_sphere(R * 0.9, effects_rotation=False),
          vel=((-0.06 * R, 0.06 * R), (-0.03 * R, 0.07 * R), (-0.06 * R, 0.06 * R)),
          size=(R * 0.05, R * 0.08), color=(255, 245, 220, 255), fade_out=(6, 0, -20)),
    ], 120, loop=True)


def dragon_fireball():
    return fireball(1.6, 22)          # 当たり判定球 radius_ 1.61 と同じ大きさ


def dragon_fireball_giant():
    return fireball(0.8, 50)          # scale 60 で半径 ~6 m。遅いので尾を長めに残す


# ================================================================ 着弾
def dragon_fire_impact():
    return project([
        F('SmokeColumn', 'smoke_puff', blend=BLEND, sheet_=smoke_sheet(), count=22, interval=1.5, delay=10,
          life=(150, 200), at=(0, 4, 0), emit=emit_sphere((1.0, 3.0), upper=True, effects_rotation=False),
          vel=((-0.01, 0.01), (0.05, 0.09), (-0.01, 0.01)), rot_rand=(0, 0, (-180, 180)), spin=(0, 0, (-0.8, 0.8)),
          grow=((5.0, 7.0), (14.0, 20.0), 0, -10), color=a(SMOKE, 190), color_to=a(SMOKE_END, 0), fade_in=8),
        F('GroundSmoke', 'smoke_puff', blend=BLEND, sheet_=smoke_sheet(), count=18, delay=3, life=(50, 80),
          at=(0, 0.8, 0), emit=emit_circle((3.0, 9.0), effects_rotation=False), rot_rand=(0, 0, (-180, 180)),
          vel=((-0.02, 0.02), (0.005, 0.02), (-0.02, 0.02)), grow=((3.0, 4.0), (8.0, 10.0), 20, -10),
          color=(70, 60, 54, 130), color_to=(110, 100, 92, 0), fade_in=4),
        flat('Scorch', 'scorch', 16.0, at=(0, 0.15, 0), blend=BLEND, delay=3, life=210, color=a(WHITE, 235),
             fade_in=4, fade_out=(60, 0, -20)),
        flat('EmberBed', 'ember_bed', 13.0, at=(0, 0.2, 0), delay=4, life=180, color=a(WHITE, 255),
             color_to=(255, 120, 80, 0), ease=(0, -20)),
        F('Flash', 'hot_glow', at=(0, 3, 0), life=10, size=26.0, color=a(WHITE, 255), fade_out=(8, 0, -20)),
        flat('FireRing', 'flame_ring', None, at=(0, 0.35, 0), life=30, grow=(3.0, 28.0, 20, -20),
             color=a(WHITE, 255), fade_out=(18, 0, -10)),
        F('Bloom', 'fire_puff', sheet_=sheet('fire_puff', 3, start=(0, 1)), count=26, interval=0.2, life=(40, 60),
          at=(0, 2, 0), emit=emit_sphere((0.5, 2.5), upper=True), vel=(0, (0.18, 0.32), 0), acc=(0, -0.005, 0),
          rot_rand=(0, 0, (-180, 180)), spin=(0, 0, (-2, 2)), grow=((4.0, 6.0), (9.0, 12.0), 20, -10),
          color=a(WHITE, 255), color_to=(255, 170, 130, 0), ease=(0, 10)),
        F('Rise', 'fire_puff', sheet_=sheet('fire_puff', 5, start=(3, 5)), count=14, interval=1, delay=8,
          life=(60, 80), at=(0, 3, 0), emit=emit_circle((0.0, 3.0), effects_rotation=False),
          vel=((-0.01, 0.01), (0.12, 0.2), (-0.01, 0.01)), acc=(0, -0.001, 0), rot_rand=(0, 0, (-180, 180)),
          spin=(0, 0, (-1.5, 1.5)), grow=((5.0, 7.0), (10.0, 14.0), 20, -10), color=(255, 230, 200, 220),
          color_to=(200, 90, 60, 0)),
        G('Brands', [
            F('BrandFlame', 'fire_puff', sheet_=sheet('fire_puff', 2, start=(2, 6)), infinite=True, interval=1,
              life=(14, 20), bind=ON_CREATE, rot_rand=(0, 0, (-180, 180)), grow=((0.9, 1.2), (0.3, 0.5), 0, 10),
              color=a(WHITE, 240), color_to=(255, 120, 80, 0)),
            F('BrandCore', 'ember', life=LIVE, bind=ALWAYS, size=0.6, color=a(WHITE, 255)),
        ], count=10, life=(45, 70), at=(0, 1.5, 0), emit=emit_sphere(1.0, upper=True), vel=(0, (0.35, 0.55), 0),
          gravity=(0, -0.018, 0)),
        F('Embers', 'ember', count=90, life=(40, 80), at=(0, 1.5, 0), emit=emit_sphere(0.5, upper=True),
          vel=(0, (0.25, 0.6), 0), gravity=(0, -0.01, 0), size=(0.2, 0.4), color=(255, 225, 180, 255),
          fade_out=(15, 0, -20)),
        F('Flames', 'fire_flip', sheet_=sheet('fire_flip', 1, loop=True, start=(0, 31)), billboard=YAXIS, count=40,
          interval=3, delay=6, life=(30, 50), at=(0, 1.5, 0), emit=emit_circle((0.5, 5.5), effects_rotation=False),
          size=((1.5, 2.5), (2.5, 4.5), 1), color=a(WHITE, 230), fade_in=4, fade_out=(12, 0, -10)),
    ], 220)


# ================================================================ 炎上 (ループ)
def dragon_fire_burning():
    return project([
        F('Smoke', 'smoke_puff', blend=BLEND, sheet_=smoke_sheet(), infinite=True, interval=4, life=(160, 220),
          at=(0, 4.5, 0), emit=emit_circle((0.5, 3.0), effects_rotation=False),
          vel=((0.005, 0.02), (0.05, 0.09), (-0.01, 0.01)), rot_rand=(0, 0, (-180, 180)), spin=(0, 0, (-0.6, 0.6)),
          grow=((3.0, 4.5), (12.0, 18.0), 0, -10), color=a(SMOKE, 150), color_to=a(SMOKE_END, 0), fade_in=10),
        flat('Scorch', 'scorch', 11.0, at=(0, 0.15, 0), blend=BLEND, life=LIVE, color=a(WHITE, 235), fade_in=20),
        flat('EmberBed', 'ember_bed', 9.0, at=(0, 0.2, 0), life=LIVE, color=a(WHITE, 200), fade_in=20),
        flat('BaseGlow', 'hot_glow', 12.0, at=(0, 0.3, 0), life=LIVE, color=(255, 120, 40, 120), fade_in=10),
        F('HeatHalo', 'hot_glow', at=(0, 3, 0), life=LIVE, size=14.0, color=(255, 110, 40, 55), fade_in=10),
        F('BigFlames', 'fire_flip', sheet_=sheet('fire_flip', 1, loop=True, start=(0, 31)), billboard=YAXIS,
          infinite=True, interval=1.5, life=(36, 54), at=(0, 2.2, 0), emit=emit_circle((0.2, 3.5), effects_rotation=False),
          size=((2.2, 3.2), (3.6, 5.2), 1), color=a(WHITE, 235), fade_in=6, fade_out=(12, 0, -10)),
        F('SmallFlames', 'fire_flip', sheet_=sheet('fire_flip', 1, loop=True, start=(0, 31)), billboard=YAXIS,
          infinite=True, interval=1, life=(20, 32), at=(0, 1.0, 0), emit=emit_circle((1.5, 4.5), effects_rotation=False),
          size=((1.0, 1.6), (1.6, 2.4), 1), color=a(WHITE, 225), fade_in=4, fade_out=(8, 0, -10)),
        F('Embers', 'ember', infinite=True, interval=2, life=(60, 110), at=(0, 1, 0),
          emit=emit_circle((0.5, 4.0), effects_rotation=False), vel=((-0.02, 0.02), (0.05, 0.1), (-0.02, 0.02)),
          size=(0.12, 0.25), color=(255, 220, 170, 255), fade_out=(20, 0, -20)),
    ], 120, loop=True)


EFFECTS = {
    'DragonFireball': (dragon_fireball, LOOP),
    'DragonFireballGiant': (dragon_fireball_giant, LOOP),
    'DragonFireImpact': (dragon_fire_impact, DESTROY),
    'DragonFireBurning': (dragon_fire_burning, LOOP),
}


def run(cmd):
    res = subprocess.run([sys.executable, '-m', *cmd], cwd=REPO, capture_output=True, text=True)
    if res.returncode != 0:
        raise SystemExit(f"FAILED: {' '.join(cmd)}\n{res.stdout}\n{res.stderr}")
    return res.stdout


def build_effect(build: Path, name: str, install: bool):
    proj = build / f'{name}.efkproj'
    xmlio.write(proj, EFFECTS[name][0]())
    run(['tools.effect', 'validate', str(proj)])
    run(['tools.effect', 'compile', str(proj)])
    print(f'compiled {proj.with_suffix(".efkefc")}')
    if install:
        print(run(['tools.effect', 'install', str(proj.with_suffix('.efkefc')), '--project', str(proj),
                   '--dest', f'{INSTALL_DIR}/{name}.efkefc']))
    return int(xmlio.read(proj).child('EndFrame').text)


def effect_guid(name):
    return asset_guid(REPO / INSTALL_DIR / f'{name}.efkefc.meta')


def build_prefab(name: str, end_frame: int):
    prefab = new_prefab(name)
    set_scale(prefab.root, METRE)
    comp = Builder(prefab).component(prefab.root, 'ParticleSystem')
    comp.data.pop('isRoop_', None)     # アーカイブしていたのはバージョン 0 だけ
    set_field(comp, 'particleFile_', effect_guid(name))
    mode = EFFECTS[name][1]
    secs = LOOP_SECS if mode == LOOP else round(end_frame / 60.0 + 0.1, 2)
    comp.data['playingDuration_secs_'] = Num.of_float(secs)
    comp.data['playMode_'] = Num.of_int(mode)
    return save(prefab, PREFAB_DIR, name)


def repoint_fireball_prefab(name: str):
    """既存の火球プレハブ (当たり判定・移動はそのまま) の ParticleSystem だけを新しいエフェクトにする。
    ループの区切りで再生し直すと尾が途切れるので、再生時間は LOOP_SECS にする (弾はその前に消える)"""
    path = FIREBALL_PREFABS[name]
    text = path.read_bytes().decode('utf-8-sig')
    block = re.search(r'"particleFile_":.*?"playMode_":\s*\d+', text, re.S)
    body = re.sub(r'("value_":\s*")[0-9A-F-]{36}(")', rf'\g<1>{effect_guid(name)}\g<2>', block.group(0), count=1)
    body = re.sub(r'("playingDuration_secs_":\s*)[\d.]+', rf'\g<1>{LOOP_SECS}', body)
    body = re.sub(r'("playMode_":\s*)\d+', rf'\g<1>{LOOP}', body)
    path.write_bytes((text[:block.start()] + body + text[block.end():]).encode('utf-8'))
    print(f'repointed {path.relative_to(REPO)} -> {name}')


def rewire_tree(new_guids: dict[str, str]):
    text = DRAGON_TREE.read_bytes().decode('utf-8-sig')
    for old, new_name in TREE_SWAPS.items():
        old_guid = asset_guid(REPO / f'{old}.meta')
        n = text.count(old_guid)
        text = text.replace(old_guid, new_guids[new_name])
        print(f'{DRAGON_TREE.name}: {Path(old).stem} -> {new_name} ({n} refs)')
    DRAGON_TREE.write_bytes(text.encode('utf-8'))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build-dir', required=True)
    ap.add_argument('--only', nargs='*', choices=sorted(EFFECTS))
    ap.add_argument('--install', action='store_true', help='install the effects, write/repoint the prefabs, rewire the BT')
    args = ap.parse_args()
    build = Path(args.build_dir).resolve()
    build.mkdir(parents=True, exist_ok=True)
    dragon_fire_textures.write_all(build / 'Texture')
    new_guids = {}
    for name in args.only or EFFECTS:
        end_frame = build_effect(build, name, args.install)
        if not args.install:
            continue
        if name in FIREBALL_PREFABS:
            repoint_fireball_prefab(name)
        else:
            new_guids[name] = build_prefab(name, end_frame)
    if args.install and set(TREE_SWAPS.values()) <= set(new_guids):
        rewire_tree(new_guids)


if __name__ == '__main__':
    main()
