"""骸竜の砂嵐のギミックで叩く光の心臓 (GamePlay::Prop::StormHeart) を、LightFloatingStone に付ける。

    python tools/art/desert_boss_heart.py

- Stone の子に HeartHitBox を作り、Sensor でない球の当たり判定・Static な RigidBody・StormHeart を付ける
  (球と RigidBody は LargeBarrelBombPlaced の写し)。StormHeart は骸竜の砂嵐の外では HeartHitBox ごと地面の下へ退ける。
- LightFloatingStone.prefab と、DesertScene の LightFloatingStone (prefab の写し) の両方に付ける。何度流しても同じ
  (前の HeartHitBox は消して付け直す)。desert_scene.py でシーンを作り直したときは prefab から入るので流さなくてよい。
- 骸竜の側 (AnimTree / BT) は desert_enemies.py (--storm-only で今のファイルに足す)。
"""
import copy
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, dumps, loads, read_text, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, reader, validate, writer  # noqa: E402

from camp_people import VersionFixer  # noqa: E402
from desert_sandstorm import base_body, component_from, new_component_with_defaults, save_prefab, set_vec3  # noqa: E402
from game_over_prefab import asset_guid, check, let_writer_place_versions  # noqa: E402
from grassland_nature_scatter import bake_world_matrices, first_versions, walk  # noqa: E402

PREFAB_DIR = REPO / 'Assets' / 'Prefab'
LIGHT_STONE = PREFAB_DIR / 'Prop' / 'Story' / 'LightFloatingStone.prefab'
BARREL = PREFAB_DIR / 'Item' / 'LargeBarrelBombPlaced.prefab'
SCENE = REPO / 'Assets' / 'Scene' / 'DesertScene.scene'

STONE_ROOT = 'LightFloatingStone'
STONE = 'Stone'
HITBOX = 'HeartHitBox'

# 心臓の水晶は高さ 17 (約 2.1 m)。剣が届きやすいよう、見た目より少し大きい球にする
SPHERE_RADIUS = 8.0
SPHERE_OFFSET = (0.0, 9.0, 0.0)
EFFECT_OFFSET = (0.0, 9.0, 0.0)
REQUIRED_HITS = 3

HIT_PARTICLE = PREFAB_DIR / 'Particle' / 'ItemPickupSparkle.prefab.meta'
SHAKEN_PARTICLE = PREFAB_DIR / 'Particle' / 'LightStoneLiftOff.prefab.meta'
HIT_SOUND = REPO / 'Assets' / 'Audio' / 'Physics' / 'HolyGlass.mp3.meta'
SHAKEN_SOUND = REPO / 'Assets' / 'Audio' / 'Magic' / 'RockWall_Crumble.mp3.meta'


def find_child(node, name):
    found = next((c for c in node.transform.children if c.name == name), None)
    if found is None:
        raise SystemExit(f'{node.name}: no child named {name!r}')
    return found


def build_hitbox(cat):
    barrel = reader.read_prefab_file(BARREL)

    sphere = component_from(barrel, 'SphereCollider')
    base = base_body(sphere)
    set_vec3(base['offset_'], SPHERE_OFFSET)
    base['isSensor_'] = False
    sphere.data['radius_'] = Num.of_float(SPHERE_RADIUS)

    body = component_from(barrel, 'RigidBody')
    body.data['motionType_'] = Num.of_int(0)  # Static
    body.data['isGravity_'] = False

    heart = new_component_with_defaults(cat, 'GamePlay::Prop::StormHeart')
    heart.data['requiredHits_'] = Num.of_int(REQUIRED_HITS)
    set_vec3(heart.data['effectOffset_'], EFFECT_OFFSET)
    for key, meta in (('hitParticle_', HIT_PARTICLE), ('shakenParticle_', SHAKEN_PARTICLE),
                      ('hitSound_', HIT_SOUND), ('shakenSound_', SHAKEN_SOUND)):
        edits._set_field_guid(heart.data[key], asset_guid(meta))

    node = edits.new_gameobject(HITBOX)
    node.components = [sphere, body, heart]
    return node


def attach(stone_root, hitbox):
    stone = find_child(stone_root, STONE)
    stone.transform.children = [c for c in stone.transform.children if c.name != HITBOX]
    node = copy.deepcopy(hitbox)
    edits._remint_guids(node, {})
    stone.transform.children.append(node)
    for n in walk(node):
        for comp in n.components:
            let_writer_place_versions(comp.data)
    return node


def update_prefab(cat, hitbox):
    prefab = reader.read_prefab_file(LIGHT_STONE)
    attach(prefab.root, hitbox)
    save_prefab(prefab, LIGHT_STONE, [BARREL, LIGHT_STONE])


def update_scene(cat, hitbox):
    scene = reader.read_scene_file(SCENE)
    index, root = next((i, r) for i, r in enumerate(scene.roots) if r.name == STONE_ROOT)
    attach(root, hitbox)
    bake_world_matrices(root)

    tree = loads(writer.write_scene(scene))
    versions = {}
    for source in (SCENE, BARREL, LIGHT_STONE):
        for key, value in first_versions(read_text(source)).items():
            if value is not None:
                versions.setdefault(key, value)
    fixer = VersionFixer(cat, f'/gameObject_{index}/', versions)
    fixer.run(tree, '')
    print(f'  versions: added {sorted(set(fixer.added))}, stripped {fixer.stripped} repeat key(s)')
    text = dumps(tree)
    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(SCENE)
    print(f'wrote {SCENE.relative_to(REPO)}  ({STONE_ROOT}/{STONE}/{HITBOX})')


def main():
    cat = catalog_mod.load()
    hitbox = build_hitbox(cat)
    update_prefab(cat, hitbox)
    update_scene(cat, hitbox)


if __name__ == '__main__':
    main()
