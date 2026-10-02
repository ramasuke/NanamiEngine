"""古竜 (終章のボス、docs/Story.md 終章「嵐の巣」) の BT / プレハブ / EnemyFactory と、巣のシーンの湧き地点・カメラを作る。

    python tools/art/story_npcs.py --only nest     # 先に: 古竜の BT が流す会話 (AncientDragon_*)
    python tools/art/nest_heightgrid_bake.py       # 先に: 古竜の経路探索に使う Nest.heightGridMap
    python tools/art/ancient_dragon.py             # BT・プレハブ・ファクトリ・巣への配置 (アセットの GUID は保つ)
    python tools/art/ancient_dragon.py place       # 巣への配置だけ (nest_scene.py でシーンを作り直した後に必ず)
    python tools/art/nest_context.py               # 配置の後に必ず (NestEndingCamera の GUID が変わるので)

- 姿は序章のドラゴン (FirstEventDragon.prefab) の写し。部品を GamePlay::Npc::Enemy::AncientDragon に替え、体力と名前を替える。
  子の "FirstTouchDownIsland ProductionCamera" (竜に付いて動くカメラ) は、登場と最期の画にそのまま使う。
- BT は序章のドラゴンの BT (FirstEventDragon.enemyBehaviourData) の写し。戦いの枝 (State 1/3/5 と攻撃) はそのままで、序章だけの
  枝 (飛行船を落とす・島を焼く・心臓を抜く) を巣の枝に替える:
    - 待機: プレイヤーが APPROACH_DISTANCE より遠い間は、心臓の山の上で羽ばたいて待つ
    - 登場: 近づくと竜のカメラ → クノイチの声 (AncientDragon_Intro) → 嵐が強まり、雷とともに降り立つ → 咆哮 → ボス曲
    - 大砲の援護: 体力が CANNON_RATES を切るたびに、教官の声と一緒に山頂の大砲 (昔の竜撃ちの砲) の弾が降り、古竜がのけぞる
    - 最期: 止まって最後の咆哮 → クノイチの声 (AncientDragon_Defeat) → 嵐が晴れ、光になって消える (OnDeath)。
      その後の「心臓が空へ散る」は DragonNestScene (OnStageClear) が流し、拠点の島へ帰る
- 巣のシーン: EnemySpawnPoints に古竜の湧き地点 (AncientDragonDefeated が立った後は湧かない) と、心臓の山を映す
  NestEndingCamera を置く。GameManage.scene の DragonNestSceneContext は nest_context.py が指す。
"""
import copy
import json
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common import meta_base  # noqa: E402
from tools.common.cereal_json import Num, dumps, loads, to_file_bytes  # noqa: E402

from camp_people import asset_guid  # noqa: E402

BT_DIR = REPO / 'Assets' / 'Data' / 'EnemyBehaviour'
SOURCE_BT = BT_DIR / 'FirstEventDragon.enemyBehaviourData'
BT = BT_DIR / 'AncientDragon.enemyBehaviourData'
PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'Npc' / 'Enemy'
SOURCE_PREFAB = PREFAB_DIR / 'FirstEventDragon.prefab'
PREFAB = PREFAB_DIR / 'AncientDragon.prefab'
FACTORY = REPO / 'Assets' / 'Data' / 'Enemy' / 'StandardEnemyFactory.enemyFactory.meta'
CHAT_DIR = REPO / 'Assets' / 'Data' / 'NpcChatText'
PARTICLE_DIR = REPO / 'Assets' / 'Prefab' / 'Particle'
NEST_GRID = REPO / 'Assets' / 'Data' / 'HeightGridMap' / 'Nest.heightGridMap.meta'
NEST_SCENE = REPO / 'Assets' / 'Scene' / 'DragonNestScene.scene'
GRASS_SCENE = REPO / 'Assets' / 'Scene' / 'GrassLandScene.scene'

CLASS = 'GamePlay::Npc::Enemy::AncientDragon'
BOSS_NAME = '古竜'
HEALTH = 2400
KIND_ANCIENT_DRAGON = 7          # EnemyKind::AncientDragon
DRAGON_CAMERA = 'FirstTouchDownIsland ProductionCamera'
# FirstEventDragon.animTree の State
STATE_FLYING_IDLE, STATE_ROAR = 4, 30
APPROACH_DISTANCE = 420.0
# (体力 %, 会話)
CANNON_RATES = ((60.0, 'AncientDragon_Cannon1'), (25.0, 'AncientDragon_Cannon2'))
# 大砲の弾が降る所 (古竜からのずらし)
CANNON_IMPACTS = ((0.0, 18.0, 0.0), (14.0, 10.0, -12.0), (-12.0, 14.0, 10.0))

