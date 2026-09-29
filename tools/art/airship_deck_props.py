"""序章の飛行船 (FirstTouchDownMainIsLandScene の AirShip) の甲板に小物を置く。

    python tools/art/airship_deck_props.py      # 何度流してもよい (AirShip/DeckProps は1組だけになる)

- 樽と木箱は、同じシーンの壊せる Burrel / WoodBox (DestructibleObject + RigidBody) を写す。壊れる演出はそのまま、
  ドロップは無し (お金やアイテムはワールド座標に出るので、航行中に割ると船に置いていかれる)。
- 航行中は RigidBody を Kinematic にして甲板に留める。着いたら AboardAirShipMovie::LoosenDeckProps が
  シーンコンテキストの airShipDeckProps_ (= DeckProps) の子孫を Dynamic にして、押したり転がしたりできるようにする。
  Dynamic のまま船の子にすると、親の移動で瞬間移動したうえ甲板の摩擦でも運ばれ、二重に動いて滑っていく。
- 砲弾は見た目だけ (ModelRenderer のみ。当たり判定は船の Body に入らないよう付けない)。
- 座標は船のローカル (船の scale 0.6 の中)。主甲板の上面は y = -60 (FloorCollider の一番大きい箱)。
  船首は -x、船尾楼 (+x) の手前 -z 側 (x 2..41) は船尾楼への階段なので空ける。
  出航時のプレイヤー (PlayerSpawnPosition -> PlayerFirstMoveTargetPos) の通り道 x -40..5, z -3..16 も空ける。
  着岸時のタラップは +z 舷の x 9 あたりに掛かるので、そこへ降りる道 x -3..21, z 0..38 も空ける。
- 序章のシーンは ColliderBase が v5 のままで、写し元もその並びなので版はいじらない。
- DeckProps の guid は GameManage.scene の FirstTouchDownMainIsLandSceneContext.airShipDeckProps_ が指すので、
  2回目以降は前の guid を使い回す。
"""
import copy
import math
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.blob import Ver  # noqa: E402
from tools.common.cereal_json import Num, OrderedObj, dumps, loads, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, model, reader, validate, writer  # noqa: E402

from game_over_prefab import asset_guid, check, let_writer_place_versions  # noqa: E402
from grassland_nature_scatter import StrayVersionStripper, bake_world_matrices, first_versions, walk  # noqa: E402

SCENE = REPO / 'Assets' / 'Scene' / 'FirstTouchDownMainIsLandScene.scene'
CANNONBALL = REPO / 'Assets' / 'Art' / 'Models' / 'Prop' / 'CannonBullet' / 'CannonBullet.mv1.meta'
ROOT_NAME = 'DeckProps'
DECK_Y = -60.0
KINEMATIC = 1

# 写し元: (名前, ワールドの scale を船の中へ直した scale, ParticlePos の高さ (モデルの単位))
TEMPLATES = {
    'Barrel': ('Burrel', 0.126 / 0.6, 45.0),
    'Crate': ('WoodBox', 0.132 / 0.6, 48.0),
}
# (グループ, 種類, x, z, 向き deg, 積む段 (0 = 甲板), 横倒し)
PROPS = [
    # 島へ運ぶ荷。木箱の2段積みは船首寄りの +z 側 (砲弾の山の船首側)、残りは降り口を挟んで両脇に置く
    ('Cargo', 'Crate', -44.0, 24.0, 10.0, 0, False),
    ('Cargo', 'Crate', -44.0, 24.0, 35.0, 1, False),
    ('Cargo', 'Crate', 24.0, -13.0, -5.0, 0, False),
    ('Cargo', 'Barrel', 28.0, 27.0, 0.0, 0, False),
    ('Cargo', 'Barrel', -11.0, 30.0, 40.0, 0, False),
    # 船首寄りの -z 側: 水や酒の樽。1つは横倒し
    ('Barrels', 'Barrel', -40.0, -26.0, 0.0, 0, False),
    ('Barrels', 'Barrel', -28.0, -27.0, 20.0, 0, False),
    ('Barrels', 'Barrel', -34.0, -16.0, 70.0, 0, False),
    ('Barrels', 'Barrel', -46.0, -10.0, 80.0, 0, True),
]
# 砲弾の山: 舷側の大砲の脇 (+z)。2x3 の上に 2 個。(dx, 段, dz) は船のローカル
# NOTE: CannonBullet.prefab の 0.2 は撃つ弾の大きさで、積むと樽より大きい。人の拳2つ分ほどにする
CANNONBALL_SCALE = 0.03 / 0.6
CANNONBALL_PILE = (-24.0, 29.0)
CANNONBALL_RADIUS = 1.7
CANNONBALL_OFFSETS = [(dx, 0, dz) for dx in (-3.4, 0.0, 3.4) for dz in (-1.7, 1.7)] + [(-1.7, 1, 0.0), (1.7, 1, 0.0)]


def find_named(scene, name, component_suffix):
    for root in scene.roots:
        for node in walk(root):
            if node.name == name and any(c.fqn.endswith(component_suffix) for c in node.components):
                return node
    raise SystemExit(f'{SCENE.name}: no {name} with {component_suffix} - nothing written')


def body_of(comp):
    slot = comp.data.get('value0') if comp.fqn.endswith('Collider') else None
    data = slot if slot is not None else comp.data
    return data.body if isinstance(data, Ver) else data


