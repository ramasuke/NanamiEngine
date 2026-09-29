"""Write the navigation targets and path grids into the main-scene contexts of Assets/Scene/GameManage.scene
(SceneContextBase v5: navigationGrid_ / navigationTargets_, read by GamePlay::Ui::NavigationPresenter through
GameCore::Navigation::INavigationSceneSource).

    python tools/art/navigation_targets.py [--dry-run]

CONTEXTS below is the source of truth; re-running replaces both members in every context. A target id must match a
targetId_ in tools/art/navigation_guide.py. The contexts are not in the tools.scene catalog, so the members are
inserted as cereal JSON: SceneContextBase's version is written once (on the first context in the file), every
context must then carry both members, and the first Field<HeightGridMap> / NavigationTargetEntry in file order
carries its own cereal_class_version.
"""
import argparse
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))
from tools.common import cereal_json as cj  # noqa: E402

SCENE = REPO_ROOT / 'Assets' / 'Scene' / 'GameManage.scene'
BASE_VERSION = 5
NO_GUID = '00000000-0000-0000-0000-000000000000'

GRID_DESERT = '4BEC7056-F622-4735-91E5-8D4FFEA411A2'
GRID_PROLOGUE = '32C78AA8-961C-406E-9861-7C55564C5B54'
GRID_GRASSLAND = '7F8280E5-F8B6-4B9D-9750-3290E536CB9C'
GRID_NEST = 'A6A6100D-8EDC-426F-AC8E-CF3DA1DE4BA5'
# 序章の格子の複製から始めた。拠点の島を開いてインスペクタの Bake Height Map で焼き直す
GRID_MAIN_ISLAND = 'D21C1310-E059-4C54-9159-67A3677D117D'

# 目印の高さはワールド単位（人の背丈がおよそ 19）。NPC は頭上の会話アイコン（約 22）の少し上に出す
CONTEXTS = {
    'GameCore::Scene::TitleSceneContext': (None, []),
    'GameCore::Scene::FirstTouchDownMainIsLandSceneContext': (GRID_PROLOGUE, [
        ('Instructor', '275F2AB2-1671-4B48-89D8-B8C2A8C7297D', 30.0),        # ActionInstructureNpc
        ('Cannon', 'D9D2F020-2197-4929-B08A-23247E0B86A8', 25.0),            # ForthIsland/Canon
    ]),
    'GameCore::Scene::MainIslandSceneContext': (GRID_MAIN_ISLAND, [
        ('Instructor', '69BF974C-B730-4E39-9BC6-F8FA9D36B901', 30.0),        # FirstIsland/ActionInstructureNpc
        ('Portal', 'FEB4046B-BF74-49AC-AA6C-5AAA356D40A7', 30.0),            # FirstIsland/IslandPedestial
        ('Board', '15FC5A6E-2E4B-4B63-9BD3-0587DB6D0CDB', 36.0),             # EventNoticeBoard
    ]),
    'GameCore::Scene::GrassLandSceneContext': (GRID_GRASSLAND, [
        ('GrassLandTyrant', '1AE61141-053C-4723-BC5F-4B02CFAB6FF1', 60.0),   # EnemySpawnPoints/Tyrannosaurus (村の跡)
        ('GrassLandHyenaWoods', '72713C78-53F7-4512-935D-02624334571E', 25.0),  # HyenaPack_WestWoods
    ]),
    'GameCore::Scene::DrySandSceneContext': (GRID_DESERT, [
        ('DesertSkeletonDragon', '7288ED07-F841-4219-B51B-A45F8F2EA3AC', 60.0),  # SkeletonDragon (城塞の広場)
        ('DesertOasis', '932D5B4A-509D-49FE-8B8F-D7AE7E3A0E0E', 25.0),      # ScorpionPack_Oasis/ScorpionPack2
    ]),
    'GameCore::Scene::DragonNestSceneContext': (GRID_NEST, [
        ('AncientDragon', 'F389D764-1664-4F54-8D27-BFE7598868A6', 60.0),     # Hearts/NestHeartDusk (心臓の山)
    ]),
}


def num_f(value):
    return cj.Num.of_float(value)


def num_i(value):
    return cj.Num.of_int(value)


def obj(*pairs):
    return cj.OrderedObj(list(pairs))


class Ids:
    def __init__(self, text):
        ids = [int(m) for m in re.findall(r'"id": (\d+)', text)]
        self.next = max(i & 0x7FFFFFFF for i in ids if i & 0x80000000) + 1

    def take(self):
        value = 0x80000000 | self.next
        self.next += 1
        return num_i(value)


def field_blob(guid, ids, first):
    data = obj(('value0', obj(('value_', guid))))
    if first:
        data = obj(('cereal_class_version', num_i(0)), *data.items())
    inner = obj(('polymorphic_id', num_i(1073741824)),
                ('ptr_wrapper', obj(('id', ids.take()), ('data', data))))
    blob = obj(('value0', inner))
    if first:
        blob = obj(('cereal_class_version', num_i(0)), *blob.items())
    return blob


def context_components(doc):
    """GameManage の全 GameObject の components_ から、名前付きのシーンコンテキストを探す"""
    found = []

    def walk(node):
        if isinstance(node, cj.OrderedObj):
            name = node.get('polymorphic_name')
            if isinstance(name, str) and name in CONTEXTS:
                found.append((name, node))
            for value in node.values():
                walk(value)
        elif isinstance(node, list):
            for value in node:
                walk(value)

    walk(doc)
    return found


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()

    text = cj.read_text(SCENE)
    doc = cj.loads(text)
    contexts = context_components(doc)
    missing = set(CONTEXTS) - {name for name, _ in contexts}
    if missing:
        sys.exit(f'contexts not found in {SCENE.name}: {sorted(missing)}')

    ids = Ids(text)
    first_grid = True
    first_entry = True
    for index, (name, component) in enumerate(contexts):
        base = component['ptr_wrapper']['data']['value0']
        if index == 0:
            assert 'cereal_class_version' in base, f'{name}: expected the SceneContextBase version here'
            base['cereal_class_version'] = num_i(BASE_VERSION)
        for key in ('navigationGrid_', 'navigationTargets_'):
            base.pop(key, None)

        grid, targets = CONTEXTS[name]
        base.append('navigationGrid_', field_blob(grid or NO_GUID, ids, first_grid))
        first_grid = False

        entries = []
        for target_id, guid, height in targets:
            entry = obj(('id_', target_id),
                        ('object_', field_blob(guid, ids, False)),
                        ('markerHeight_', num_f(height)))
            if first_entry:
                entry = obj(('cereal_class_version', num_i(0)), *entry.items())
                first_entry = False
            entries.append(entry)
        base.append('navigationTargets_', entries)
        print(f'{name.split("::")[-1]}: grid={"yes" if grid else "no"}, targets={[t[0] for t in targets]}')

    if args.dry_run:
        return
    cj.write_file(SCENE, doc)
    print(f'wrote {SCENE.relative_to(REPO_ROOT)}')


if __name__ == '__main__':
    main()