# 巣の配置 (world。nest_terrain.py: 心臓の山 MOUND (750, 720)、底の高さ 100、着き場は南)
SPAWN_POS = (750.0, 150.0, 800.0)
SPAWN_YAW = 0.0                   # ラジアン (set_trs)。+z = 着き場 (南) の方
ENDING_CAMERA_POS = (750.0, 190.0, 1010.0)

# 序章の BT のノード (FirstEventDragon.enemyBehaviourData)。名前が重なるものがあるので GUID で指す
ROOT = '5AC0B521-0AB7-4530-A728-54619B8B51CF'
DEATH_SEQ = '438BBF45-D801-4E07-99D0-A9597DF3F880'
DEATH_KEEP = {
    'IsDeath': '07112C7A-11A8-49D7-9959-4544E6B16652',
    'Dying Sound': '4F91A6A1-5072-40B1-A8B3-2BE830DD73DB',
    'Victory Chime': 'EBD4D3EF-FFF8-40A6-9567-523EC8542CD9',
    'Defeat Light Burst': 'C9205A3C-AED5-498C-85E1-38A9B984DDA9',
    'Defeat Light Burst Action': '3A9A7B78-E850-4EDF-9AFA-963B6C796560',
    'Wait Defeat Beat': '8CC3E7C1-3310-40B9-816C-7E159DB28886',
    'Storm Clear': '114920D9-1490-44F2-8E96-76557641A381',
}
STATE0_SEQ = '04E3F0D6-E481-4609-BA33-F4FC957497E5'
STATE0_READ = '5212EAF3-12DB-405F-A012-528DBE2C3A9C'
STATE0_STORM_BEGIN = 'C7BE81D6-D4A6-4BC6-8503-DE9EBA683BB0'
STATE0_REMOVE = ('90B8438C-F0FB-42F2-86A1-950A8BA2717B',
                 '76605FB4-3EA2-453E-8E51-96F20FBA1277', 'CFB1E11A-A14F-4687-9DD6-4D8DD64B782A',
                 '80D57EC7-B83A-465B-AFC9-1EC16ACF4AFF',
                 # 飛来の不穏な音 (dragon_omen_bt.py)。ToDestroyAirShip は Omen Approach の Cue に入っている
                 '2C2E254E-76A4-435F-85DD-3FD847DF67AC', '53E3B11A-1894-4164-BC6F-D2A0FDB726C0',
                 '50C4A7DF-E279-4784-9A0B-6B665CB26191', '055914D0-3CF1-4E93-ACBF-D7444D87193E')
DESTROY_ISLAND_SEQ = '94CD5153-9D21-42DD-A535-28FEA3A73499'
VELOCITY_ZERO = '83393CA0-7408-46E9-BBCA-2D9554F326FE'
CHASE_PLAYER = 'F3AFC96C-214F-4188-9484-F32499396BBC'
IMPACT_PARTICLE = 'A05B1905-1ACA-4698-A6BF-84E839AE21E0'   # IslandFireImpact (序章の火球の着弾)
IMPACT_SOUND = 'E196D03A-BB3E-4443-B347-FC4EEE017451'


def run(*args):
    subprocess.run([sys.executable, '-m', *args], cwd=REPO, check=True)


def chat(name):
    return asset_guid(CHAT_DIR / f'{name}.npcChat.meta')


def particle(name):
    return asset_guid(PARTICLE_DIR / f'{name}.prefab.meta')


