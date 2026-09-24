"""アイテムを使うモーションを両アバターのアニメーションツリーに足し、ItemData に使い方(モーションと時刻)を書く。

    python tools/art/item_use_anim.py

クリップは Mixamo から各キャラのリグで書き出したもの (SwordMan = Brute, MagicCaster = Knight D Pelegrini, 30fps):
- ItemDrink.mv1 : "Drinking"               左手のカップを口へ運んで飲む (f60 持ち上げ / f100-145 飲む / f170 下ろす)
- ItemPlace.mv1 : "Putting Down An Object" かがんで右手で床に置く   (f165 床に着く)
Eat は Drinking の同じ区間を速く回す (Mixamo に食べる動きが無いため)。

ステート番号は SwordManAvatarAnimation.h / MagicCasterAvatarAnimation.h の ItemDrink/ItemEat/ItemPlace と一致させる。
既にある名前のノードは足さない (再実行しても増えない)。
"""
from __future__ import annotations

import json
import re
import subprocess
import sys
import tempfile
import uuid
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ITEM_DIR = REPO / 'Assets/Data/Item'
ITEM_DATA_VERSION = 5
SWITCH_ANY_TIME = 100.0
TRANSITION_SECS = 0.2
BLEND_OUT_SECS = 0.1   # ステートはクリップの終わりより少し前に抜けて Idle へ混ぜる

# 名前: (クリップ, 開始フレーム, 終了フレーム, 速度 (フレーム/秒), 効果フレーム)
MOTIONS = {
    'Drink': ('ItemDrink', 54, 180, 45.0, 120),
    'Eat':   ('ItemDrink', 56, 176, 60.0, 110),
    'Place': ('ItemPlace', 60, 250, 66.0, 165),
}
# ItemUseMotion の値 (Assets/Scripts/Core/Game/PlayerAvatar/Item/ItemUseMotion.h)
USE_MOTION = {'Instant': 0, 'Drink': 1, 'Eat': 2, 'Place': 3}

TREES = {
    'SwordManAnimation': {'anim_dir': 'Assets/Art/Animation/Man', 'state': {'Drink': 60, 'Eat': 61, 'Place': 62},
                          'pos': (720, 200)},
    'MagicCasterAnimation': {'anim_dir': 'Assets/Art/Animation/MagicCaster', 'state': {'Drink': 10, 'Eat': 11, 'Place': 12},
                             'pos': (720, 200)},
}

ITEMS = {'Herb': 'Eat', 'HealPotion': 'Drink', 'RoastedMeat': 'Eat', 'LargeBarrelBomb': 'Place'}


def timing(motion):
    _, start, end, speed, effect = MOTIONS[motion]
    return round((effect - start) / speed, 2), round((end - start) / speed - BLEND_OUT_SECS, 2)


def run(*cmd):
    res = subprocess.run([sys.executable, '-m', *cmd], cwd=REPO, capture_output=True, text=True)
    if res.returncode != 0:
        raise SystemExit(f"FAILED: {' '.join(cmd)}\n{res.stdout}\n{res.stderr}")
    return res.stdout


def build_tree(name, spec):
    path = f'Assets/Animations/{name}.animTree'
    shown = run('tools.animtree', 'show', path)
    any_state = re.search(r'^AnyState\s+([0-9A-F-]{36})', shown, re.M).group(1)
    ops = []
    x, y = spec['pos']
    for i, (motion, (clip, start, end, speed, _)) in enumerate(MOTIONS.items()):
        node_name = f'Item{motion}'
        if f'"{node_name}"' in shown:
            print(f'{name}: {node_name} already present')
            continue
        guid = str(uuid.uuid4()).upper()
        ops += [
            {'op': 'add-clip-node', 'name': node_name, 'clip': f"{spec['anim_dir']}/{clip}.mv1", 'speed': speed,
             'blend_offset_secs': SWITCH_ANY_TIME, 'is_loop': False, 'clip_start_time': start, 'clip_end_time': end,
             'guid': guid, 'pos': [x, y + i * 70]},
            {'op': 'add-transition', 'from': any_state, 'next': guid, 'any_state': True,
             'duration_secs': TRANSITION_SECS},
            {'op': 'add-condition', 'any_state': True, 'from': any_state, 'next': guid,
             'name': 'State', 'kind': 'int', 'value': spec['state'][motion]},
        ]
    if not ops:
        return
    with tempfile.NamedTemporaryFile('w', suffix='.json', delete=False, encoding='utf-8') as f:
        json.dump(ops, f)
    print(run('tools.animtree', 'apply', path, f.name).strip())
    print(run('tools.animtree', 'validate', path).strip())


def wire_items():
    for item, motion in ITEMS.items():
        path = ITEM_DIR / f'{item}.itemData.meta'
        raw = path.read_bytes()
        bom = raw.startswith(b'\xef\xbb\xbf')
        text = raw.decode('utf-8-sig').replace('\r\n', '\n')
        effect_secs, total_secs = timing(motion)
        fields = {'useMotion_': str(USE_MOTION[motion]), 'useEffectTime_secs_': repr(effect_secs),
                  'useTotalDuration_secs_': repr(total_secs)}
        text = re.sub(r'("cereal_class_version": )\d+', rf'\g<1>{ITEM_DATA_VERSION}', text, count=1)
        for key, value in fields.items():
            if f'"{key}"' in text:
                text = re.sub(rf'("{key}": )[^,\n]+', rf'\g<1>{value}', text)
            else:
                end = text.rindex('\n            }\n        }\n    }\n}')
                text = text[:end] + f',\n                "{key}": {value}' + text[end:]
        path.write_bytes((b'\xef\xbb\xbf' if bom else b'') + text.replace('\n', '\r\n').encode('utf-8'))
        print(f'wired {path.relative_to(REPO)} -> {motion} (effect {effect_secs}s / total {total_secs}s)')


def main():
    for name, spec in TREES.items():
        build_tree(name, spec)
    wire_items()


if __name__ == '__main__':
    main()
