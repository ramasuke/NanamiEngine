"""砂漠ステージの砂嵐 (GamePlay::Weather::Sandstorm) と、転がる枝玉 (GamePlay::Prop::Tumbleweed) を置く。

    python tools/art/desert_sandstorm.py prefabs   # SandstormParticle.prefab を作り、Tumbleweed.prefab に物理を付け直す
    python tools/art/desert_sandstorm.py place     # DesertScene の Sandstorm / Tumbleweeds ルートを置き直す

- 砂嵐は凪と交互に来る。長さ・霧・押す強さは Sandstorm コンポーネントの値 (エディタで調整する)。
  風向きはシーンの WindZone (草と木が見ているのと同じ向き)。
- 砂のエフェクトは tktk2/sandStorm.efkefc。Sandstorm が砂嵐の間だけ有効にして、カメラの位置へ動かす。
- Tumbleweed.prefab は Dynamic な RigidBody と、Sensor でない球の当たり判定 (LargeBarrelBombPlaced の写し) を持つ。
  以前の Sensor の BoxCollider は外す (先頭のコンポーネントなので、版キーは VersionFixer が付け直す)。
- desert_scene.py でシーンを作り直した後は place を流し直す。
"""
import argparse
import copy
import math
import random
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, dumps, loads, read_text, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, meta as scene_meta, model, reader, validate, writer  # noqa: E402

from camp_people import VersionFixer  # noqa: E402
from game_over_prefab import asset_guid, check, let_writer_place_versions  # noqa: E402
from grassland_nature_scatter import bake_world_matrices, first_versions, quat_axis, walk  # noqa: E402

PREFAB_DIR = REPO / 'Assets' / 'Prefab'
TUMBLEWEED = PREFAB_DIR / 'Prop' / 'Desert' / 'Tumbleweed.prefab'
PARTICLE = PREFAB_DIR / 'Particle' / 'SandstormParticle.prefab'
BARREL = PREFAB_DIR / 'Item' / 'LargeBarrelBombPlaced.prefab'
GUST = PREFAB_DIR / 'Particle' / 'StormWindGustParticle.prefab'
SAND_EFFECT = REPO / 'Assets' / 'Art' / 'Effect' / 'tktk2' / 'sandStorm.efkefc.meta'

SANDSTORM_ROOT = 'Sandstorm'
TUMBLEWEED_ROOT = 'Tumbleweeds'

# 球の当たり判定。Model (scale 0.08) の見た目に合わせた
SPHERE_RADIUS = 3.5
SPHERE_OFFSET = (0.0, 3.9, 0.0)
SPHERE_FRICTION = 0.8
TUMBLEWEED_MASS = 1.0
PARTICLE_SCALE = 5.0
PARTICLE_LOOP_SECS = 2.0

# 枝玉の置き場所 (x, z)。desert_terrain.py の CAMP (420, 1000) / OASIS (600, 1040) / BONES (1100, 900) /
# PLAZA (880, 420) / WORM_SEA (430, 640) の間の開けた砂地。入口 (380, 1200) の近くには置かない
TUMBLEWEED_SPOTS = [
    (520.0, 880.0), (700.0, 880.0), (820.0, 760.0), (940.0, 700.0),
    (620.0, 740.0), (760.0, 620.0), (560.0, 540.0), (1000.0, 560.0),
    (700.0, 1120.0), (900.0, 1000.0), (1180.0, 700.0), (480.0, 780.0),
    (1080.0, 1080.0), (640.0, 440.0), (1200.0, 520.0), (860.0, 1150.0),
]
MAX_SLOPE_DEG = 25.0
SEED = 20260926


def fqn_leaf(comp):
    return comp.fqn.rsplit('::', 1)[-1]


def base_body(comp):
    base = comp.data['value0']
    return base.body if hasattr(base, 'body') else base


def set_vec3(obj, values):
    for i, v in enumerate(values):
        obj[f'value{i}'] = Num.of_float(float(v))