# ---------------------------------------------------------------- BT
class Ops:
    def __init__(self):
        self.ops = []
        self.added = []

    def _guid(self):
        g = meta_base.mint_guid()
        self.added.append(g)
        return g

    def node(self, parent, kind, index=None):
        guid = self._guid()
        op = {'op': 'add-node', 'parent': parent, 'kind': kind, 'guid': guid}
        if index is not None:
            op['index'] = index
        self.ops.append(op)
        return guid

    def action(self, parent, label, kind, index=None, **params):
        guid = self._guid()
        op = {'op': 'add-node', 'parent': parent, 'kind': 'action', 'name': label, 'type': kind, 'guid': guid}
        if index is not None:
            op['index'] = index
        self.ops.append(op)
        if params:
            self.set(guid, **params)
        return guid

    def once(self, parent, label, kind, index=None, **params):
        return self.action(self.node(parent, 'once-exec', index), label, kind, **params)

    def set(self, guid, **params):
        values = {k: (str(v).lower() if isinstance(v, bool) else
                      ','.join(str(x) for x in v) if isinstance(v, tuple) else str(v)) for k, v in params.items()}
        self.ops.append({'op': 'set-params', 'node': guid, 'set': values})

    def remove(self, guid):
        self.ops.append({'op': 'remove-node', 'node': guid})

    def move(self, guid, parent, index):
        self.ops.append({'op': 'move-node', 'node': guid, 'parent': parent, 'index': index})

    def copy(self, guid, parent, index):
        self.ops.append({'op': 'copy-node', 'node': guid, 'parent': parent, 'index': index})