def yaw(deg, lay_down=False):
    q = (0.0, math.sin(math.radians(deg) / 2), 0.0, math.cos(math.radians(deg) / 2))
    if not lay_down:
        return q
    # NOTE: 横倒しは z 軸まわりに 90 度 (樽の芯が x 向き) のあと向きを付ける
    roll = (0.0, 0.0, math.sin(math.pi / 4), math.cos(math.pi / 4))
    x1, y1, z1, w1 = q
    x2, y2, z2, w2 = roll
    return (w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2,
            w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2,
            w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2,
            w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2)


def set_trs(node, pos, rot=(0.0, 0.0, 0.0, 1.0), scale=1.0):
    node.transform.local_pos = edits._vec3_from_floats(tuple(float(v) for v in pos))
    node.transform.local_rot = edits._quat_from_floats(tuple(float(v) for v in rot))
    node.transform.local_scale = edits._vec3_from_floats((float(scale),) * 3)


def copy_prop(template, kind, parent, x, z, deg, tier, lay_down):
    _, scale, particle_y = TEMPLATES[kind]
    node = copy.deepcopy(template)
    remap = {}
    edits._remint_guids(node, remap)
    edits._remap_guid_references(node, remap)
    node.name = kind
    node.is_active = False
    parent.transform.children.append(node)

    size = 94.0 * scale if kind == 'Crate' else 90.0 * scale
    y = DECK_Y + tier * size
    if lay_down:
        # 横倒しの樽は半径の分だけ上げ、原点 (底) を横へずらす
        y = DECK_Y + 30.0 * scale
    set_trs(node, (x, y + 0.3, z), yaw(deg, lay_down), scale)

    for comp in node.components:
        body = body_of(comp)
        if comp.fqn.endswith('::RigidBody'):
            body['motionType_'] = Num.of_int(KINEMATIC)
        elif comp.fqn.endswith('Collider') and 'emotionType_' in body:
            body['emotionType_'] = Num.of_int(KINEMATIC)
        elif comp.fqn.endswith('::DestructibleObject'):
            edits._set_field_guid(body['dropTable_'], edits.EMPTY_GUID)
        elif comp.fqn.endswith('::ModelRenderer'):
            # NOTE: 船 (useFixedInterpolation_) と同じ補間にしないと、航行中に甲板の上でずれて見える
            body['useFixedInterpolation_'] = True

    for child in node.transform.children:
        if child.name == 'ParticlePos':
            child.is_active = False
            set_trs(child, (0.0, particle_y, 0.0))
    return node


def cannonball(template, parent, pos):
    node = copy.deepcopy(template)
    remap = {}
    edits._remint_guids(node, remap)
    node.name = 'Cannonball'
    node.is_active = False
    node.components = [c for c in node.components if c.fqn.endswith('::ModelRenderer')]
    node.transform.children = []
    renderer = body_of(node.components[0])
    edits._set_field_guid(renderer['mv1File_'], asset_guid(CANNONBALL))
    renderer['useFixedInterpolation_'] = True
    set_trs(node, pos, scale=CANNONBALL_SCALE)
    parent.transform.children.append(node)


def main():
    scene = reader.read_scene_file(SCENE)
    ship = next((r for r in scene.roots if r.name == 'AirShip'), None)
    if ship is None:
        raise SystemExit(f'{SCENE.name}: no AirShip root - nothing written')

    old = next((c for c in ship.transform.children if c.name == ROOT_NAME), None)
    guid = old.guid if old else None
    ship.transform.children = [c for c in ship.transform.children if c.name != ROOT_NAME]
    templates = {kind: find_named(scene, name, '::DestructibleObject') for kind, (name, _, _) in TEMPLATES.items()}

    root = edits.add_gameobject(scene, parent=ship.guid, name=ROOT_NAME, is_active=False, guid=guid)
    groups = {}
    for group, kind, x, z, deg, tier, lay_down in PROPS:
        if group not in groups:
            groups[group] = edits.add_gameobject(scene, parent=root.guid, name=group, is_active=False)
        copy_prop(templates[kind], kind, groups[group], x, z, deg, tier, lay_down)

    pile = edits.add_gameobject(scene, parent=root.guid, name='Cannonballs', is_active=False)
    px, pz = CANNONBALL_PILE
    for dx, tier, dz in CANNONBALL_OFFSETS:
        y = DECK_Y + CANNONBALL_RADIUS + tier * CANNONBALL_RADIUS * 1.4
        cannonball(templates['Barrel'], pile, (px + dx, y, pz + dz))

    for node in walk(root):
        for comp in node.components:
            let_writer_place_versions(comp.data)
    bake_world_matrices(root, edits._node_local_trs(ship))

    text = writer.write_scene(scene)
    tree = loads(text)
    stripper = StrayVersionStripper(catalog_mod.load(), f'/gameObject_{scene.roots.index(ship)}/')
    stripper.run(tree, '')
    text = dumps(tree)
    versions = first_versions(text)
    if versions.get('ColliderBase') != 5:
        raise SystemExit('ColliderBase: first occurrence is not v5 - nothing written')
    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(SCENE)
    print(f'wrote {SCENE.relative_to(REPO)}  {len(PROPS)} props + {len(CANNONBALL_OFFSETS)} cannonballs')
    print(f'DeckProps guid: {root.guid}')


if __name__ == '__main__':
    main()
