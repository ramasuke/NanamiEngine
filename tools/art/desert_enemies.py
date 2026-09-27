"""砂漠の敵 (大サソリ・サンドワーム・骸竜) の AnimTree / BT / プレハブと、EnemyFactory の設定を作る。

    python tools/art/desert_enemies.py                # 全部作り直す (アセットの GUID は保つ)
    python tools/art/desert_enemies.py --storm-only   # 骸竜の AnimTree / BT に砂嵐のギミックだけ足す

- モデルは Assets/Art/Models/Monster/<Name>/<Name>.mv1 (Sketchfab、NanamiAssetsWork/Desert/process_anim.py で 30fps に
  焼いて変換)。1つの .mv1 にクリップが名前順で入っているので、AnimTree のクリップは modelAnimationIndex_ で選ぶ。
- AnimTree の State 番号はハイエナ (Assets/Animations/Enemy/Hyena.animTree) と同じにしてある。だから BT はハイエナの
  BT を写して、高さグリッドを砂漠のものに替え、速さなどを少し変えるだけで動く。
    0 = 移動 / 11111 = 待機 / 7 = 嗅ぐ / 8 = 眠る / 15 = 吠える / 20 = 死ぬ / 23, 24, 25 = 攻撃1..3
- プレハブはハイエナのプレハブの写し。モデル・AnimTree・BT・体力を替え、当たり判定と攻撃範囲を体の大きさ (SIZE) 倍にする。
  骸竜はボス (BossEnemyBase) なので、敵の部品を BossEnemyBase で包み、bossName_ を付ける (大顎と同じ形)。
- EnemyFactory (StandardEnemyFactory.enemyFactory.meta) の desertScorpionPrefab_ / sandWormPrefab_ / skeletonDragonPrefab_
  にプレハブを入れる (EnemyFactory の版 2)。
"""
import json
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common import meta_base  # noqa: E402

from camp_people import apply_ops, asset_guid  # noqa: E402

TREE_DIR = REPO / 'Assets' / 'Animations' / 'Enemy'
BT_DIR = REPO / 'Assets' / 'Data' / 'EnemyBehaviour'
PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'Npc' / 'Enemy'
MODEL_DIR = REPO / 'Assets' / 'Art' / 'Models' / 'Monster'
HYENA_BT = BT_DIR / 'HyenaBehaviour.enemyBehaviourData'
HYENA_PREFAB = PREFAB_DIR / 'Hyena.prefab'
FACTORY = REPO / 'Assets' / 'Data' / 'Enemy' / 'StandardEnemyFactory.enemyFactory.meta'
GRASS_GRID = asset_guid(REPO / 'Assets/Data/HeightGridMap/GrassLand.heightGridMap.meta')
DESERT_GRID = asset_guid(REPO / 'Assets/Data/HeightGridMap/Desert.heightGridMap.meta')
CLIP_FPS = 30.0

# 骸竜の砂嵐のギミック (add_storm_gimmick) で使う State。番号は SkeletonDragon::HeartStun の既定値と揃える
STORM_STATES = {30: ('Summon', 5, False), 31: ('StunStart', 15, False), 32: ('StunIdle', 13, True),
                33: ('StunOver', 14, False)}
STATE_SUMMON = 30
# 体力がこの割合 (%) を切ったら砂嵐を呼ぶ。1回目と2回目
STORM_HEALTH_RATES = (70.0, 35.0)
SUMMON_SECS = 2.0