def edit_ops():
    o = Ops()
    o.ops.append({'op': 'add-bb-param', 'name': 'Cannon', 'value': 0})

    # --- 最期
    tree = load_source()
    death = tree.find(DEATH_SEQ)
    keep = set(DEATH_KEEP.values())
    for child in list(death.children):
        if child.guid not in keep:
            o.remove(child.guid)
    i = iter(range(100))
    o.move(DEATH_KEEP['IsDeath'], DEATH_SEQ, next(i))
    o.copy(VELOCITY_ZERO, DEATH_SEQ, next(i))
    o.once(DEATH_SEQ, 'Death Camera On', 'PurposeCamera', next(i), prefabPurposeCamera_=DRAGON_CAMERA, priority_=100)
    o.move(DEATH_KEEP['Dying Sound'], DEATH_SEQ, next(i))
    o.once(DEATH_SEQ, 'Last Roar Animation', 'PlayAnimation', next(i), animatorSetParamNumber_=STATE_ROAR)
    o.once(DEATH_SEQ, 'Death Shake', 'ShakeCamera', next(i), intensity_=1.0, duration_=3.0)
    o.once(DEATH_SEQ, 'Defeat Chat', 'StartChat', next(i), displayName_='クノイチ', chatData_=chat('AncientDragon_Defeat'))
    o.move(DEATH_KEEP['Storm Clear'], DEATH_SEQ, next(i))
    o.move(DEATH_KEEP['Wait Defeat Beat'], DEATH_SEQ, next(i))
    o.set(DEATH_KEEP['Wait Defeat Beat'], waitSeconds_=3.0)
    o.move(DEATH_KEEP['Victory Chime'], DEATH_SEQ, next(i))
    o.move(DEATH_KEEP['Defeat Light Burst'], DEATH_SEQ, next(i))
    o.set(DEATH_KEEP['Defeat Light Burst Action'], offset_=(0.0, 15.0, 0.0))
    o.once(DEATH_SEQ, 'Heart Light Burst', 'GenerateParticle', next(i),
           offset_=(0.0, 12.0, 0.0), lifeTime_=3.3, isUseAbsolutePosition_=False, particlePrefab_=particle('HeartShardScatter'))
    o.action(DEATH_SEQ, 'Dissolve Wait', 'WaitSeconds', next(i), waitSeconds_=1.2)
    o.action(DEATH_SEQ, 'Death Camera Off', 'PurposeCamera', next(i), prefabPurposeCamera_=DRAGON_CAMERA, priority_=-1)
    o.action(DEATH_SEQ, 'On Death', 'OnDeath', next(i), value1=0)

    # --- 待機 (プレイヤーが遠い間は、心臓の山の上で羽ばたいて待つ)
    waiting = o.node(ROOT, 'sequence', 1)
    o.copy(STATE0_READ, waiting, 0)
    o.action(waiting, 'Player Far', 'ToPlayerDistance', distance_=APPROACH_DISTANCE, isInnerDistance_=False)
    o.action(waiting, 'Hover Animation', 'PlayAnimation', animatorSetParamNumber_=STATE_FLYING_IDLE)
    o.copy(VELOCITY_ZERO, waiting, 3)

    # --- 登場 (State 0 の枝を、飛行船と島の場面から巣の場面に替える)
    for guid in STATE0_REMOVE:
        o.remove(guid)
    o.set(STATE0_STORM_BEGIN, intensity_=0.85, blendSeconds_=6.0)
    # State0 / FlyingIdleAnimation / Storm Begin の後ろへ
    o.once(STATE0_SEQ, 'Intro Camera On', 'PurposeCamera', 3, prefabPurposeCamera_=DRAGON_CAMERA, priority_=100)
    o.once(STATE0_SEQ, 'Intro Chat', 'StartChat', 4, displayName_='クノイチ', chatData_=chat('AncientDragon_Intro'))
    o.once(STATE0_SEQ, 'Intro Lightning', 'Lightning', 5, intensity_=1.0, durationSeconds_=0.6)
    o.action(STATE0_SEQ, 'Intro Beat', 'WaitSeconds', 6, waitSeconds_=2.5)

    # --- 大砲の援護 (序章の「島を焼く」枝の代わり)
    o.remove(DESTROY_ISLAND_SEQ)
    for n, (rate, chat_name) in enumerate(CANNON_RATES):
        seq = o.node(ROOT, 'sequence', 4 + n)
        o.action(seq, f'Cannon {n}', 'ReadBlackBoard', keyName_='Cannon', equalValue_=n)
        o.action(seq, f'Health {rate:.0f}%', 'IsCurrentHealthRate', rate_=rate)
        o.copy(VELOCITY_ZERO, seq, 2)
        o.once(seq, f'Cannon Chat {n}', 'StartChat', displayName_='教官', chatData_=chat(chat_name))
        o.action(seq, 'Cannon Aim Wait', 'WaitSeconds', waitSeconds_=1.2)
        volley = o.node(o.node(seq, 'once-exec'), 'sequence')
        for k, offset in enumerate(CANNON_IMPACTS):
            o.action(volley, f'Cannon Impact {k}', 'GenerateParticle', offset_=offset, lifeTime_=3.0,
                     isUseAbsolutePosition_=False, particlePrefab_=IMPACT_PARTICLE)
        o.action(volley, 'Cannon Impact Sound', 'PlaySE', sound_=IMPACT_SOUND)
        o.action(volley, 'Cannon Impact Shake', 'ShakeCamera', intensity_=1.0, duration_=1.2)
        o.action(volley, 'Cannon Flash', 'Lightning', intensity_=0.6, durationSeconds_=0.3)
        o.once(seq, 'Cannon Reel Animation', 'PlayAnimation', animatorSetParamNumber_=STATE_ROAR)
        o.action(seq, 'Cannon Reel Wait', 'WaitSeconds', waitSeconds_=3.0)
        o.action(seq, f'Cannon To {n + 1}', 'WriteBlackBoard', keyName_='Cannon', value_=n + 1)
        o.action(seq, 'State To 3', 'WriteBlackBoard', keyName_='State', value_=3)

    # --- 巣の地形で経路を探す
    o.set(CHASE_PLAYER, heightGridMap_=asset_guid(NEST_GRID))
    return o


def load_source():
    from tools.bt import cli_edit
    _, tree, _ = cli_edit._load(SOURCE_BT)
    return tree


def build_tree():
    from tools.bt import cli_edit, edits, model
    from tools.bt.layout import auto_layout
    from tools.bt.validate import validate

    text, tree, cat = cli_edit._load(SOURCE_BT)
    o = edit_ops()
    edits.apply(tree, o.ops, cat=cat)
    # NOTE: 序章の BT には古い版のノードがあって全体の検証に落ちるので、足したノードだけ確かめる
    problems = [p for p in validate(tree, cat=cat) if not p.startswith('note:') and any(g in p for g in o.added)]
    if problems:
        raise SystemExit('AncientDragon BT failed validation:\n' + '\n'.join(problems))
    auto_layout(tree)
    out = cli_edit.write_tree(tree)

    # 序章の BT と GUID が重ならないよう、ノードの GUID を振り直す (アセットの GUID はそのまま)
    nodes = {tree.entry.guid} | {n.guid for n, *_ in tree.walk()}
    for guid in nodes:
        out = out.replace(guid, meta_base.mint_guid())
    BT.write_bytes(cli_edit.to_file_bytes(out))

    meta = Path(str(BT) + '.meta')
    guid = asset_guid(meta) if meta.exists() else meta_base.mint_guid()
    src_meta = Path(str(SOURCE_BT) + '.meta').read_bytes().decode('utf-8-sig')
    src_meta = src_meta.replace(asset_guid(Path(str(SOURCE_BT) + '.meta')), guid)
    src_meta = src_meta.replace('FirstEventDragon.enemyBehaviourData', 'AncientDragon.enemyBehaviourData')
    meta.write_bytes(src_meta.encode('utf-8'))
    print(f'wrote {BT.relative_to(REPO)}  (guid {guid}, {len(o.added)} nodes added)')
    return guid


