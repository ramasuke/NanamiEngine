"""敵 BT の PhysicsAttack / ChargeRush に攻撃予兆 (warning_ / warningBoneName_ / warningBoneOffset_) を書き込む。

    python tools/art/attack_warning_bt.py [--dry-run]

PhysicsAttack v7 / ChargeRush v2 で増えたフィールドは古い blob に無いので tools.bt set-params では足せない。
cereal_json で直接書き、型ごとに最初の cereal_class_version を上げ、新しい FIELD には重複しない ptr id を振る。
Field<EnemyAttackWarning> はファイル内で初めて出る型なので、最初の1つだけ Field / FieldHolder に版番号を付ける。
既に予兆を持つノードは書き直す (何度流してもよい)。

予兆の見た目・音・何秒前に出すかは Assets/Data/Enemy/AttackWarning/*.enemyAttackWarning (EnemyAttackWarning) にまとめてある。
色: 金 = 通常攻撃、赤 = 突進 (ChargeRush) と FirstEventDragon の尻尾振り。
ボスではない敵は金の 1/3 サイズ (AttackWarningGoldSmall) を使う。
ボーンは .mv1 のフレーム名から攻撃する部位を選んだもの。AnimationView で測り直したらここを直して流し直す。
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))

from tools.common.cereal_json import Num, OrderedObj, dumps, loads, read_text, to_file_bytes  # noqa: E402
from tools.common.meta_base import read_guid  # noqa: E402

BT_DIR = REPO / 'Assets/Data/EnemyBehaviour'
ACTION_NS = 'GameCore::Npc::Enemy::Behaviour::Action::'
# 型 -> 予兆フィールドを足した版
TYPES = {'PhysicsAttack': 7, 'ChargeRush': 2}
PTR_FLAG = 0x80000000
# 以前の書き方 (予兆をアクションに直接持たせていた) のフィールド。見つけたら消す
LEGACY_FIELDS = ('warningPrefab_', 'warningLead_secs_', 'warningSound_')
WARNING_FIELDS = ('warning_', 'warningBoneName_', 'warningBoneOffset_')


def guid_of(rel):
    return read_guid(REPO / f'{rel}.meta')[0]


GOLD = guid_of('Assets/Data/Enemy/AttackWarning/AttackWarningGold.enemyAttackWarning')
RED = guid_of('Assets/Data/Enemy/AttackWarning/AttackWarningRed.enemyAttackWarning')
GOLD_SMALL = guid_of('Assets/Data/Enemy/AttackWarning/AttackWarningGoldSmall.enemyAttackWarning')

# ファイル -> [(ノード名の条件, ボーン名, 予兆)]。上から最初に当てはまったもの
RULES = {
    'HyenaBehaviour': [(lambda n: True, 'bip01_head', GOLD_SMALL)],
    'DesertScorpion': [(lambda n: True, 'tailSeg5_013', GOLD_SMALL)],
    'SandWorm': [(lambda n: True, 'Bone.019_020', GOLD_SMALL)],
    'SkeletonDragon': [(lambda n: True, 'Bip001 Head_032', GOLD)],
    'FirstEventDragon': [
        (lambda n: n == 'WeggingTail', 'Bone008', RED),
        (lambda n: True, 'Bip001-Head', GOLD),
    ],
    'Tyrannosaurus': [
        (lambda n: 'Rush' in n, 'jt_Head_C', RED),
        (lambda n: 'HeadButt' in n, 'jt_Head_C', GOLD),
        (lambda n: 'Bite' in n, 'jt_Jaw_C', GOLD),
    ],
}


def field_blob(guid, ptr_id, first):
    """FIELD(T) の blob。first ならその型の初出なので Field / FieldHolder に版番号を付ける"""
    version = [('cereal_class_version', Num.of_int(0))] if first else []
    return OrderedObj(version + [('value0', OrderedObj([
        ('polymorphic_id', Num.of_int(1073741824)),
        ('ptr_wrapper', OrderedObj([
            ('id', Num.of_int(ptr_id)),
            ('data', OrderedObj(version + [('value0', OrderedObj([('value_', guid)]))])),
        ])),
    ]))])


def walk(node, visit):
    if isinstance(node, OrderedObj):
        visit(node)
        for v in node.values():
            walk(v, visit)
    elif isinstance(node, list):
        for v in node:
            walk(v, visit)


def rule_for(stem, name):
    for match, bone, warning in RULES.get(stem, []):
        if match(name):
            return bone, warning
    raise SystemExit(f'{stem}: no rule for attack node "{name}"')


def process(path: Path, dry_run: bool):
    doc = loads(read_text(path))
    poly_names: dict[int, str] = {}
    max_ptr = 0
    nodes = []

    def visit(obj):
        nonlocal max_ptr
        pid = obj.get('polymorphic_id')
        if isinstance(pid, Num) and 'polymorphic_name' in obj:
            poly_names[int(pid.value) & ~PTR_FLAG] = obj['polymorphic_name']
        pw = obj.get('ptr_wrapper')
        if isinstance(pw, OrderedObj) and isinstance(pw.get('id'), Num):
            max_ptr = max(max_ptr, int(pw['id'].value) & ~PTR_FLAG)
        action = obj.get('action_')
        if isinstance(action, OrderedObj) and isinstance(obj.get('name_'), str):
            apid = action.get('polymorphic_id')
            name = action.get('polymorphic_name') or (poly_names.get(int(apid.value) & ~PTR_FLAG) if isinstance(apid, Num) else None)
            if name and name.startswith(ACTION_NS) and name[len(ACTION_NS):] in TYPES:
                nodes.append((obj['name_'], name[len(ACTION_NS):], action['ptr_wrapper']['data']))

    walk(doc, visit)
    if not nodes:
        return

    bumped = set()
    first_field = True
    for node_name, type_name, data in nodes:
        if type_name not in bumped and 'cereal_class_version' in data:
            data['cereal_class_version'] = Num.of_int(TYPES[type_name])
            bumped.add(type_name)
        bone, warning_guid = rule_for(path.stem, node_name)
        for key in LEGACY_FIELDS + WARNING_FIELDS:
            data.pop(key, None)
        max_ptr += 1
        data.append('warning_', field_blob(warning_guid, PTR_FLAG | max_ptr, first_field))
        data.append('warningBoneName_', bone)
        data.append('warningBoneOffset_', OrderedObj([(f'value{i}', Num.of_float(0.0)) for i in range(3)]))
        first_field = False
        print(f'{path.stem:18s} {node_name:20s} {type_name:14s} {bone:16s} {"red" if warning_guid == RED else "gold-small" if warning_guid == GOLD_SMALL else "gold"}')

    missing = set(t for _, t, _ in nodes) - bumped
    if missing:
        raise SystemExit(f'{path.name}: no cereal_class_version found for {missing}')
    if not dry_run:
        path.write_bytes(to_file_bytes(dumps(doc)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()
    for path in sorted(BT_DIR.glob('*.enemyBehaviourData')):
        process(path, args.dry_run)


if __name__ == '__main__':
    main()