def header_defaults(header):
    """ヘッダーの [[serialize]] な float / Color32 の初期値。new_component は 0 で埋めるので、C++ と同じ値を入れ直す"""
    text = (REPO / header).read_text(encoding='utf-8-sig')
    out = {}
    for m in re.finditer(r'\[\[serialize\(\d+\)\]\]\s+float\s+(\w+)\s*=\s*(-?[\d.]+)f?\s*;', text):
        out[m[1]] = float(m[2])
    for m in re.finditer(r'\[\[serialize\(\d+\)\]\]\s+NanamiEngine::Color32\s+(\w+)\s*=\s*'
                         r'NanamiEngine::Color32\(\s*(\d+),\s*(\d+),\s*(\d+)\)\s*;', text):
        out[m[1]] = [int(m[2]), int(m[3]), int(m[4])]
    return out


def new_component_with_defaults(cat, fqn):
    entry = cat.component_by_fqn(fqn)
    comp = edits.new_component(entry, cat=cat)
    for key, value in header_defaults(entry['header']).items():
        if key not in comp.data:
            raise SystemExit(f'{fqn}: {key} is not in the catalog (regen-catalog?)')
        if isinstance(value, list):
            edits._set_color32(comp.data[key], value)
        else:
            comp.data[key] = Num.of_float(value)
    return comp


def component_from(source, leaf):
    comp = copy.deepcopy(next(c for c in source.root.components if fqn_leaf(c) == leaf))
    model.set_component_guid(comp, edits.mint_guid())
    return comp


def write_fixed(path, text, sources, label):
    """写したコンポーネントの版キーを、このファイルでの初出/2回目に合わせてから書く"""
    versions = {}
    for source in sources:
        for key, value in first_versions(read_text(source)).items():
            if value is not None:
                versions.setdefault(key, value)
    tree = loads(text)
    fixer = VersionFixer(catalog_mod.load(), '', versions)
    fixer.run(tree, '')
    text = dumps(tree)
    print(f'  versions: added {sorted(set(fixer.added))}, stripped {fixer.stripped} repeat key(s)')
    return text


def save_prefab(prefab, path, sources):
    for node in walk(prefab.root):
        for comp in node.components:
            let_writer_place_versions(comp.data)
    bake_world_matrices(prefab.root)
    text = write_fixed(path, writer.write_prefab(prefab), sources, path.name)
    check(text, [], path.name)
    path.write_bytes(to_file_bytes(text))
    reader.read_prefab_file(path)

    meta = Path(str(path) + '.meta')
    if meta.exists():
        guid = asset_guid(meta)
    else:
        guid = scene_meta.mint_guid().upper()
        content_path = scene_meta.content_path_for(scene_meta.PREFAB_SPEC, path.stem, path.parent, REPO)
        scene_meta.write_meta(scene_meta.PREFAB_SPEC, meta, path.stem, guid, content_path)
    print(f'wrote {path.relative_to(REPO)}  (asset guid {guid})')


def build_particle():
    # 作り直すときは GUID を保つ
    prefab = reader.read_prefab_file(PARTICLE) if PARTICLE.exists() else edits.copy_prefab(reader.read_prefab_file(GUST))
    root = prefab.root
    root.name = 'SandstormParticle'
    root.transform.local_scale = edits._vec3_from_floats((PARTICLE_SCALE,) * 3)
    comp = root.components[0]
    edits._set_field_guid(comp.data['particleFile_'], asset_guid(SAND_EFFECT))
    comp.data['playingDuration_secs_'] = Num.of_float(PARTICLE_LOOP_SECS)
    comp.data['playMode_'] = Num.of_int(0)  # Loop
    # 凪の間は再生しない。Sandstorm が砂嵐の間だけ有効にする
    model.set_component_enabled(comp, False)
    save_prefab(prefab, PARTICLE, [GUST])