# ---------------------------------------------------------------- プレハブ
def build_prefab(bt_guid):
    meta = Path(str(PREFAB) + '.meta')
    old_guid = asset_guid(meta) if meta.exists() else None
    run('tools.scene', 'copy-prefab', SOURCE_PREFAB.relative_to(REPO).as_posix(), '--name', 'AncientDragon', '--force')
    if old_guid:
        new_guid = asset_guid(meta)
        meta.write_bytes(meta.read_bytes().replace(new_guid.encode(), old_guid.encode()))

    text = PREFAB.read_bytes().decode('utf-8')
    nl = '\r\n' if '\r\n' in text else '\n'
    source_bt = asset_guid(Path(str(SOURCE_BT) + '.meta'))
    assert text.count(source_bt) == 1, source_bt
    text = text.replace(source_bt, bt_guid)

    # 部品: FirstEventDragon (版 4 = BossEnemyBase だけ) -> AncientDragon (版 0 = BossEnemyBase + 落ちたときの戻し先)
    key = '"polymorphic_name": "GamePlay::Npc::Enemy::FirstEventDragon"'
    assert text.count(key) == 1
    i = text.index(key)
    text = text[:i] + f'"polymorphic_name": "{CLASS}"' + text[i + len(key):]
    d = text.index('"data": {', i)
    v = text.index('"cereal_class_version": 4', d)
    text = text[:v] + '"cereal_class_version": 0' + text[v + len('"cereal_class_version": 4'):]
    j = text.index('"value0": {', v)
    k, depth = text.index('{', j), 0
    while True:
        if text[k] == '{':
            depth += 1
        elif text[k] == '}':
            depth -= 1
            if depth == 0:
                break
        k += 1
    extra = (f',{nl}"respawnPosition_": {{{nl}"value0": {SPAWN_POS[0]},{nl}"value1": {SPAWN_POS[1] + 70.0},{nl}'
             f'"value2": {SPAWN_POS[2]}{nl}}},{nl}"fallLimitY_": 0.0')
    text = text[:k + 1] + extra + text[k + 1:]

    text = text.replace('"bossName_": "ドラゴン"', f'"bossName_": "{BOSS_NAME}"')
    text = re.sub(r'("maxHealth_": \{\s*"cereal_class_version": 0,\s*"value_": )600', rf'\g<1>{HEALTH}', text)
    text = re.sub(r'("currentHealth_": \{\s*"polymorphic_id": \d+,\s*"ptr_wrapper": \{\s*"id": \d+,\s*"data": \{\s*"value0": \{\s*"value_": )600',
                  rf'\g<1>{HEALTH}', text)
    assert text.count(f'"value_": {HEALTH}') == 2, 'health not replaced'
    text = text.replace('"name_": "FirstEventDragon"', '"name_": "AncientDragon"')
    PREFAB.write_bytes(text.encode('utf-8'))

    # 整形し直して検証する
    from tools.scene import reader, writer
    model = reader.read_prefab_file(PREFAB)
    PREFAB.write_bytes(to_file_bytes(writer.write_prefab(model)))
    res = subprocess.run([sys.executable, '-m', 'tools.scene', 'validate', PREFAB.relative_to(REPO).as_posix()],
                         cwd=REPO, capture_output=True, text=True)
    if res.returncode != 0:
        raise SystemExit(f'{PREFAB.name}: validate failed\n{res.stdout}{res.stderr}')
    guid = asset_guid(meta)
    print(f'wrote {PREFAB.relative_to(REPO)}  (guid {guid})')
    return guid


