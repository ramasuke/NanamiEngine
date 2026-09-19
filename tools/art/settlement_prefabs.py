"""GrassLand の「荒れた村」と「狩猟民のキャンプ」の建物プレハブを組む。

    python tools/art/settlement_prefabs.py

モデルは Assets/Art/Models/Settlement (tools/art/settlement/ の Blender スクリプトで m 単位に作り、tools.model で変換)。
木や岩のプレハブと同じく「ルート(scale 1) + Model 子(scale 0.08)」で、world = m x 8。

当たり判定は Blender で箱として定義したもの (data/settlement_colliders.json) を、ルート直下の Collider 子
(Transform に位置と回転、BoxCollider の size_ に寸法) にする。Blender の (x, y, z) はエンジンの (x, z, y)。
.meta (asset guid) は既存があれば保つので、組み直してもシーン側の参照は切れない。
"""
import json
import math
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import to_file_bytes  # noqa: E402
from tools.scene import edits, meta as scene_meta, reader, validate, writer  # noqa: E402

from game_over_prefab import Builder, all_nodes, asset_guid, check, let_writer_place_versions, new_prefab  # noqa: E402
from grassland_nature_prefabs import MODEL_SCALE, box_collider  # noqa: E402
from grassland_nature_scatter import bake_world_matrices  # noqa: E402

PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'Prop' / 'Settlement'
MODELS = REPO / 'Assets' / 'Art' / 'Models' / 'Settlement'
COLLIDERS = Path(__file__).resolve().parent / 'data' / 'settlement_colliders.json'
METERS = 8.0  # Blender 1m = world 8 (Model 0.08 x .mv1 のフレーム倍率 100)

RUINS = ['RuinedStoneHouse', 'RuinedTimberHouse', 'CrushedHut', 'BrokenFence', 'RubblePile', 'BrokenCart']
CAMP = ['HideTent', 'LeanTo', 'Campfire', 'DryingRack', 'Totem', 'Palisade', 'Lookout', 'SupplyPile']


def quat_from_matrix(m):
    """3x3 回転行列 (行優先) -> (x, y, z, w)"""
    tr = m[0][0] + m[1][1] + m[2][2]
    if tr > 0:
        s = math.sqrt(tr + 1.0) * 2
        return ((m[2][1] - m[1][2]) / s, (m[0][2] - m[2][0]) / s, (m[1][0] - m[0][1]) / s, 0.25 * s)
    if m[0][0] > m[1][1] and m[0][0] > m[2][2]:
        s = math.sqrt(1.0 + m[0][0] - m[1][1] - m[2][2]) * 2
        return (0.25 * s, (m[0][1] + m[1][0]) / s, (m[0][2] + m[2][0]) / s, (m[2][1] - m[1][2]) / s)
    if m[1][1] > m[2][2]:
        s = math.sqrt(1.0 + m[1][1] - m[0][0] - m[2][2]) * 2
        return ((m[0][1] + m[1][0]) / s, 0.25 * s, (m[1][2] + m[2][1]) / s, (m[0][2] - m[2][0]) / s)
    s = math.sqrt(1.0 + m[2][2] - m[0][0] - m[1][1]) * 2
    return ((m[0][2] + m[2][0]) / s, (m[1][2] + m[2][1]) / s, 0.25 * s, (m[1][0] - m[0][1]) / s)


def to_engine(box):
    """Blender の箱 -> (位置, 回転 quat, 寸法)。軸の入れ替え P (y<->z) で R' = P R P"""
    cx, cy, cz = box['center']
    sx, sy, sz = box['size']
    r = box['rot']
    perm = (0, 2, 1)
    m = [[r[perm[i]][perm[j]] for j in range(3)] for i in range(3)]
    return (cx * METERS, cz * METERS, cy * METERS), quat_from_matrix(m), (sx * METERS, sz * METERS, sy * METERS)


def build(name, boxes):
    prefab = new_prefab(name)
    b = Builder(prefab)
    for i, box in enumerate(boxes):
        pos, rot, size = to_engine(box)
        node = edits.add_gameobject(prefab, parent=prefab.root.guid, name='Collider', pos=pos, rot=rot)
        node.components.append(box_collider(size, (0.0, 0.0, 0.0)))
    node = edits.add_gameobject(prefab, parent=prefab.root.guid, name='Model', pos=(0.0, 0.0, 0.0),
                                scale=(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE))
    b.component(node, 'ModelRenderer', mv1File_=asset_guid(MODELS / f'{name}.mv1.meta'), useFixedInterpolation_='false')
    return save(prefab, name)


def save(prefab, name):
    """game_over_prefab.save_prefab と同じ手順。ただし Collider 子に回転があるので world は回転込みで焼く"""
    PREFAB_DIR.mkdir(parents=True, exist_ok=True)
    path = PREFAB_DIR / f'{name}.prefab'
    for node in all_nodes(prefab.root):
        for comp in node.components:
            let_writer_place_versions(comp.data)
    bake_world_matrices(prefab.root)
    text = writer.write_prefab(prefab)
    check(text, validate.validate_prefab(prefab), path.name)
    path.write_bytes(to_file_bytes(text))
    meta = Path(str(path) + '.meta')
    if meta.exists():
        guid = asset_guid(meta)
    else:
        guid = scene_meta.mint_guid().upper()
        content_path = scene_meta.content_path_for(scene_meta.PREFAB_SPEC, name, PREFAB_DIR, REPO)
        scene_meta.write_meta(scene_meta.PREFAB_SPEC, meta, name, guid, content_path)
    reader.read_prefab_file(path)
    print(f'wrote {path.relative_to(REPO)}  (asset guid {guid})')
    return guid, path


def main():
    colliders = json.loads(COLLIDERS.read_text(encoding='utf-8'))
    for name in RUINS + CAMP:
        build(name, colliders.get(name, []))


if __name__ == '__main__':
    main()
