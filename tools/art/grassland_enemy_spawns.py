"""GrassLandScene の敵の湧き地点 (EnemySpawnPoints の子) を置き直す。

    python tools/art/grassland_enemy_spawns.py

- ティラノサウルスは盆地 (荒れた村) の北寄り。正面を北の入口へ向け、登場演出のカメラ (プレハブの
  Tyrannosaurus ProductionCamera = 背後 (-170, 120, -250)) が盆地の南側の空中に来る向きにしてある。
- ハイエナは西の林に3頭ずつの群れで置く。仲間を呼ぶ (Hyena::CallAllies) のは EnemyFactory から湧いた個体だけ。
- 各地点は EnemySpawnPoint (kind_) を持つ。種別ごとのグループ GameObject は EnemySpawnPoint を持たないので湧かない。
- 高さは地形メッシュ + 少し浮かせる (RigidBody で落ちて接地する)。
"""
import math
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, reader, validate, writer  # noqa: E402

from game_over_prefab import check  # noqa: E402
from grassland_nature_scatter import SCENE, Terrain, bake_world_matrices, quat_axis  # noqa: E402

ROOT_NAME = 'EnemySpawnPoints'
DROP_HEIGHT = 6.0

# EnemyKind (Assets/Scripts/Core/Game/Npc/Enemy/Type/EnemyKind.h)
HYENA = 2
TYRANNOSAURUS = 3

TREX = ('Tyrannosaurus', 860.0, 690.0, -17.0)
# 群れ: (名前, 中心 x, z, [(dx, dz, yaw deg), ...])
HYENA_PACKS = [
    ('HyenaPack_WestWoods', 402.0, 845.0, [(0.0, 0.0, 200.0), (-16.0, 12.0, 150.0), (14.0, 15.0, 240.0)]),
    ('HyenaPack_WestRidge', 300.0, 712.0, [(0.0, 0.0, 90.0), (-10.0, -16.0, 40.0), (12.0, -12.0, 130.0)]),
    ('HyenaPack_WestShelf', 458.0, 492.0, [(0.0, 0.0, 30.0), (-15.0, 10.0, 350.0), (16.0, 8.0, 70.0)]),
]


def spawn_point(scene, cat, parent, name, x, z, yaw_deg, kind, t):
    y = float(t.height(x, z)) + DROP_HEIGHT
    node = edits.add_gameobject(scene, parent=parent.guid, name=name, pos=(x, y, z),
                                rot=quat_axis((0.0, 1.0, 0.0), math.radians(yaw_deg)))
    comp = edits.add_component(scene, node.guid, 'EnemySpawnPoint', cat=cat, params={})
    # enum は tools.scene が 0 で書くので直接入れる
    comp.data['kind_'] = Num.of_int(kind)
    print(f'  {name:14s} kind {kind}  ({x:6.1f}, {y:6.1f}, {z:6.1f})  yaw {yaw_deg:6.1f}  slope {float(t.slope(x, z)):4.1f}')
    return node


def main():
    t = Terrain()
    cat = catalog_mod.load()
    scene = reader.read_scene_file(SCENE)
    root = next(r for r in scene.roots if r.name == ROOT_NAME)
    root.transform.children = []

    name, x, z, yaw = TREX
    spawn_point(scene, cat, root, name, x, z, yaw, TYRANNOSAURUS, t)
    for pack, cx, cz, members in HYENA_PACKS:
        group = edits.add_gameobject(scene, parent=root.guid, name=pack, pos=(cx, float(t.height(cx, cz)), cz))
        for i, (dx, dz, yaw) in enumerate(members):
            node = spawn_point(scene, cat, root, f'Hyena{i + 1}', cx + dx, cz + dz, yaw, HYENA, t)
            edits.move_gameobject(scene, guid=node.guid, new_parent=group.guid)
    bake_world_matrices(root)

    text = writer.write_scene(scene)
    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(SCENE)
    print(f'wrote {SCENE.relative_to(REPO)}')


if __name__ == '__main__':
    main()