# ---------------------------------------------------------------- ファクトリ
def factory(prefab_guid):
    raw = FACTORY.read_bytes()
    crlf = b'\r\n' in raw[:200]
    meta = json.loads(raw.decode('utf-8-sig'))
    data = meta['value0']['ptr_wrapper']['data']
    data['cereal_class_version'] = 3
    blob = json.loads(json.dumps(data['skeletonDragonPrefab_']))
    blob['value0']['ptr_wrapper']['data']['value0']['value_'] = prefab_guid
    data['ancientDragonPrefab_'] = blob
    ids = [0x80000000 | 1]

    def renum(o):
        if isinstance(o, dict):
            for key, val in o.items():
                if key == 'id' and isinstance(val, int):
                    ids[0] += 1
                    o[key] = ids[0]
                else:
                    renum(val)
        elif isinstance(o, list):
            for val in o:
                renum(val)
    renum(data)
    text = json.dumps(meta, indent=4, ensure_ascii=False)
    if crlf:
        text = text.replace('\n', '\r\n')
    FACTORY.write_bytes(text.encode('utf-8'))
    print(f'wrote {FACTORY.relative_to(REPO)}  (ancientDragonPrefab_ {prefab_guid})')


# ---------------------------------------------------------------- 巣への配置
def place():
    from tools.common.blob import Ver  # noqa: F401
    from tools.scene import catalog as catalog_mod, edits, reader, validate, writer
    from game_over_prefab import check, let_writer_place_versions
    from grassland_nature_scatter import StrayVersionStripper, bake_world_matrices, walk
    from desert_scene import component, find, set_trs
    from main_island_finale import shake_component

    scene = reader.read_scene_file(NEST_SCENE)
    grass = reader.read_scene_file(GRASS_SCENE)

    # 湧き地点: 草原の地点を1つ写す
    root = find(scene, 'EnemySpawnPoints')
    root.transform.children = [c for c in root.transform.children if c.name != 'AncientDragon']
    template = next(n for pack in find(grass, 'EnemySpawnPoints').transform.children for n in walk(pack)
                    if any(c.fqn.endswith('::EnemySpawnPoint') for c in n.components))
    spawn = copy.deepcopy(template)
    spawn.transform.children = []
    edits._remint_guids(spawn, {})
    spawn.name = 'AncientDragon'
    sp = component(spawn, 'EnemySpawnPoint')
    sp.data['kind_'] = Num.of_int(KIND_ANCIENT_DRAGON)
    set_trs(spawn, pos=SPAWN_POS, yaw=SPAWN_YAW)
    root.transform.children.append(spawn)

    # 心臓の山を映すカメラ: 草原の GreenStoneCamera (VirtualCamera + LookAt) を写す
    scene.roots = [r for r in scene.roots if r.name != 'NestEndingCamera']
    camera = copy.deepcopy(find(grass, 'GreenStoneCamera'))
    camera.components.append(shake_component())
    edits._remint_guids(camera, {})
    camera.name = 'NestEndingCamera'
    set_trs(camera, pos=ENDING_CAMERA_POS, yaw=0.0)
    # NOTE: 序章から写した部品 (WEATHER_ROOTS) より前に置く (同じ型が先に出る方に版キーが付くため)
    weather_at = next((i for i, r in enumerate(scene.roots) if r.name in ('StormSkyDomeLower', 'LightningFlash', 'StormSky')),
                      len(scene.roots))
    scene.roots.insert(weather_at, camera)

    for r in scene.roots:
        for node in walk(r):
            for comp in node.components:
                let_writer_place_versions(comp.data)
        bake_world_matrices(r)
    text = writer.write_scene(scene)
    tree = loads(text)
    StrayVersionStripper(catalog_mod.load(), '/').run(tree, '')
    text = dumps(tree)
    check(text, validate.validate_scene(scene), NEST_SCENE.name)
    NEST_SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(NEST_SCENE)
    print(f'placed AncientDragon spawn {SPAWN_POS} and NestEndingCamera {ENDING_CAMERA_POS} in {NEST_SCENE.name}')


def main():
    if sys.argv[1:] == ['place']:
        place()
        return
    bt = build_tree()
    prefab = build_prefab(bt)
    factory(prefab)
    place()


if __name__ == '__main__':
    main()
