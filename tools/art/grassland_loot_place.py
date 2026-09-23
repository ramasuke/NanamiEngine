"""GrassLandScene に薬草と宝箱を置く (ルート `Loot` を毎回作り直す)。

    python tools/art/grassland_loot_place.py

置き場所は data/grassland_loot.json。エディタ (AutoMCP) で見ながら直した値をここへ書き戻して再実行する。
高さは地形 (grassland_nature_scatter.Terrain) の足元の一番低い所。json に y があればそちらを使う。
プレハブは tools/art/loot_prefabs.py。
"""
import json
import math
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import OrderedObj, dumps, loads, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, reader, validate, writer  # noqa: E402

from game_over_prefab import check, let_writer_place_versions  # noqa: E402
from grassland_nature_scatter import (SCENE, StrayVersionStripper, Terrain, bake_world_matrices,  # noqa: E402
                                      quat_axis, walk)

ROOT_NAME = 'Loot'
PLACEMENTS = Path(__file__).resolve().parent / 'data' / 'grassland_loot.json'
PREFABS = {
    'TreasureChest': REPO / 'Assets' / 'Prefab' / 'Prop' / 'Loot' / 'TreasureChest.prefab',
    'HerbPatch': REPO / 'Assets' / 'Prefab' / 'Prop' / 'Loot' / 'HerbPatch.prefab',
}
# 足元を調べる半径と、地面へ沈める深さ (world)。斜面で浮かないよう一番低い所に合わせてから少し埋める
FOOTPRINT = {'TreasureChest': (7.0, 0.6), 'HerbPatch': (1.5, 0.2)}
GROUPS = {'TreasureChest': 'TreasureChests', 'HerbPatch': 'Herbs'}


PREFAB_FIELDS = ('harvestParticle_', 'openParticle_')


def strip_prefab_field_versions(node):
    """NOTE: Field<PrefabGameObjectFile> の初出は CustomNetworkRunner (カタログ外) なので、監査が見落とす版キーを消す"""
    if isinstance(node, OrderedObj):
        for key, value in node.items():
            if key in PREFAB_FIELDS and isinstance(value, OrderedObj):
                value.pop(validate._VER, None)
                value['value0']['ptr_wrapper']['data'].pop(validate._VER, None)
            else:
                strip_prefab_field_versions(value)
    elif isinstance(node, list):
        for value in node:
            strip_prefab_field_versions(value)


def main():
    placements = json.loads(PLACEMENTS.read_text(encoding='utf-8'))
    terrain = Terrain()
    scene = reader.read_scene_file(SCENE)
    # NOTE: 元の並び順に戻さないと、後ろのルートの polymorphic_id が全部ずれて差分が膨らむ
    index = next((i for i, r in enumerate(scene.roots) if r.name == ROOT_NAME), None)
    scene.roots = [r for r in scene.roots if r.name != ROOT_NAME]

    root = edits.add_gameobject(scene, parent=None, name=ROOT_NAME)
    if index is not None:
        scene.roots.remove(root)
        scene.roots.insert(index, root)
    count = 0
    for kind, path in PREFABS.items():
        prefab = reader.read_prefab_file(path)
        group = edits.add_gameobject(scene, parent=root.guid, name=GROUPS[kind])
        radius, sink = FOOTPRINT[kind]
        for i, p in enumerate(placements.get(kind, [])):
            x, z = float(p['x']), float(p['z'])
            y = float(p['y']) if 'y' in p else terrain.lowest(x, z, radius) - sink
            yaw = math.radians(float(p.get('yaw', (i * 137.5) % 360)))
            node = edits.instantiate_prefab(scene, prefab, parent=group.guid)
            node.transform.local_pos = edits._vec3_from_floats((x, y, z))
            node.transform.local_rot = edits._quat_from_floats(quat_axis((0.0, 1.0, 0.0), yaw))
            count += 1
            print(f'  {kind:14s} ({x:7.1f}, {y:6.1f}, {z:7.1f})  yaw {math.degrees(yaw):5.1f}')

    for node in walk(root):
        for comp in node.components:
            let_writer_place_versions(comp.data)
    bake_world_matrices(root)

    text = writer.write_scene(scene)
    tree = loads(text)
    stripper = StrayVersionStripper(catalog_mod.load(), f'/gameObject_{scene.roots.index(root)}/')
    stripper.run(tree, '')
    strip_prefab_field_versions(tree[f'gameObject_{scene.roots.index(root)}'])
    text = dumps(tree)
    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(SCENE)
    print(f'wrote {SCENE.relative_to(REPO)}  ({count} objects under {ROOT_NAME}, '
          f'stripped {stripper.stripped} repeat version key(s))')


if __name__ == '__main__':
    main()
