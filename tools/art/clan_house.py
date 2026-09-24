"""Build ClanHouse.prefab: the grassland hunter clan's home on the fountain island (Facility::ClanHouse).

    python tools/art/story_npcs.py --only clan     # 先に女狩人の BT と会話を作る
    python tools/art/clan_house.py                 # ClanHouse.prefab を組む
    python tools/art/restoration_sites.py          # 噴水の島に RestorationSite_ClanHouse を置く

建った姿 = 草原の野営地と同じ Settlement の置き物 (獣皮のテント・焚き火・干し棚・トーテム) と、女狩人 (ClanHuntress)。
RestorationGate の restoredPrefab_ に入れて、建てたとき(と下見中)だけ生成する。NPC は Dynamic の RigidBody なので、
シーンに置いたまま隠すと、噴水の島が戻る前は床が無くて落ちていく。
女狩人は拠点の島の仲介人 (CharacterBrokerNpc) の複製に、草原の CampPeopleHuntress の見た目を載せる (story_npcs.py の newcomers と同じ)。
話しかけると仲間を選ばせる (OpenCharacterSelect)。展示台は MainIslandScene の CharacterPodium を家の前に置いてある。
"""
import copy
import math
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, dumps, loads, read_text, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, reader, validate, writer  # noqa: E402
from tools.scene import meta as scene_meta  # noqa: E402

from camp_people import BT_DIR, ICON_WORLD_SCALE, ICON_X, VersionFixer, asset_guid, set_vec3  # noqa: E402
from event_board_prefab import bake_rotated_world_matrices  # noqa: E402
from game_over_prefab import check, let_writer_place_versions, new_prefab  # noqa: E402
from grassland_nature_scatter import first_versions, quat_axis  # noqa: E402
from grassland_nature_scatter import walk as scene_walk  # noqa: E402
from restoration_sites import SETTLEMENT_DIR, SITE_DIR  # noqa: E402

MAIN_ISLAND = REPO / 'Assets' / 'Scene' / 'MainIslandScene.scene'
GRASSLAND = REPO / 'Assets' / 'Scene' / 'GrassLandScene.scene'
TEMPLATE_NPC = 'CharacterBrokerNpc'
SOURCE_NPC = 'CampPeopleHuntress'
NPC_NAME = 'ClanHuntress'
NAME = 'ClanHouse'

# (Settlement の prefab, 家の root からの位置, yaw 度)。家の正面 (入口) は -z
PROPS = [
    ('HideTent', (0.0, 0.0, 0.0), 0.0),
    ('Campfire', (14.0, 0.0, -20.0), 0.0),
    ('DryingRack', (-28.0, 0.0, 10.0), 80.0),
    ('Totem', (16.0, 0.0, 10.0), -20.0),
]
# 女狩人は入口の左で焚き火の側に立ち、正面 (-z) を向く。Dynamic なので少し上から落とす
NPC_POS = (-10.0, 2.0, -24.0)
NPC_YAW_DEGREES = 0.0


def field_guid(blob):
    import re
    return re.findall(r'[0-9A-F]{8}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{12}', repr(blob))[0]


def find(scene, name):
    node = next((n for r in scene.roots for n in scene_walk(r) if n.name == name), None)
    if node is None:
        raise SystemExit(f'{name} not found')
    return node


def world_scale(scene, name):
    def rec(node, scale):
        scale *= float(node.transform.local_scale.x.value)
        if node.name == name:
            return scale
        for child in node.transform.children:
            found = rec(child, scale)
            if found is not None:
                return found
        return None
    return next(s for s in (rec(r, 1.0) for r in scene.roots) if s is not None)


def yaw(degrees):
    return edits._quat_from_floats(quat_axis((0.0, 1.0, 0.0), math.radians(degrees)))