# State 番号 -> (ノード名, .mv1 の中のクリップ番号, ループするか)
ENEMIES = {
    'DesertScorpion': dict(
        cls='GamePlay::Npc::Enemy::DesertScorpion', health=90, size=1.6, boss=None,
        # Attack, Death, Defend, Idle, Walk
        states={0: ('Walk', 4, True), 11111: ('Idle', 3, True), 7: ('Sniff', 3, True), 8: ('Rest', 2, True),
                15: ('Threat', 2, False), 20: ('Die', 1, False), 23: ('Sting1', 0, False),
                24: ('Sting2', 0, False), 25: ('Sting3', 0, False)},
        speeds={'moveSpeed_': 0.8}),
    'SandWorm': dict(
        cls='GamePlay::Npc::Enemy::SandWorm', health=140, size=1.8, boss=None,
        # Attack, Damage, Death, Idle, Run, Walk
        states={0: ('Run', 4, True), 11111: ('Idle', 3, True), 7: ('Idle2', 3, True), 8: ('Lurk', 5, True),
                15: ('Rear', 1, False), 20: ('Die', 2, False), 23: ('Bite1', 0, False),
                24: ('Bite2', 0, False), 25: ('Bite3', 0, False)},
        speeds={'moveSpeed_': 0.7}),
    'SkeletonDragon': dict(
        cls='GamePlay::Npc::Enemy::SkeletonDragon', health=1600, size=3.0, boss='骸竜',
        # attack_2..5, born, cast_1, dead, hurtbig, hurtsmall, pettyidle, soar, stun*, travel*, win*
        states={0: ('TravelMove', 17, True), 11111: ('PettyIdle', 9, True), 7: ('TravelIdle', 16, True),
                8: ('Born', 4, False), 15: ('Roar', 22, False), 20: ('Dead', 6, False),
                23: ('Attack2', 0, False), 24: ('Attack3', 1, False), 25: ('Attack4', 2, False),
                **STORM_STATES},
        speeds={'moveSpeed_': 1.1},
        boss_bgm=REPO / 'Assets/Audio/BGM/Boss_SkeletonDragon.mp3.meta',
        storm_gimmick=True),
}
# ボス曲を流し始める、プレイヤーとの距離 (大顎の "Player Near" と同じ)
BOSS_BGM_DISTANCE = 220.0


def run(*args):
    subprocess.run([sys.executable, '-m', *map(str, args)], cwd=REPO, check=True, capture_output=True)


# ---------------------------------------------------------------- AnimTree
def clip_ops(any_state, mv1, state, label, index, loop, guid, pos):
    """クリップのノードと、State == state で AnyState から入る遷移"""
    dur = 0.1 if state in (23, 24, 25) else 0.15
    return [
        {'op': 'add-clip-node', 'name': label, 'clip': mv1, 'speed': CLIP_FPS, 'blend_offset_secs': 100000.0,
         'is_loop': loop, 'model_anim_index': index, 'guid': guid, 'pos': pos},
        {'op': 'add-transition', 'from': any_state, 'next': guid, 'any_state': True, 'duration_secs': dur},
        {'op': 'add-condition', 'any_state': True, 'from': any_state, 'next': guid,
         'name': 'State', 'kind': 'int', 'value': state},
    ]


