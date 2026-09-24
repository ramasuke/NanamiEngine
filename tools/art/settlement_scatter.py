"""GrassLandScene に「ティラノに踏み荒らされた村」と「山の上へ逃げた狩猟民のキャンプ」を置く。

    python tools/art/settlement_prefabs.py       # 先にプレハブを組む
    python tools/art/settlement_scatter.py        # シーンの Settlement ルートを置き直す
    python tools/art/settlement_scatter.py --plan plan.json   # 配置案だけ JSON に書き出す (シーンは触らない)

- 村は中央の盆地 (床の高さ ~66) の縁に沿って、家の正面を広場へ向けて並べる。広場の真ん中はティラノと戦う場所なので
  当たり判定のある物を置かない (VILLAGE_CLEAR)。
- キャンプは盆地の北、一段高い棚 (高さ ~186)。焚き火を囲んでテントを張り、盆地側の縁に防柵と見張り台を置く。
  見張り台は盆地 (村) の方を向く。
- 高さは grassland_nature_scatter と同じ地形メッシュから三角形補間で取り、足元の一番低い所より少し沈める。
- 置く物に重なる Nature の木・岩・茂み・花・倒木は取り除く (取り除いた数を表示する)。草は GrassField から抜く。
- 何度実行しても Settlement ルートは1組だけになる。Nature から抜いた物は戻らないので、戻すなら nature_scatter を再実行する。
"""
import argparse
import base64
import json
import math
import sys
from pathlib import Path

import numpy as np

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, dumps, loads, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, reader, validate, writer  # noqa: E402

from game_over_prefab import check, let_writer_place_versions  # noqa: E402
from grassland_nature_scatter import (SCENE, StrayVersionStripper, Terrain, bake_world_matrices, first_versions,  # noqa: E402
                                      quat_axis, walk)

PREFAB = REPO / 'Assets' / 'Prefab' / 'Prop' / 'Settlement'
GRASS_META = REPO / 'Assets' / 'Data' / 'GrassField' / 'GrassLandGrass.grassField.meta'
ROOT_NAME = 'Settlement'

VILLAGE_CENTER = (840.0, 610.0)
VILLAGE_CLEAR = 95.0          # 広場の中心からこの半径には当たり判定のある物を置かない
CAMP_FIRE = (978.0, 1160.0)
BASIN_VIEW = (840.0, 610.0)   # 見張り台・トーテムが向く先

# 名前: (幅 x, 奥行き z) world @scale 1 と、足元を沈める量 world。Nature を抜く半径は外形の対角の半分
FOOTPRINT = {
    'RuinedStoneHouse': ((87.0, 53.0), 2.0),
    'RuinedTimberHouse': ((60.0, 56.0), 2.0),
    'CrushedHut': ((50.0, 50.0), 1.5),
    'BrokenFence': ((50.0, 8.0), 0.8),
    'RubblePile': ((25.0, 22.0), 1.2),
    'BrokenCart': ((42.0, 34.0), 0.6),
    'HideTent': ((40.0, 40.0), 0.6),
    'LeanTo': ((35.0, 21.0), 0.6),
    'Campfire': ((25.0, 28.0), 0.4),
    'DryingRack': ((55.0, 21.0), 0.6),
    'Totem': ((18.0, 9.0), 1.5),
    'Palisade': ((51.0, 16.0), 1.5),
    'Lookout': ((24.0, 31.0), 1.0),
    'SupplyPile': ((25.0, 18.0), 0.6),
}
MID_HEIGHT = {'Palisade'}
# 草を抜く物 (床のある家・テントの中・焚き火)
CLEAR_GRASS = {'RuinedStoneHouse': 0.8, 'RuinedTimberHouse': 0.8, 'CrushedHut': 0.9, 'HideTent': 1.0,
               'LeanTo': 0.9, 'Campfire': 1.2, 'RubblePile': 0.8, 'SupplyPile': 0.9}

