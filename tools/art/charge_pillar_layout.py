"""GrassLandScene の「突進で倒せる柱」(Gimmicks > ChargePillars) の並びを決め、倒れた柱の置物を置く。

    python tools/art/charge_pillar_layout.py

- Assets/Prefab/Prop/Settlement/FallenRuinPillar.prefab : BreakableRuinPillar から柱の仕掛けを外し、上半分を倒した置物。
  倒れた柱に大顎が突っ込んだ跡、として見せる (上半分の当たりは倒れた向きに付いて行く)
- 立っている柱: 上半分を少し傾け、ぐらついて見せる。登場演出で大顎が最初に突っ込む柱 (isIntroTarget_) は
  大顎の湧き位置の正面に置く (湧き位置から約 110。頭の先 40 + 突進 0.6 秒ぶん)
何度流しても同じ結果になる (柱は guid で探し、置物は置き直す)。高さは地形 - 0.3。
"""
from __future__ import annotations

import math
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.blob import Ver  # noqa: E402
from tools.common.cereal_json import Num, dumps, loads, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, mathutil, reader, validate, writer  # noqa: E402

from game_over_prefab import check  # noqa: E402
from grassland_nature_scatter import SCENE, StrayVersionStripper, Terrain, bake_world_matrices, quat_axis, walk  # noqa: E402
from magic_spell_prefabs import find_child, find_comp, save  # noqa: E402

PREFAB_DIR = REPO / 'Assets/Prefab/Prop/Settlement'
BREAKABLE = PREFAB_DIR / 'BreakableRuinPillar.prefab'
FALLEN_NAME = 'FallenRuinPillar'
GROUP = 'ChargePillars'
SINK = 0.3
FALL_DEG = 84.0     # ChargeBreakPillar::fallAngle_deg_ と同じだけ倒す

# guid の頭8桁: (x, z, 根元の向き deg, 上半分を傾ける向き deg, 傾き deg, 登場演出の標的)
STANDING = {
    'DF304F5D': (812.0, 655.0, 20.0, 200.0, 3.0, False),
    '01ECCC6C': (908.0, 648.0, 75.0, 35.0, 2.5, False),
    '3A9886B7': (842.0, 738.0, 140.0, 290.0, 3.5, False),
    'C831C0E3': (898.0, 738.0, 250.0, 120.0, 2.5, False),
    '582D9B41': (878.0, 562.0, 310.0, 250.0, 3.0, True),
}
# 倒れた柱: (x, z, 倒れた向き deg)。0 = -Z、90 = -X。登場演出で倒れる柱と並べて南側に置く
FALLEN = (826.0, 606.0, 35.0)


def tilt_rot(toward_deg, tilt_deg):
    """上向きを toward_deg (0 = -Z, 90 = -X) の方へ tilt_deg 傾ける"""
    a = math.radians(toward_deg)
    direction = (-math.sin(a), 0.0, -math.cos(a))
    axis = (direction[2], 0.0, -direction[0])     # cross(up, direction)
    return quat_axis(axis, math.radians(tilt_deg))


def yaw_rot(deg):
    return quat_axis((0.0, 1.0, 0.0), math.radians(deg))


def build_fallen_prefab():
    prefab = edits.copy_prefab(reader.read_prefab_file(BREAKABLE))
    root = prefab.root
    root.name = FALLEN_NAME
    root.components = [c for c in root.components
                        if not c.fqn.endswith(('::ChargeBreakPillar', '::ChargeStuckObstacle'))]
    dust = find_child(root, 'DustPoint')
    root.transform.children.remove(dust)
    # NOTE: 子の並びを変えると版キーの初出がずれるので、当たりは並びを保ったまま倒れた上半分に重ねる
    top = find_child(root, 'Top')
    fall = tilt_rot(0.0, FALL_DEG)
    top.transform.local_rot = edits._quat_from_floats(fall)
    standing = find_child(root, 'StandingCollider')
    standing.name = 'TopCollider'
    top_pos = edits._vec3_floats(top.transform.local_pos)
    offset = mathutil.vec3_sub(edits._vec3_floats(standing.transform.local_pos), top_pos)
    standing.transform.local_pos = edits._vec3_from_floats(
        mathutil.vec3_add(top_pos, mathutil.quat_rotate_vec(fall, offset)))
    standing.transform.local_rot = edits._quat_from_floats(fall)
    # NOTE: ComponentBase の初出は外した ChargeStuckObstacle にあったので、次に書かれる当たりへ版キーを移す
    collider_base = find_comp(standing, 'BoxCollider').data['value0']
    component_base = collider_base.body['value0']
    if not isinstance(component_base, Ver):
        collider_base.body['value0'] = Ver(('type', 'ComponentBase'), 0, component_base, literal_presence=True)
    return save(prefab, PREFAB_DIR, FALLEN_NAME)


def pillar_component(node):
    return find_comp(node, 'ChargeBreakPillar')


def main():
    build_fallen_prefab()
    terrain = Terrain()
    scene = reader.read_scene_file(SCENE)
    group = next(n for r in scene.roots for n in walk(r) if n.name == GROUP)

    for node in group.transform.children:
        spec = STANDING.get(node.guid[:8].upper())
        if spec is None:
            continue
        x, z, yaw, toward, tilt, intro = spec
        y = float(terrain.height(x, z)) - SINK
        node.transform.local_pos = edits._vec3_from_floats((x, y, z))
        node.transform.local_rot = edits._quat_from_floats(yaw_rot(yaw))
        # NOTE: 上半分の傾きは根元の向きを打ち消してワールドの向きで決める
        top = find_child(node, 'Top')
        local_tilt = mathutil.quat_mul(mathutil.quat_conjugate(yaw_rot(yaw)),
                                       mathutil.quat_mul(tilt_rot(toward, tilt), yaw_rot(yaw)))
        top.transform.local_rot = edits._quat_from_floats(local_tilt)
        comp = pillar_component(node)
        comp.data['isIntroTarget_'] = intro
        comp.data['collapseDamage_'] = Num.of_int(0 if intro else 30)
        print(f'  {node.guid[:8]}  ({x:.0f}, {y:.2f}, {z:.0f})  tilt {tilt} toward {toward}{"  [intro]" if intro else ""}')

    # NOTE: 置物はプレハブの写しなので、プレハブを作り直したら置き直す
    group.transform.children = [n for n in group.transform.children if n.name != FALLEN_NAME]
    fallen = edits.instantiate_prefab(scene, reader.read_prefab_file(PREFAB_DIR / f'{FALLEN_NAME}.prefab'),
                                      parent=group.guid)
    x, z, toward = FALLEN
    fallen.transform.local_pos = edits._vec3_from_floats((x, float(terrain.height(x, z)) - SINK, z))
    fallen.transform.local_rot = edits._quat_from_floats(yaw_rot(toward))
    parent_world = mathutil.IDENTITY
    bake_world_matrices(group, parent_world)

    text = writer.write_scene(scene)
    tree = loads(text)
    group_index = next(i for i, r in enumerate(scene.roots) if any(n is group for n in walk(r)))
    stripper = StrayVersionStripper(catalog_mod.load(), f'/gameObject_{group_index}/')
    stripper.run(tree, '')
    text = dumps(tree)
    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(SCENE)
    print(f'wrote {SCENE.relative_to(REPO)}')


if __name__ == '__main__':
    main()
