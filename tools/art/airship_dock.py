"""飛空艇が着く台 (FirstIsland/AirShipPedestal) の見た目を AirShipDock.mv1 に差し替える。序章と第1章の両方。

    python tools/art/airship_dock.py [prologue] [main]      # 省略時は両方。何度流してもよい

- モデルは NanamiAssetsWork/Dock/build_dock.py (Blender) で作る。台の BoxCollider と 3 つの Stairs/Collider に
  合わせてあるので、当たり判定はそのまま使う。
- 古い見た目 (道のタイル 6 枚と鉄の階段 3 つ) は ModelRenderer を切るだけにする。Stairs は子の Collider を持つので消さない。
- AirShipDock は FirstIsland の子。モデルの原点 = 台の上面の中心、ワールドの大きさ (FirstIsland の scale 0.4 を打ち消す)。
"""
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import dumps, loads, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, model, reader, validate, writer  # noqa: E402

from game_over_prefab import Builder, asset_guid, check, let_writer_place_versions, world_matrix_blob  # noqa: E402
from desert_scene import find  # noqa: E402
from grassland_nature_scatter import StrayVersionStripper  # noqa: E402

SCENES = {
    'prologue': REPO / 'Assets' / 'Scene' / 'FirstTouchDownMainIsLandScene.scene',
    'main': REPO / 'Assets' / 'Scene' / 'MainIslandScene.scene',
}
MODEL = REPO / 'Assets' / 'Art' / 'Models' / 'Pedestal' / 'AirShipDock' / 'AirShipDock.mv1.meta'
NAME = 'AirShipDock'
ISLAND_POS, ISLAND_SCALE = (0.0, 29.92, 0.0), 0.4
# 台の BoxCollider の上面の中心 (ワールド)
DOCK_WORLD = (-12.0, 34.7697, -21.712)
MODEL_RENDERER = 'NanamiEngine::Module::Component::ModelRenderer'


def build(key):
    path = SCENES[key]
    scene = reader.read_scene_file(path)
    island = find(scene, 'FirstIsland')
    pedestal = find(scene, 'FirstIsland', 'AirShipPedestal')
    if abs(edits._vec3_floats(island.transform.local_scale)[0] - ISLAND_SCALE) > 1e-4:
        raise SystemExit(f'{path.name}: FirstIsland scale changed - nothing written')

    hidden = 0
    for child in pedestal.transform.children:
        if child.name not in ('Tile', 'Stairs'):
            continue
        for comp in child.components:
            if comp.fqn == MODEL_RENDERER:
                model.set_component_enabled(comp, False)
                hidden += 1

    island.transform.children = [c for c in island.transform.children if c.name != NAME]
    local = tuple((DOCK_WORLD[i] - ISLAND_POS[i]) / ISLAND_SCALE for i in range(3))
    node = edits.add_gameobject(scene, parent=island.guid, name=NAME, pos=local,
                                scale=(1.0 / ISLAND_SCALE,) * 3)
    Builder(scene).component(node, 'ModelRenderer', mv1File_=asset_guid(MODEL))
    for comp in node.components:
        let_writer_place_versions(comp.data)
    node.transform.world_matrix = world_matrix_blob((1.0, 1.0, 1.0), DOCK_WORLD)

    text = writer.write_scene(scene)
    # NOTE: 新しい ModelRenderer の Field<Mv1File> に付いた版キーは、シーンの中では2回目以降なので消す
    tree = loads(text)
    stripper = StrayVersionStripper(catalog_mod.load(), f'/gameObject_{scene.roots.index(island)}/')
    stripper.run(tree, '')
    text = dumps(tree)
    check(text, validate.validate_scene(scene), path.name)
    path.write_bytes(to_file_bytes(text))
    reader.read_scene_file(path)
    print(f'wrote {path.relative_to(REPO)}  ({hidden} old renderers off)')


def main():
    keys = sys.argv[1:] or list(SCENES)
    for key in keys:
        if key not in SCENES:
            raise SystemExit(f'unknown scene {key!r} (prologue / main)')
        build(key)


if __name__ == '__main__':
    main()