# (プレハブ, x, z, 向き, scale)。向き: ('face', (x, z)) で正面(+Z)をその点へ、('away', (x, z)) で背を向ける、
# ('run', deg) で柵などの長手(+X)を world の XZ 平面上の角度 deg (0 = +X, 90 = +Z) へ、数値なら yaw(deg)
RUINS = [
    ('RuinedStoneHouse', 684.0, 655.0, ('face', VILLAGE_CENTER), 1.1),
    ('RuinedTimberHouse', 700.0, 540.0, ('face', VILLAGE_CENTER), 1.1),
    ('CrushedHut', 748.0, 728.0, ('face', VILLAGE_CENTER), 1.0),
    ('CrushedHut', 772.0, 423.0, ('face', VILLAGE_CENTER), 1.05),
    ('RuinedStoneHouse', 884.0, 433.0, ('face', VILLAGE_CENTER), 1.0),
    ('RuinedTimberHouse', 990.0, 494.0, ('face', VILLAGE_CENTER), 1.05),
    ('RuinedStoneHouse', 1006.0, 628.0, ('face', VILLAGE_CENTER), 0.95),
    ('CrushedHut', 963.0, 758.0, ('face', VILLAGE_CENTER), 0.95),
    ('BrokenFence', 738.0, 612.0, ('run', 95.0), 1.0),
    ('BrokenFence', 752.0, 482.0, ('run', 20.0), 1.0),
    ('BrokenFence', 948.0, 452.0, ('run', 15.0), 1.0),
    ('BrokenFence', 957.0, 560.0, ('run', 80.0), 1.0),
    ('BrokenFence', 910.0, 772.0, ('run', 25.0), 1.0),
    ('BrokenFence', 806.0, 762.0, ('run', -25.0), 1.0),
    ('RubblePile', 772.0, 680.0, 30.0, 1.2),
    ('RubblePile', 938.0, 690.0, 110.0, 1.0),
    ('RubblePile', 826.0, 482.0, 200.0, 1.1),
    ('RubblePile', 972.0, 545.0, 300.0, 0.9),
    ('BrokenCart', 870.0, 803.0, 35.0, 1.0),
    ('BrokenCart', 790.0, 470.0, 160.0, 1.0),
]
CAMP = [
    ('Campfire', CAMP_FIRE[0], CAMP_FIRE[1], 0.0, 1.0),
    ('HideTent', 938.0, 1200.0, ('face', CAMP_FIRE), 1.0),
    ('HideTent', 990.0, 1218.0, ('face', CAMP_FIRE), 1.1),
    ('HideTent', 1036.0, 1190.0, ('face', CAMP_FIRE), 0.95),
    ('LeanTo', 925.0, 1162.0, ('face', CAMP_FIRE), 1.0),
    ('DryingRack', 1020.0, 1133.0, ('run', 12.0), 1.0),
    ('SupplyPile', 1052.0, 1164.0, ('face', CAMP_FIRE), 1.0),
    ('Totem', 1066.0, 1126.0, ('face', BASIN_VIEW), 1.2),
    ('Totem', 898.0, 1204.0, ('face', BASIN_VIEW), 1.0),
    ('Lookout', 985.0, 1104.0, ('away', BASIN_VIEW), 1.0),
    ('Palisade', 946.0, 1120.0, ('run', -45.0), 1.0),
    ('Palisade', 1032.0, 1104.0, ('run', 22.0), 1.0),
]


class Placement:
    __slots__ = ('group', 'prefab', 'x', 'z', 'y', 'yaw', 'scale')

    def __init__(self, group, prefab, x, z, y, yaw, scale):
        self.group, self.prefab, self.x, self.z, self.y, self.yaw, self.scale = group, prefab, x, z, y, yaw, scale

    def corners(self, pad=0.0):
        (w, d), _ = FOOTPRINT[self.prefab]
        hw, hd = w * self.scale / 2 + pad, d * self.scale / 2 + pad
        c, s = math.cos(self.yaw), math.sin(self.yaw)
        # yaw a: ローカル +X -> (cos a, -sin a)、+Z -> (sin a, cos a)
        return [(self.x + c * sx * hw + s * sz * hd, self.z - s * sx * hw + c * sz * hd)
                for sx, sz in ((-1, -1), (1, -1), (1, 1), (-1, 1))]

    def contains(self, x, z, pad=0.0):
        (w, d), _ = FOOTPRINT[self.prefab]
        dx, dz = x - self.x, z - self.z
        c, s = math.cos(self.yaw), math.sin(self.yaw)
        lx = dx * c - dz * s
        lz = dx * s + dz * c
        return abs(lx) <= w * self.scale / 2 + pad and abs(lz) <= d * self.scale / 2 + pad


