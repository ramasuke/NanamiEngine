"""Build the island-restoration sites and place them in MainIslandScene.

    python tools/art/restoration_sites.py            # 施設ごとの prefab を組み、MainIslandScene に置き直す
    python tools/art/restoration_sites.py --no-place # prefab だけ組む

施設1つ = RestorationSite_<Name>.prefab (Assets/Prefab/Prop/Restoration/)。root に RestorationGate、子に
下見のカメラ(PreviewCamera → PreviewTarget を見る)を持つ。建った姿は子に置かず、RestorationGate の restoredPrefab_ に
Settlement の prefab を入れて、建てた時(と下見中)だけ生成する(隠しただけではコライダーが当たり続けるため)。
施設は壊れた物を直すのではなく新しく建てるので、建てる前は何も置かない(brokenObject_ は空)。
掲示板の「復興」で施設を選ぶと、そのカメラへ寄って建った姿を出す(RestorationGate::BeginPreview)。
置き直すときは前の RestorationSite_* をシーンから消してから置く。
建った姿のモデルは仮 (既存の Settlement の prefab を借りている)。
"""
import argparse
import math
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, OrderedObj, to_file_bytes  # noqa: E402
from tools.scene import edits, reader, validate, writer  # noqa: E402

from event_board_prefab import (  # noqa: E402
    MAIN_ISLAND_SCENE, bake_rotated_world_matrices, first_versions, strip_repeat_versions, to_collider_base_v5)
from game_over_prefab import Builder, asset_guid, check, guid_of, let_writer_place_versions, new_prefab  # noqa: E402
from tools.scene import meta as scene_meta  # noqa: E402
from grassland_nature_scatter import quat_axis  # noqa: E402

SITE_DIR = REPO / 'Assets' / 'Prefab' / 'Prop' / 'Restoration'
SETTLEMENT_DIR = REPO / 'Assets' / 'Prefab' / 'Prop' / 'Settlement'
DISABLE_PRIORITY = -1
# 噴水の島の芝生 (エディタで高さを見て決めた)
CLAN_HOUSE_POS = (-205.0, 87.0, 432.0)
CLAN_HOUSE_YAW = -20.0
# 草地の高さ (AutoMCP で測った拠点の島の芝。docs 参照は memory の MainIsland heights)
GRASS_Y = 22.3

# facility は GameCore::Story::Facility の値。pos は MainIslandScene のワールド座標(島の上から見て +z が台座の側)
# NOTE: 仮置きだった4つ (船着き場・雑貨屋・狩人小屋・畑) は 2026-09-24 に削除した。
#   restored = Settlement の prefab 名、restored_prefab = それ以外の prefab。yaw は root の向き (度。家の正面 -z がどちらを向くか)
SITES = [
    # 一族の家: 噴水の島の芝生、階段を上がった先。入口は階段の方を向く (clan_house.py)
    dict(name='ClanHouse', facility=4, pos=CLAN_HOUSE_POS, yaw=CLAN_HOUSE_YAW,
         restored_prefab=SITE_DIR / 'ClanHouse.prefab', camera=(10.0, 30.0, -70.0)),
]
# 下見のカメラは広場の側(-z)から見下ろす。手前の建物が写り込まないよう高めから寄る(site の camera で向きを変えられる)
CAMERA_OFFSET = (0.0, 40.0, -50.0)
TARGET_OFFSET = (0.0, 8.0, 0.0)


def all_nodes(node):
    yield node
    for child in node.transform.children:
        yield from all_nodes(child)


def build_site(site):
    prefab = new_prefab(f"RestorationSite_{site['name']}")
    b = Builder(prefab)
    root = prefab.root

    target = edits.add_gameobject(prefab, parent=root.guid, name='PreviewTarget', pos=TARGET_OFFSET)
    camera_node = edits.add_gameobject(prefab, parent=root.guid, name='PreviewCamera',
                                       pos=site.get('camera', CAMERA_OFFSET))
    camera = b.component(camera_node, 'CineMachineVirtualCamera')
    camera.data['priority_'] = OrderedObj([('value', Num.of_int(DISABLE_PRIORITY))])
    look_at = b.component(camera_node, 'VirtualCameraLookAtBehaviour')
    b.field(look_at, 'target_', target.guid)

    gate = b.component(root, 'RestorationGate', facility_=site['facility'])
    restored = site.get('restored_prefab') or SETTLEMENT_DIR / f"{site['restored']}.prefab"
    b.field(gate, 'restoredPrefab_', asset_guid(Path(str(restored) + '.meta')))
    b.field(gate, 'previewCamera_', guid_of(camera))
    return save_site_prefab(prefab, f"RestorationSite_{site['name']}")


def save_site_prefab(prefab, name):
    """game_over_prefab.save_prefab と同じだが、既存の prefab を複製して入れたので版の重なりを確かめてから書く"""
    SITE_DIR.mkdir(parents=True, exist_ok=True)
    path = SITE_DIR / f'{name}.prefab'
    # NOTE: prepare の bake は回転を見ないので、置き物を回す一族の家のために回転込みで焼く
    for node in all_nodes(prefab.root):
        for comp in node.components:
            let_writer_place_versions(comp.data)
    bake_rotated_world_matrices(prefab.root)
    text = writer.write_prefab(prefab)
    check(text, validate.validate_prefab(prefab), path.name)
    path.write_bytes(to_file_bytes(text))

    meta = Path(str(path) + '.meta')
    if meta.exists():
        guid = asset_guid(meta)
    else:
        guid = scene_meta.mint_guid().upper()
        content_path = scene_meta.content_path_for(scene_meta.PREFAB_SPEC, name, SITE_DIR, REPO)
        scene_meta.write_meta(scene_meta.PREFAB_SPEC, meta, name, guid, content_path)
    reader.read_prefab_file(path)
    print(f'wrote {path.relative_to(REPO)}  (asset guid {guid})')
    return guid, path


def place_in_main_island(site_paths):
    scene = reader.read_scene_file(MAIN_ISLAND_SCENE)
    scene.roots = [r for r in scene.roots if not r.name.startswith('RestorationSite_')]
    collider_version = first_versions(MAIN_ISLAND_SCENE.read_text(encoding='utf-8')).get('ColliderBase')
    if collider_version not in (None, 5, 6):
        raise SystemExit(f'{MAIN_ISLAND_SCENE.name}: ColliderBase v{collider_version} is not handled')

    first_index = len(scene.roots)
    for site, path in zip(SITES, site_paths):
        node = edits.instantiate_prefab(scene, reader.read_prefab_file(path), parent=None)
        if collider_version == 5:
            for child in all_nodes(node):
                to_collider_base_v5(child)
        node.transform.local_pos = edits._vec3_from_floats(site['pos'])
        node.transform.local_rot = edits._quat_from_floats(quat_axis((0.0, 1.0, 0.0), math.radians(site.get('yaw', 0.0))))
        bake_rotated_world_matrices(node)

    text = writer.write_scene(scene)
    for i in range(first_index, len(scene.roots)):
        text = strip_repeat_versions(text, f'/gameObject_{i}/')
    check(text, validate.validate_scene(scene), MAIN_ISLAND_SCENE.name)
    MAIN_ISLAND_SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(MAIN_ISLAND_SCENE)
    for site in SITES:
        print(f"placed RestorationSite_{site['name']} at {site['pos']}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--no-place', action='store_true', help='prefab だけ組み、シーンには置かない')
    args = ap.parse_args()

    paths = [build_site(site)[1] for site in SITES]
    if not args.no_place:
        place_in_main_island(paths)


if __name__ == '__main__':
    main()