def build_tumbleweed():
    prefab = reader.read_prefab_file(TUMBLEWEED)
    barrel = reader.read_prefab_file(BARREL)
    root = prefab.root
    cat = catalog_mod.load()

    sphere = component_from(barrel, 'SphereCollider')
    base = base_body(sphere)
    set_vec3(base['offset_'], SPHERE_OFFSET)
    base['isSensor_'] = False
    base['friction_'] = Num.of_float(SPHERE_FRICTION)
    sphere.data['radius_'] = Num.of_float(SPHERE_RADIUS)

    body = component_from(barrel, 'RigidBody')
    body.data['motionType_'] = Num.of_int(2)  # Dynamic
    body.data['mass_'] = Num.of_float(TUMBLEWEED_MASS)
    body.data['isGravity_'] = True

    roller = new_component_with_defaults(cat, 'GamePlay::Prop::Tumbleweed')
    roller.data['radius_'] = Num.of_float(SPHERE_RADIUS)

    root.components = [sphere, body, roller]
    save_prefab(prefab, TUMBLEWEED, [BARREL, TUMBLEWEED])


def prefabs():
    build_particle()
    build_tumbleweed()


def pick_spots(t):
    rng = random.Random(SEED)
    spots = []
    for x, z in TUMBLEWEED_SPOTS:
        slope = t.slope(x, z)
        if slope > MAX_SLOPE_DEG:
            print(f'  skip ({x:.0f}, {z:.0f}): slope {slope:.1f} deg')
            continue
        spots.append((x, z, rng.uniform(0.0, 2.0 * math.pi)))
    return spots


def place():
    from desert_scene import SCENE, Terrain

    t = Terrain()
    cat = catalog_mod.load()
    scene = reader.read_scene_file(SCENE)
    scene.roots = [r for r in scene.roots if r.name not in (SANDSTORM_ROOT, TUMBLEWEED_ROOT)]

    storm = edits.new_gameobject(SANDSTORM_ROOT)
    scene.roots.append(storm)
    particle = edits.instantiate_prefab(scene, reader.read_prefab_file(PARTICLE), parent=storm.guid)
    service = new_component_with_defaults(cat, 'GamePlay::Weather::Sandstorm')
    edits._set_field_guid(service.data['sandParticle_'], model.find_component_guid(particle.components[0]))
    storm.components.append(service)

    weeds = edits.new_gameobject(TUMBLEWEED_ROOT)
    scene.roots.append(weeds)
    tumbleweed = reader.read_prefab_file(TUMBLEWEED)
    for i, (x, z, yaw) in enumerate(pick_spots(t)):
        node = edits.instantiate_prefab(scene, tumbleweed, parent=weeds.guid)
        node.name = f'Tumbleweed{i + 1:02d}'
        ground = t.height(x, z)
        node.transform.local_pos = edits._vec3_from_floats((x, ground + 0.5, z))
        node.transform.local_rot = edits._quat_from_floats(quat_axis((0.0, 1.0, 0.0), yaw))
        print(f'  {node.name}  ({x:7.1f}, {ground:6.1f}, {z:7.1f})')

    for root in (storm, weeds):
        for n in walk(root):
            for comp in n.components:
                let_writer_place_versions(comp.data)
        bake_world_matrices(root)

    text = writer.write_scene(scene)
    tree = loads(text)
    versions = {}
    for source in (SCENE, PARTICLE, TUMBLEWEED):
        for key, value in first_versions(read_text(source)).items():
            if value is not None:
                versions.setdefault(key, value)
    first = len(scene.roots) - 2
    for index in (first, first + 1):
        fixer = VersionFixer(cat, f'/gameObject_{index}/', versions)
        fixer.run(tree, '')
        print(f'  versions: added {sorted(set(fixer.added))}, stripped {fixer.stripped} repeat key(s)')
        tree = loads(dumps(tree))
    text = dumps(tree)
    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(SCENE)
    print(f'wrote {SCENE.relative_to(REPO)}  (Sandstorm + {len(weeds.transform.children)} tumbleweeds)')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('step', choices=['prefabs', 'place'])
    a = ap.parse_args()
    {'prefabs': prefabs, 'place': place}[a.step]()


if __name__ == '__main__':
    main()