def yaw_of(spec, x, z):
    if isinstance(spec, tuple):
        kind, arg = spec
        if kind == 'face':
            return math.atan2(arg[0] - x, arg[1] - z)
        if kind == 'away':
            return math.atan2(x - arg[0], z - arg[1])
        if kind == 'run':
            a = math.radians(arg)
            return math.atan2(-math.sin(a), math.cos(a))
        raise ValueError(kind)
    return math.radians(spec)


def plan(t):
    out = []
    for group, items in (('Ruins', RUINS), ('Camp', CAMP)):
        for prefab, x, z, facing, scale in items:
            yaw = yaw_of(facing, x, z)
            p = Placement(group, prefab, x, z, 0.0, yaw, scale)
            samples = p.corners() + [(x, z)] + [((a[0] + x) / 2, (a[1] + z) / 2) for a in p.corners()]
            hs = [float(t.height(sx, sz)) for sx, sz in samples]
            _, sink = FOOTPRINT[prefab]
            # 防柵は縁に沿って長いので、低い端に合わせると高い側が埋まる。真ん中の高さに合わせる
            base = (min(hs) + max(hs)) / 2 if prefab in MID_HEIGHT else min(hs)
            p.y = base - sink * scale
            relief = max(hs) - min(hs)
            slope = float(t.slope(x, z))
            warn = '  <- uneven' if relief > 6.0 * scale else ''
            if group == 'Ruins' and prefab not in ('BrokenCart',) and math.hypot(x - VILLAGE_CENTER[0], z - VILLAGE_CENTER[1]) < VILLAGE_CLEAR:
                warn += '  <- inside the fight area'
            print(f'  {group:5s} {prefab:18s} ({x:6.1f}, {p.y:6.1f}, {z:6.1f}) yaw {math.degrees(yaw):7.1f}  '
                  f'slope {slope:4.1f}  relief {relief:4.1f}{warn}')
            out.append(p)
    return out


# ---------------------------------------------------------------- Nature / 草の整理
def remove_overlapping_nature(scene, placements):
    nature = next((r for r in scene.roots if r.name == 'Nature'), None)
    if nature is None:
        return
    removed = {}
    for group in nature.transform.children:
        if group.name == 'Grass':
            continue
        keep = []
        for obj in group.transform.children:
            x, _, z = edits._node_local_trs(obj).pos
            hit = next((p for p in placements if p.contains(x, z, pad=6.0 if group.name in ('Trees', 'Rocks') else 2.0)), None)
            if hit is not None:
                removed[group.name] = removed.get(group.name, 0) + 1
            else:
                keep.append(obj)
        group.transform.children = keep
    print(f'  removed from Nature: {removed}')