def anim_tree(name, spec):
    tree = f'{name}'
    data_path, meta_path = TREE_DIR / f'{tree}.animTree', TREE_DIR / f'{tree}.animTree.meta'
    old = asset_guid(meta_path) if meta_path.exists() else None
    run('tools.animtree', 'new-tree', tree, '--dir', 'Assets/Animations/Enemy', '--force')
    guid = asset_guid(meta_path)
    if old:
        meta_path.write_bytes(meta_path.read_bytes().replace(guid.encode(), old.encode()))
        data_path.write_bytes(data_path.read_bytes().replace(guid.encode(), old.encode()))
        guid = old
    rel = f'Assets/Animations/Enemy/{tree}.animTree'
    shown = subprocess.run([sys.executable, '-m', 'tools.animtree', 'show', rel], cwd=REPO, check=True,
                           capture_output=True, text=True).stdout
    entry = next(line.split()[1] for line in shown.splitlines() if line.startswith('Entry'))
    any_state = next(line.split()[1] for line in shown.splitlines() if line.startswith('AnyState'))
    mv1 = (MODEL_DIR / name / f'{name}.mv1').relative_to(REPO).as_posix()
    ops = [{'op': 'add-param', 'name': 'State', 'kind': 'int', 'value': 11111}]
    idle = None
    for i, (state, (label, index, loop)) in enumerate(spec['states'].items()):
        g = meta_base.mint_guid()
        idle = g if state == 11111 else idle
        ops += clip_ops(any_state, mv1, state, label, index, loop, g, [340 + 200 * (i // 5), 120 + 90 * (i % 5)])
    ops.append({'op': 'add-transition', 'from': entry, 'next': idle})
    apply_ops('tools.animtree', rel, ops)
    run('tools.animtree', 'validate', rel)
    return guid


# ---------------------------------------------------------------- BT
def behaviour(name, spec):
    """ハイエナの BT を写す。GUID は自分のもの、高さグリッドは砂漠、速さは spec の倍率"""
    data = BT_DIR / f'{name}.enemyBehaviourData'
    meta = Path(str(data) + '.meta')
    guid = asset_guid(meta) if meta.exists() else None
    if not guid:
        run('tools.bt', 'new-tree', name, '--npc-kind', 'enemy')
        guid = asset_guid(meta)
    hyena_guid = asset_guid(Path(str(HYENA_BT) + '.meta'))
    text = HYENA_BT.read_bytes().decode('utf-8')
    text = text.replace(hyena_guid, guid).replace(GRASS_GRID, DESERT_GRID)
    for key, rate in spec['speeds'].items():
        text = re.sub(rf'("{key}": )([\d.]+)', lambda m: f'{m.group(1)}{float(m.group(2)) * rate:.1f}', text)
    data.write_bytes(text.encode('utf-8'))
    if spec.get('boss_bgm'):
        add_boss_bgm(data, asset_guid(spec['boss_bgm']))
    if spec.get('storm_gimmick'):
        add_storm_gimmick(data)
    # NOTE: tools.bt validate はハイエナの BT そのもの (古い版の RecoverFacingPlayer) でも落ちるので掛けない
    return guid


def add_boss_bgm(data, bgm_guid):
    """プレイヤーが近づくか、攻撃されて警戒したら (Alert 1)、ボス曲を1回だけ流す枝を根の先頭に足す"""
    # NOTE: tools.bt apply はハイエナ由来の古い版のノードで検証に落ちて書かないので、足したノードだけ検証して書く
    from tools.bt import cli_edit, edits
    from tools.bt.layout import auto_layout
    from tools.bt.validate import validate
    text, tree, cat = cli_edit._load(data)
    root = tree.entry.child.guid
    seq, trigger = meta_base.mint_guid(), meta_base.mint_guid()
    first, near, alert, play, done = (meta_base.mint_guid() for _ in range(5))
    edits.apply(tree, [
        {'op': 'add-bb-param', 'name': 'BossBgm', 'value': 0},
        {'op': 'add-node', 'parent': root, 'kind': 'sequence', 'index': 0, 'guid': seq},
        {'op': 'add-node', 'parent': seq, 'kind': 'action', 'name': 'BossBgm 0', 'type': 'ReadBlackBoard',
         'guid': first},
        {'op': 'add-node', 'parent': seq, 'kind': 'selector', 'guid': trigger},
        {'op': 'add-node', 'parent': trigger, 'kind': 'action', 'name': 'Player Near', 'type': 'ToPlayerDistance',
         'guid': near},
        {'op': 'add-node', 'parent': trigger, 'kind': 'action', 'name': 'Alert 1', 'type': 'ReadBlackBoard',
         'guid': alert},
        {'op': 'add-node', 'parent': seq, 'kind': 'action', 'name': 'Play BossBGM', 'type': 'PlayBGM', 'guid': play},
        {'op': 'add-node', 'parent': seq, 'kind': 'action', 'name': 'BossBgm To 1', 'type': 'WriteBlackBoard',
         'guid': done},
        {'op': 'set-params', 'node': first, 'set': {'keyName_': 'BossBgm', 'equalValue_': '0'}},
        {'op': 'set-params', 'node': near,'set': {'distance_': str(BOSS_BGM_DISTANCE), 'isInnerDistance_': 'true'}},
        {'op': 'set-params', 'node': alert, 'set': {'keyName_': 'Alert', 'equalValue_': '1'}},
        {'op': 'set-params', 'node': play, 'set': {'bgm_': bgm_guid}},
        {'op': 'set-params', 'node': done, 'set': {'keyName_': 'BossBgm', 'value_': '1'}},
    ], cat=cat)
    added = (seq, trigger, first, near, alert, play, done)
    problems = [p for p in validate(tree, cat=cat) if not p.startswith('note:') and any(g in p for g in added)]
    if problems:
        raise SystemExit('boss BGM branch failed validation:\n' + '\n'.join(problems))
    auto_layout(tree)
    data.write_bytes(cli_edit.to_file_bytes(cli_edit.write_tree(tree)))


def add_storm_gimmick(data):
    """骸竜の砂嵐のギミック。体力が STORM_HEALTH_RATES を切るたびに砂嵐を呼び、光の心臓が揺らいだら気絶する。

    根の Selector の、いつもの動き (最後の Sequence) の手前に次の枝を足す:
      - 気絶: SkeletonDragon::HeartStun (心臓が揺らいだら砂嵐を止めて倒れ込む) -> State 3 (プレイヤーへ向き直る)
      - 砂嵐 i: Storm == i -> (前の砂嵐が止んでいる) -> 体力 <= rate -> 止まって Summon を再生 -> 砂嵐を呼ぶ -> Storm = i + 1
    死ぬ枝では、倒れる前に砂嵐を止める。
    """
    from tools.bt import cli_edit, edits, model
    from tools.bt.layout import auto_layout
    from tools.bt.validate import validate
    text, tree, cat = cli_edit._load(data)
    if any(p.name == 'Storm' for p in tree.params):
        print(f'{data.name}: storm gimmick already added')
        return
    root = tree.entry.child
    kids = model.children_of(root)
    main_index = len(kids) - 1
    nodes = [n for n, *_ in tree.walk()]
    velocity_zero = next(n for n in nodes if isinstance(n, model.Action) and n.name == 'VelocityZero')
    death_seq = next((n for n in nodes if isinstance(n, model.Sequence)
                      and any(getattr(c, 'name', '') == 'IsDeath' for c in n.children)), None)
    if death_seq is None:
        raise SystemExit(f'{data.name}: no death sequence (IsDeath)')

    added = []

    def g():
        added.append(meta_base.mint_guid())
        return added[-1]

    def action(parent, name, kind, params=None, index=None):
        guid = g()
        op = {'op': 'add-node', 'parent': parent, 'kind': 'action', 'name': name, 'type': kind, 'guid': guid}
        if index is not None:
            op['index'] = index
        ops.append(op)
        if params:
            ops.append({'op': 'set-params', 'node': guid, 'set': params})
        return guid

    ops = [{'op': 'add-bb-param', 'name': 'Storm', 'value': 0}]
    stun = g()
    ops.append({'op': 'add-node', 'parent': root.guid, 'kind': 'sequence', 'index': main_index, 'guid': stun})
    # NOTE: 足したノードの値は 0 で埋まるので、C++ の既定値と同じ値を入れる
    action(stun, 'Heart Shaken Stun', 'HeartStun', {
        'stunStartState_': '31', 'stunIdleState_': '32', 'stunOverState_': '33',
        'start_secs_': '1.2', 'idle_secs_': '6.0', 'over_secs_': '1.5'})
    action(stun, 'State To 3', 'WriteBlackBoard', {'keyName_': 'State', 'value_': '3'})

    for i, rate in enumerate(STORM_HEALTH_RATES):
        seq = g()
        ops.append({'op': 'add-node', 'parent': root.guid, 'kind': 'sequence', 'index': main_index + 1 + i,
                    'guid': seq})
        action(seq, f'Storm {i}', 'ReadBlackBoard', {'keyName_': 'Storm', 'equalValue_': str(i)})
        if i > 0:
            action(seq, 'Storm Calm', 'IsBossSandstorm', {'isActive_': 'false'})
        action(seq, f'Health {rate:.0f}%', 'IsCurrentHealthRate', {'rate_': str(rate)})
        # NOTE: copy-node の GUID は返らないので、置き場所だけ index で決める
        ops.append({'op': 'copy-node', 'node': velocity_zero.guid, 'parent': seq, 'index': 3 if i > 0 else 2})
        action(seq, 'Summon Animation', 'PlayAnimation', {'animatorSetParamNumber_': str(STATE_SUMMON)})
        action(seq, 'Wait Summon', 'WaitSeconds', {'waitSeconds_': str(SUMMON_SECS)})
        action(seq, 'Summon Sandstorm', 'BossSandstorm', {'isSummon_': 'true'})
        action(seq, f'Storm To {i + 1}', 'WriteBlackBoard', {'keyName_': 'Storm', 'value_': str(i + 1)})
        action(seq, 'State To 3', 'WriteBlackBoard', {'keyName_': 'State', 'value_': '3'})

    action(death_seq.guid, 'Stop Sandstorm', 'BossSandstorm', {'isSummon_': 'false'}, index=1)
    edits.apply(tree, ops, cat=cat)

    problems = [p for p in validate(tree, cat=cat) if not p.startswith('note:') and any(x in p for x in added)]
    if problems:
        raise SystemExit('storm gimmick branch failed validation:\n' + '\n'.join(problems))
    auto_layout(tree)
    data.write_bytes(cli_edit.to_file_bytes(cli_edit.write_tree(tree)))
    print(f'{data.name}: added storm gimmick ({len(added)} nodes)')


def add_storm_states(name, spec):
    """今ある AnimTree に、まだ無い STORM_STATES のクリップと遷移だけを足す (作り直さない)"""
    rel = f'Assets/Animations/Enemy/{name}.animTree'
    shown = subprocess.run([sys.executable, '-m', 'tools.animtree', 'show', rel], cwd=REPO, check=True,
                           capture_output=True, text=True).stdout
    any_state = next(line.split()[1] for line in shown.splitlines() if line.startswith('AnyState'))
    mv1 = (MODEL_DIR / name / f'{name}.mv1').relative_to(REPO).as_posix()
    ops = []
    for i, (state, (label, index, loop)) in enumerate(STORM_STATES.items()):
        if re.search(rf'\b{label}\b', shown):
            continue
        g = meta_base.mint_guid()
        ops += clip_ops(any_state, mv1, state, label, index, loop, g, [940, 120 + 90 * i])
    if not ops:
        print(f'{rel}: storm states already added')
        return
    apply_ops('tools.animtree', rel, ops)
    run('tools.animtree', 'validate', rel)
    print(f'{rel}: added {len(ops) // 3} storm state(s)')


# ---------------------------------------------------------------- プレハブ
def scale_block(text, key, rate):
    """"key": {"value0": x, "value1": y, "value2": z} の3つの数を rate 倍にする"""
    def repl(m):
        nums = [float(v) * rate for v in m.group(2, 4, 6)]
        return f'{m.group(1)}{nums[0]}{m.group(3)}{nums[1]}{m.group(5)}{nums[2]}'
    return re.sub(rf'("{key}": \{{\s*"value0": )(-?[\d.e+-]+)(,\s*"value1": )(-?[\d.e+-]+)(,\s*"value2": )(-?[\d.e+-]+)',
                  repl, text)


def prefab(name, spec, tree_guid, bt_guid):
    path = PREFAB_DIR / f'{name}.prefab'
    meta = Path(str(path) + '.meta')
    old_guid = asset_guid(meta) if meta.exists() else None
    run('tools.scene', 'copy-prefab', HYENA_PREFAB.relative_to(REPO).as_posix(), '--name', name, '--force')
    if old_guid:
        new_guid = asset_guid(meta)
        meta.write_bytes(meta.read_bytes().replace(new_guid.encode(), old_guid.encode()))
    text = path.read_bytes().decode('utf-8')
    hyena_tree = asset_guid(REPO / 'Assets/Animations/Enemy/Hyena.animTree.meta')
    hyena_bt = asset_guid(Path(str(HYENA_BT) + '.meta'))
    hyena_mv1 = asset_guid(MODEL_DIR / 'Hyenas' / 'Hyenas_A4_AllMotion.mv1.meta')
    for a, b in ((hyena_tree, tree_guid), (hyena_bt, bt_guid), (hyena_mv1, asset_guid(MODEL_DIR / name / f'{name}.mv1.meta'))):
        assert a in text, a
        text = text.replace(a, b)
    text = text.replace('"Hyena"', f'"{name}"')
    k = spec['size']
    text = re.sub(r'("radius_": )([\d.]+)', lambda m: f'{m.group(1)}{float(m.group(2)) * k}', text)
    text = re.sub(r'("height_": )([\d.]+)', lambda m: f'{m.group(1)}{float(m.group(2)) * k}', text)
    text = scale_block(text, 'offset_', k)
    text = scale_block(text, 'localPos_', k)
    text = scale_block(text, 'size_', k)
    text = re.sub(r'("value_": )60(\s*\})', rf'\g<1>{spec["health"]}\2', text)
    text = re.sub(r'("ptr_wrapper": \{\s*"id": \d+,\s*"data": \{\s*"value0": \{\s*"value_": )60(\s*\})',
                  rf'\g<1>{spec["health"]}\2', text)
    if spec['boss']:
        text = wrap_boss(text, spec)
    else:
        text = text.replace('"polymorphic_name": "GamePlay::Npc::Enemy::Hyena"', f'"polymorphic_name": "{spec["cls"]}"')
    path.write_bytes(text.encode('utf-8'))
    # NOTE: 位置を書き換えたので worldMatrix_ も焼き直す (エンジンは読み込み時に計算し直さない)
    from tools.common.cereal_json import to_file_bytes
    from tools.scene import reader, writer
    from grassland_nature_scatter import bake_world_matrices
    model = reader.read_prefab_file(path)
    bake_world_matrices(model.root)
    path.write_bytes(to_file_bytes(writer.write_prefab(model)))
    res = subprocess.run([sys.executable, '-m', 'tools.scene', 'validate', path.relative_to(REPO).as_posix()],
                         cwd=REPO, capture_output=True, text=True)
    if res.returncode != 0:
        raise SystemExit(f'{path.name}: validate failed\n{res.stdout}{res.stderr}')
    return asset_guid(meta)


def wrap_boss(text, spec):
    """ハイエナの部品 {ccv 0, value0: EnemyBase} を {ccv 0, value0: {ccv 1, value0: EnemyBase, bossName_}} にする"""
    i = text.index('"polymorphic_name": "GamePlay::Npc::Enemy::Hyena"')
    text = text[:i] + f'"polymorphic_name": "{spec["cls"]}"' + text[i + len('"polymorphic_name": "GamePlay::Npc::Enemy::Hyena"'):]
    d = text.index('"data": {', i)
    v = text.index('"value0": {', d)
    # value0 (EnemyBase) の閉じ括弧
    j, depth = text.index('{', v), 0
    k = j
    while True:
        if text[k] == '{':
            depth += 1
        elif text[k] == '}':
            depth -= 1
            if depth == 0:
                break
        k += 1
    enemy_base = text[j:k + 1]
    nl = '\r\n' if '\r\n' in text else '\n'
    boss = ('{' + nl + '"cereal_class_version": 1,' + nl + '"value0": ' + enemy_base + ',' + nl
            + f'"bossName_": "{spec["boss"]}"' + nl + '}')
    return text[:j] + boss + text[k + 1:]


# ---------------------------------------------------------------- ファクトリ
def factory(prefabs):
    raw = FACTORY.read_bytes()
    crlf = b'\r\n' in raw[:200]
    meta = json.loads(raw.decode('utf-8-sig'))
    data = meta['value0']['ptr_wrapper']['data']
    data['cereal_class_version'] = 2
    template = data['tyrannosaurusPrefab_']
    for key, name in (('desertScorpionPrefab_', 'DesertScorpion'), ('sandWormPrefab_', 'SandWorm'),
                      ('skeletonDragonPrefab_', 'SkeletonDragon')):
        blob = json.loads(json.dumps(template))
        blob.pop('cereal_class_version', None)
        inner = blob['value0']['ptr_wrapper']
        inner['data'].pop('cereal_class_version', None)
        inner['data']['value0']['value_'] = prefabs[name]
        data[key] = blob
    ids = [0x80000000 | 1]
    def renum(o):
        if isinstance(o, dict):
            for key2, val in o.items():
                if key2 == 'id' and isinstance(val, int):
                    ids[0] += 1
                    o[key2] = ids[0]
                else:
                    renum(val)
        elif isinstance(o, list):
            for val in o:
                renum(val)
    renum(meta['value0']['ptr_wrapper']['data'])
    text = json.dumps(meta, indent=4, ensure_ascii=False)
    if crlf:
        text = text.replace('\n', '\r\n')
    FACTORY.write_bytes(text.encode('utf-8'))


def main():
    if '--storm-only' in sys.argv[1:]:
        # 作り直さずに、今の骸竜の AnimTree と BT へ砂嵐のギミックだけを足す
        add_storm_states('SkeletonDragon', ENEMIES['SkeletonDragon'])
        add_storm_gimmick(BT_DIR / 'SkeletonDragon.enemyBehaviourData')
        return
    prefabs = {}
    for name, spec in ENEMIES.items():
        tree = anim_tree(name, spec)
        bt = behaviour(name, spec)
        prefabs[name] = prefab(name, spec, tree, bt)
        print(f'{name}: animTree {tree}  bt {bt}  prefab {prefabs[name]}')
    factory(prefabs)
    print(f'wrote {FACTORY.relative_to(REPO)}')


if __name__ == '__main__':
    main()