def build_npc(template, source, scale, bt_guid):
    src = {c.fqn.rsplit('::', 1)[-1]: c.data for c in source.components}
    src_icon = next(c for c in source.transform.children if c.name == 'BillBoardNpcChatIcon')

    node = copy.deepcopy(template)
    remap = {}
    edits._remint_guids(node, remap)
    edits._remap_guid_references(node, remap)
    node.kind = 'scene'
    node.name = NPC_NAME
    node.transform.children = [c for c in node.transform.children if c.name == 'BillBoardNpcChatIcon']
    node.transform.local_pos = edits._vec3_from_floats(NPC_POS)
    node.transform.local_rot = yaw(NPC_YAW_DEGREES)
    node.transform.local_scale = edits._vec3_from_floats((scale,) * 3)

    for comp in node.components:
        leaf = comp.fqn.rsplit('::', 1)[-1]
        if leaf == 'FriendlyNpc':
            comp.data['name_'] = src['FriendlyNpc']['name_']
            edits._set_field_guid(comp.data['friendlyNpcBehaviourFile_'], bt_guid)
        elif leaf == 'ModelRenderer':
            edits._set_field_guid(comp.data['mv1File_'], field_guid(src['ModelRenderer']['mv1File_']))
        elif leaf == 'Animator':
            edits._set_field_guid(comp.data['animationTreeFile_'], field_guid(src['Animator']['animationTreeFile_']))
        elif leaf == 'CapsuleCollider':
            base = comp.data['value0']
            base = base.body if hasattr(base, 'body') else base
            src_base = src['CapsuleCollider']['value0']
            src_base = src_base.body if hasattr(src_base, 'body') else src_base
            set_vec3(base['offset_'], [float(src_base['offset_'][k].value) for k in ('value0', 'value1', 'value2')])
            comp.data['radius_'] = Num.of_float(float(src['CapsuleCollider']['radius_'].value))
            comp.data['height_'] = Num.of_float(float(src['CapsuleCollider']['height_'].value))
        elif leaf == 'RigidBody':
            if int(comp.data['motionType_'].value) != 2 or int(comp.data['constraints_'].value) != 61:
                raise SystemExit('template RigidBody is no longer Dynamic + constraints 61')

    icon = node.transform.children[0]
    icon.transform.local_pos = edits._vec3_from_floats((ICON_X, float(src_icon.transform.local_pos.y.value), 0.0))
    icon.transform.local_scale = edits._vec3_from_floats((ICON_WORLD_SCALE / scale,) * 3)
    return node


def main():
    bt_guid = asset_guid(BT_DIR / f'{NPC_NAME}.friendBehaviourData.meta')
    main_island = reader.read_scene_file(MAIN_ISLAND)
    grassland = reader.read_scene_file(GRASSLAND)

    prefab = new_prefab(NAME)
    root = prefab.root
    for name, pos, degrees in PROPS:
        node = edits.instantiate_prefab(prefab, reader.read_prefab_file(SETTLEMENT_DIR / f'{name}.prefab'), parent=root.guid)
        node.transform.local_pos = edits._vec3_from_floats(pos)
        node.transform.local_rot = yaw(degrees)

    npc = build_npc(find(main_island, TEMPLATE_NPC), find(grassland, SOURCE_NPC), world_scale(grassland, SOURCE_NPC), bt_guid)
    root.transform.children.append(npc)

    for node in scene_walk(root):
        for comp in node.components:
            let_writer_place_versions(comp.data)
    bake_rotated_world_matrices(root)
    text = writer.write_prefab(prefab)
    # NOTE: 写してきた置き物と NPC の版キーを、この prefab の中での初出/2回目に合わせる
    tree = loads(text)
    fixer = VersionFixer(catalog_mod.load(), '/', first_versions(read_text(MAIN_ISLAND)))
    fixer.run(tree, '')
    text = dumps(tree)
    print(f'  versions: added {sorted(set(fixer.added))}, stripped {fixer.stripped} repeat key(s)')

    path = SITE_DIR / f'{NAME}.prefab'
    check(text, validate.validate_prefab(prefab), path.name)
    path.write_bytes(to_file_bytes(text))
    meta = Path(str(path) + '.meta')
    if not meta.exists():
        content_path = scene_meta.content_path_for(scene_meta.PREFAB_SPEC, NAME, SITE_DIR, REPO)
        scene_meta.write_meta(scene_meta.PREFAB_SPEC, meta, NAME, scene_meta.mint_guid().upper(), content_path)
    reader.read_prefab_file(path)
    print(f'wrote {path.relative_to(REPO)}  (asset guid {asset_guid(meta)})')


if __name__ == '__main__':
    main()