def clear_grass(placements):
    root = loads(GRASS_META.read_bytes().decode('utf-8-sig'))
    body = root['value0']['ptr_wrapper']['data']
    chunk = float(body['chunkSize_'].value)
    targets = [p for p in placements if p.prefab in CLEAR_GRASS]
    total = removed = 0
    for rec in body['chunks']:
        cx, cz = int(rec['cx'].value), int(rec['cz'].value)
        x0, z0 = cx * chunk, cz * chunk
        near = [p for p in targets
                if x0 - 60 <= p.x <= x0 + chunk + 60 and z0 - 60 <= p.z <= z0 + chunk + 60]
        count = int(rec['count'].value)
        total += count
        if not near:
            continue
        raw = np.frombuffer(base64.b64decode(rec['blades']), dtype='<u2').reshape(-1, 3)
        x = (raw[:, 0] / 65535.0 + cx) * chunk
        z = (raw[:, 1] / 65535.0 + cz) * chunk
        keep = np.ones(len(raw), bool)
        for p in near:
            (w, d), _ = FOOTPRINT[p.prefab]
            f = CLEAR_GRASS[p.prefab]
            c, s = math.cos(p.yaw), math.sin(p.yaw)
            dx, dz = x - p.x, z - p.z
            lx = dx * c - dz * s
            lz = dx * s + dz * c
            keep &= ~((np.abs(lx) <= w * p.scale / 2 * f) & (np.abs(lz) <= d * p.scale / 2 * f))
        if keep.all():
            continue
        removed += int((~keep).sum())
        kept = raw[keep]
        rec['count'] = Num.of_int(len(kept))
        rec['blades'] = base64.b64encode(kept.astype('<u2').tobytes()).decode('ascii')
    GRASS_META.write_bytes(to_file_bytes(dumps(root)))
    print(f'  grass: removed {removed} of {total} blades under houses / tents / fire')


# ---------------------------------------------------------------- シーン
def build_scene(placements):
    scene = reader.read_scene_file(SCENE)
    scene.roots = [r for r in scene.roots if r.name != ROOT_NAME]
    remove_overlapping_nature(scene, placements)

    root = edits.add_gameobject(scene, parent=None, name=ROOT_NAME)
    groups = {name: edits.add_gameobject(scene, parent=root.guid, name=name) for name in ('Ruins', 'Camp')}
    prefabs = {}
    for p in placements:
        if p.prefab not in prefabs:
            prefabs[p.prefab] = reader.read_prefab_file(PREFAB / f'{p.prefab}.prefab')
        node = edits.instantiate_prefab(scene, prefabs[p.prefab], parent=groups[p.group].guid)
        node.transform.local_pos = edits._vec3_from_floats((p.x, p.y, p.z))
        node.transform.local_rot = edits._quat_from_floats(quat_axis((0.0, 1.0, 0.0), p.yaw))
        node.transform.local_scale = edits._vec3_from_floats((float(p.scale),) * 3)
    for node in walk(root):
        for comp in node.components:
            let_writer_place_versions(comp.data)
    bake_world_matrices(root)

    text = writer.write_scene(scene)
    tree = loads(text)
    stripper = StrayVersionStripper(catalog_mod.load(), f'/gameObject_{len(scene.roots) - 1}/')
    stripper.run(tree, '')
    text = dumps(tree)
    print(f'  stripped {stripper.stripped} repeat cereal_class_version key(s) inside {ROOT_NAME}')

    versions = first_versions(text)
    for type_key, want in (('ColliderBase', 6), ('NanamiEngine::Module::Component::BoxCollider', 6),
                           ('NanamiEngine::Module::Component::ModelRenderer', 5)):
        if versions.get(type_key) != want:
            raise SystemExit(f'{type_key}: first occurrence is v{versions.get(type_key)}, expected v{want} - nothing written')
    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(SCENE)
    print(f'wrote {SCENE.relative_to(REPO)}  ({len(placements)} objects under {ROOT_NAME})')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--plan', help='配置案を JSON に書き出すだけでシーンは触らない')
    ap.add_argument('--no-grass', action='store_true', help='草を抜かない')
    args = ap.parse_args()

    t = Terrain()
    placements = plan(t)
    if args.plan:
        items = []
        for p in placements:
            (w, d), _ = FOOTPRINT[p.prefab]
            items.append({'name': p.prefab, 'x': p.x, 'z': p.z, 'yaw': math.degrees(p.yaw),
                          'w': w * p.scale, 'd': d * p.scale})
        Path(args.plan).write_text(json.dumps(items, indent=1), encoding='utf-8')
        return
    build_scene(placements)
    if not args.no_grass:
        clear_grass(placements)


if __name__ == '__main__':
    main()
