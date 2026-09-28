"""敵を倒したときに落とすお金の DropTable を作り、敵プレハブの EnemyBase.dropTable_ に付ける。

    python tools/art/enemy_drops.py

- Assets/Data/Drop/<Enemy>.dropTable : お金だけ (アイテムは items_ に足せば混ぜられる)
- Assets/Prefab/Npc/Enemy/<Enemy>.prefab : EnemyBase を版 6 にして dropTable_ を足す (既にあれば guid を差し替える)

.meta (asset guid) は既存があれば保つので、額を変えて組み直してもプレハブ側の参照は切れない。
プレハブはテキストに差し込む (tools.scene の catalog は EnemyBase の版上げを扱えない)。
"""
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from loot_prefabs import DROP_DIR, existing_guid, ordered, write_json, _set_key  # noqa: E402

ENEMY_PREFABS = REPO / 'Assets' / 'Prefab' / 'Npc' / 'Enemy'
ENEMY_BASE_VERSION = 6

# (プレハブ名, 合計額, コインの枚数)
ENEMY_DROPS = [
    ('Hyena',            15,  3),
    ('DesertScorpion',   20,  4),
    ('FirstEventDragon', 200, 10),
    ('SandWorm',         150, 10),
    ('Tyrannosaurus',    300, 12),
    ('SkeletonDragon',   400, 15),
    ('AncientDragon',    800, 20),
]


def build_drop_table(name, money, coins):
    meta = DROP_DIR / f'{name}.dropTable.meta'
    guid = existing_guid(meta)
    obj = ordered((DROP_DIR / 'Barrel.dropTable.meta').read_text(encoding='utf-8'))
    data = obj['value0']['ptr_wrapper']['data']
    data['value0']['contentPath_'] = f'Assets\\Data\\Drop/{name}.dropTable'
    data['value0']['guid_']['value_'] = guid
    _set_key(data['money_'], 'value_', money)
    data['moneyPickupCount_'] = coins
    data['items_'] = []
    write_json(meta, obj)
    (DROP_DIR / f'{name}.dropTable').write_bytes(b'')
    print(f'wrote Assets/Data/Drop/{name}.dropTable  ({money} / {coins} coins, asset guid {guid})')
    return guid


def field_blob(indent, ptr_id, guid):
    lines = [
        '"dropTable_": {',
        '    "cereal_class_version": 0,',
        '    "value0": {',
        '        "polymorphic_id": 1073741824,',
        '        "ptr_wrapper": {',
        f'            "id": {ptr_id},',
        '            "data": {',
        '                "cereal_class_version": 0,',
        '                "value0": {',
        f'                    "value_": "{guid}"',
        '                }',
        '            }',
        '        }',
        '    }',
        '}',
    ]
    return '\n'.join(indent + line for line in lines)


def block_end(text, key_pos, indent):
    """key_pos から始まる "key": { ... } の閉じ括弧の直後の位置"""
    m = re.compile(r'\r?\n' + re.escape(indent) + r'\}').search(text, key_pos)
    assert m, 'closing brace not found'
    return m.end()


def patch_prefab(name, guid):
    path = ENEMY_PREFABS / f'{name}.prefab'
    raw = path.read_bytes()
    bom = raw.startswith(b'\xef\xbb\xbf')
    text = raw.decode('utf-8-sig')
    newline = '\r\n' if '\r\n' in text else '\n'

    keys = [m for m in re.finditer(r'^([ \t]*)"lockOnPoint_": \{', text, re.M)]
    assert len(keys) == 1, f'{name}: expected one lockOnPoint_, found {len(keys)}'
    indent = keys[0].group(1)

    existing = re.compile(r'^' + re.escape(indent) + r'"dropTable_": \{', re.M).search(text, keys[0].end())
    lock_end = block_end(text, keys[0].end(), indent)
    if existing and existing.start() - lock_end < 16:
        # 組み直し: guid だけ差し替える
        end = block_end(text, existing.end(), indent)
        body = re.sub(r'"value_": "[0-9A-Fa-f-]+"', f'"value_": "{guid}"', text[existing.start():end])
        text = text[:existing.start()] + body + text[end:]
    else:
        ptr_id = max(int(i) for i in re.findall(r'"id": (\d+)', text)) + 1
        blob = field_blob(indent, ptr_id, guid).replace('\n', newline)
        text = text[:lock_end] + ',' + newline + blob + text[lock_end:]

        # EnemyBase の版: lockOnPoint_ と同じ字下げで、それより前にある最後の cereal_class_version
        versions = list(re.finditer(r'^' + re.escape(indent) + r'"cereal_class_version": (\d+)', text[:keys[0].start()], re.M))
        assert versions and versions[-1].group(1) == '5', f'{name}: EnemyBase version not found'
        v = versions[-1]
        text = text[:v.start(1)] + str(ENEMY_BASE_VERSION) + text[v.end(1):]

    json.loads(text)
    path.write_bytes((b'\xef\xbb\xbf' if bom else b'') + text.encode('utf-8'))
    print(f'patched Assets/Prefab/Npc/Enemy/{name}.prefab')


def main():
    for name, money, coins in ENEMY_DROPS:
        patch_prefab(name, build_drop_table(name, money, coins))


if __name__ == '__main__':
    main()
