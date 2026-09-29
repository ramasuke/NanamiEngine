"""操作ヒントの札を、キーボードとパッドで差し替えるための絵と取り付け。

    python tools/art/device_hint.py --emit      # Assets/Art/UI/Hint へパッドの札を書き出す
    python tools/art/device_hint.py --patch     # 店 / 設定 / 帰還の貼り紙 / 掲示板 / キャラ選択のプレハブに取り付ける

パッドの札は「真鍮の鋲」(A / B は丸い鋲、LB RB は小さな板)。キーボードの札と同じ大きさの透明な絵の右寄せに描くので、
差し替えてもラベルの位置は動かない。矢印 (▲▼ / ◀▶) はどちらの機器でも同じ札を使い、差し替えない。
決定は Enter / A、やめるは Esc / B、分類や頁の切り替えは Q E / LB RB。

取り付けは UiFlow::DeviceHint。札の Renderer と同じ GameObject に付け、keyboardSprite_ / gamepadSprite_ を指す。
--patch は既存のプレハブを組み直さず、操作ヒントの所だけを書き換える (手で調整したほかの値を保つ)。

Requires Pillow + numpy.
"""
import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from character_select import BODY_FONT, BRASS, fbm, font, grid, hint_tag, rgba, soften, text, write_sprite  # noqa: E402

EMIT_DIR = REPO / 'Assets' / 'Art' / 'UI' / 'Hint'
UI_DIR = REPO / 'Assets' / 'Art' / 'UI'
TAG_H = 42
STUD_SIZE = 40
PLATE_H = 34
ENGRAVE = (40, 24, 12, 255)
ENGRAVE_LIGHT = (250, 226, 170, 150)

# 役割ごとの札の幅と、パッドのときの絵
ROLES = {
    'confirm': {'width': 68, 'gamepad': 'HintPad_A'},
    'cancel': {'width': 68, 'gamepad': 'HintPad_B'},
    'tab': {'width': 84, 'gamepad': 'HintPad_LBRB'},
    # 見出しの両脇に 1 つずつ置く札
    'tab_prev': {'width': 52, 'gamepad': 'HintPad_LB'},
    'tab_next': {'width': 52, 'gamepad': 'HintPad_RB'},
}
# キーボードのときの絵。同じ用途の既存の札を使い回す (設定と帰還の貼り紙は自分の絵を持つ)
KEYBOARD_SPRITES = {
    'confirm': UI_DIR / 'StageReturn' / 'StageReturn_Hint_Enter.png',
    'cancel': UI_DIR / 'StageReturn' / 'StageReturn_Hint_Esc.png',
    'tab': UI_DIR / 'Settings' / 'Settings_Hint_Tab.png',
    'tab_prev': EMIT_DIR / 'HintKey_Q.png',
    'tab_next': EMIT_DIR / 'HintKey_E.png',
}


# ---------------------------------------------------------------- 絵
def engrave_label(part, label, px):
    w, h = part.size
    text(part, (w / 2 + 1, h / 2 + 1), label, px, ENGRAVE_LIGHT, BODY_FONT, anchor='mm')
    text(part, (w / 2, h / 2), label, px, ENGRAVE, BODY_FONT, anchor='mm')


def stud(label, seed, size=STUD_SIZE):
    """真鍮の丸い鋲。左上から光が当たり、文字は彫り込み"""
    xx, yy = grid(size, size)
    c = (size - 1) / 2
    r = size / 2 - 1.5
    d = np.hypot(xx - c, yy - c)
    mask = soften(np.clip(r - d + 0.5, 0, 1), 0.5)
    n = fbm(size, size, seed, octaves=4, base=6)
    light = np.clip(0.5 - ((xx - c) * 0.55 + (yy - c) * 0.8) / (r * 2), 0, 1)
    rim = np.clip((d - (r - 4.5)) / 4.5, 0, 1)
    tone = 0.55 + 0.75 * light + 0.18 * (n - 0.5) - 0.38 * rim
    part = rgba(BRASS[None, None, :] * tone[..., None] * 1.25, mask)
    engrave_label(part, label, 22)
    return part


def plate(w, label, seed, h=PLATE_H):
    """真鍮の小さな板 (LB / RB)"""
    xx, yy = grid(w, h)
    rad = 9.0
    qx = np.abs(xx - (w - 1) / 2) - ((w - 1) / 2 - rad - 1.5)
    qy = np.abs(yy - (h - 1) / 2) - ((h - 1) / 2 - rad - 1.5)
    d = np.hypot(np.maximum(qx, 0), np.maximum(qy, 0)) + np.minimum(np.maximum(qx, qy), 0) - rad
    mask = soften(np.clip(0.5 - d, 0, 1), 0.5)
    n = fbm(w, h, seed, octaves=4, base=6)
    light = np.clip(0.5 - (yy - (h - 1) / 2) / h * 1.1, 0, 1)
    rim = np.clip((d + 4.5) / 4.5, 0, 1)
    tone = 0.6 + 0.6 * light + 0.18 * (n - 0.5) - 0.38 * rim
    part = rgba(BRASS[None, None, :] * tone[..., None] * 1.25, mask)
    engrave_label(part, label, 18)
    return part


def on_canvas(part, w, h=TAG_H, centered=False):
    """札と同じ大きさの透明な絵の右寄せに置く (ラベルとの間を保つ)。ラベルの無い札は centered"""
    canvas = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    x = (w - part.width) // 2 if centered else w - part.width
    canvas.alpha_composite(part, (x, (h - part.height) // 2))
    return canvas


def shoulder_pair(w):
    canvas = Image.new('RGBA', (w, TAG_H), (0, 0, 0, 0))
    half = (w - 4) // 2
    top = (TAG_H - PLATE_H) // 2
    canvas.alpha_composite(plate(half, 'LB', 430), (0, top))
    canvas.alpha_composite(plate(half, 'RB', 440), (w - half, top))
    return canvas


def sprites():
    return {
        'HintPad_A': on_canvas(stud('A', 410), ROLES['confirm']['width']),
        'HintPad_B': on_canvas(stud('B', 420), ROLES['cancel']['width']),
        'HintPad_LBRB': shoulder_pair(ROLES['tab']['width']),
        'HintPad_LB': on_canvas(plate(46, 'LB', 430), ROLES['tab_prev']['width'], centered=True),
        'HintPad_RB': on_canvas(plate(46, 'RB', 440), ROLES['tab_next']['width'], centered=True),
        'HintKey_Q': hint_tag(ROLES['tab_prev']['width'], TAG_H, 'Q'),
        'HintKey_E': hint_tag(ROLES['tab_next']['width'], TAG_H, 'E'),
    }


def emit(out_dir=EMIT_DIR):
    out_dir.mkdir(parents=True, exist_ok=True)
    print(f'emit -> {out_dir}')
    for name, image in sprites().items():
        write_sprite(out_dir, name, image)
    emit_settings_keys()


def emit_settings_keys():
    """設定のキーボードの札のうち、表記を替えた / 足したものだけを書き出す (ほかの絵は触らない)"""
    import settings_screen

    names = [f'Settings_Hint_{key}' for key in ('Tab', 'Cancel')]
    print(f'emit -> {settings_screen.EMIT_DIR}')
    for name, image in settings_screen.sprites().items():
        if name in names:
            write_sprite(settings_screen.EMIT_DIR, name, image)


# ---------------------------------------------------------------- 取り付け
def gamepad_sprite(role):
    return EMIT_DIR / f'{ROLES[role]["gamepad"]}.png'


def keyboard_sprite(role):
    return KEYBOARD_SPRITES[role]


def sprite_guid(path):
    from game_over_prefab import asset_guid

    return asset_guid(str(path) + '.meta')


def attach(b, node, role, keyboard_sprite_guid=None):
    """node (札の Renderer がある GameObject) に DeviceHint を付ける。b は game_over_prefab.Builder"""
    comp = b.component(node, 'DeviceHint')
    b.field(comp, 'keyboardSprite_', keyboard_sprite_guid or sprite_guid(keyboard_sprite(role)))
    b.field(comp, 'gamepadSprite_', sprite_guid(gamepad_sprite(role)))
    return comp


def swap_tag(b, node, role):
    """既にある札の絵をキーボードのものに替え、DeviceHint を付ける"""
    renderer = node.components[0]
    b.field(renderer, 'spriteFile_', sprite_guid(keyboard_sprite(role)))
    if not any(c.fqn.endswith('::DeviceHint') for c in node.components):
        attach(b, node, role)


# ---------------------------------------------------------------- 既存のプレハブの書き換え
def find_child(node, name):
    return next((c for c in node.transform.children if c.name == name), None)


def find_node(node, name):
    if node.name == name:
        return node
    for child in node.transform.children:
        found = find_node(child, name)
        if found is not None:
            return found
    return None


def parent_of(root, target):
    for child in root.transform.children:
        if child is target:
            return root
        found = parent_of(child, target)
        if found is not None:
            return found
    return None


def world_of(root, target):
    """target のワールドの (拡縮, 位置)。回転は使っていない前提 (game_over_prefab.bake_world_matrices と同じ)"""
    from tools.scene import edits

    def walk(node, scale, pos):
        t = node.transform
        lp = edits._vec3_floats(t.local_pos)
        ls = edits._vec3_floats(t.local_scale)
        ws = tuple(scale[i] * ls[i] for i in range(3))
        wp = tuple(pos[i] + scale[i] * lp[i] for i in range(3))
        if node is target:
            return ws, wp
        for child in t.children:
            found = walk(child, ws, wp)
            if found is not None:
                return found
        return None

    return walk(root, (1.0, 1.0, 1.0), (0.0, 0.0, 0.0))


def bake_new(root, nodes):
    """新しく足した所だけ、ワールド行列を焼く。

    版キーは edits の既定のままにする (let_writer_place_versions は呼ばない)。読み込んだファイルの既存の
    Field<T> は writer から見ると別のキーなので、任せると 2 回目以降にも版キーを書いて読み込みを壊す
    """
    from game_over_prefab import bake_world_matrices

    for node in nodes:
        scale, pos = world_of(root, parent_of(root, node))
        bake_world_matrices(node, scale, pos)


def rebuild_hints(target_root, holder_name, build):
    """holder_name の下の Hints を捨てて build(parent) で組み直す。ほかからの参照が無い画面だけに使う"""
    holder = find_node(target_root, holder_name) if holder_name else target_root
    old = find_child(holder, 'Hints')
    if old is not None:
        holder.transform.children.remove(old)
    hints = build(holder)
    bake_new(target_root, [hints])


def patch_shop():
    import shop_prefab
    from game_over_prefab import Builder
    from tools.scene import reader

    path = shop_prefab.UI_PREFAB_DIR / 'ShopUI.prefab'
    prefab = reader.read_prefab_file(path)
    b = Builder(prefab)
    rebuild_hints(prefab.root, None, lambda parent: shop_prefab.build_hints(b, parent))
    write_prefab(path, prefab)


def patch_settings():
    import settings_prefab
    import settings_screen
    from game_over_prefab import Builder
    from tools.scene import reader

    path = settings_prefab.PREFAB_DIR / 'SettingsScreen.prefab'
    prefab = reader.read_prefab_file(path)
    b = Builder(prefab)
    L = settings_screen.layout()
    rebuild_hints(prefab.root, 'Visual', lambda parent: settings_prefab.build_hints(b, parent, L))
    write_prefab(path, prefab)


def patch_stage_return_tree(b, root):
    """Click (Button) を StageReturnNoticeUi が参照しているので、組み直さずに札へ足すだけにする"""
    import stage_return_prefab

    hints = find_node(root, 'Hints')
    for key, role in stage_return_prefab.HINT_ROLES.items():
        tag = find_child(find_child(hints, key), 'Tag')
        if any(c.fqn.endswith('::DeviceHint') for c in tag.components):
            continue
        attach(b, tag, role, stage_return_prefab.sprite_guid(f'StageReturn_Hint_{key}'))


def patch_stage_return():
    import stage_return_prefab
    from game_over_prefab import Builder
    from tools.scene import reader

    path = stage_return_prefab.UI_PREFAB_DIR / 'StageReturnUI.prefab'
    prefab = reader.read_prefab_file(path)
    patch_stage_return_tree(Builder(prefab), prefab.root)
    write_prefab(path, prefab)

    scene_path = REPO / 'Assets' / 'Scene' / 'StageReturnUiScene.scene'
    scene = reader.read_scene_file(scene_path)
    for root in scene.roots:
        if find_node(root, 'Hints') is not None:
            patch_stage_return_tree(Builder(scene), root)
    write_scene(scene_path, scene)


def add_screen(b, root, screen_id):
    if any(c.fqn.endswith('::UiScreen') for c in root.components):
        return
    b.component(root, 'UiScreen', screenId_=screen_id, locksPlayerControl_='true', destroysOnClose_='true',
                repeatDelay_secs_='0.35', repeatInterval_secs_='0.08')


def patch_event_board():
    import event_board as art
    import event_board_prefab
    from game_over_prefab import Builder
    from tools.scene import edits, reader

    path = event_board_prefab.PREFAB_DIR / 'EventBoardUI.prefab'
    prefab = reader.read_prefab_file(path)
    b = Builder(prefab)
    root = prefab.root
    add_screen(b, root, 'EventBoard')

    swap_tag(b, find_child(root, 'TabHintLB'), 'tab_prev')
    swap_tag(b, find_child(root, 'TabHintRB'), 'tab_next')

    # 組み直さずに、今ある札と文字を動かす。入れ物は EventBoardUi が参照していて、
    # 文字はこのファイルで最初の Field<TtfFontFile> (版キーを持つ) なので、作り直すと読めなくなる
    groups = {'HintsWithAccept': art.V2_HINTS_WITH_ACCEPT, 'HintsWithoutAccept': art.V2_HINTS_WITHOUT_ACCEPT,
              'HintsWithRestore': art.V2_HINTS_WITH_RESTORE}
    for name, items in groups.items():
        group = find_child(root, name)
        for i, item in enumerate(art.v2_hint_layout(items)):
            is_tag = item['kind'] == 'tag'
            node = find_child(group, f'Tag{i}' if is_tag else f'Text{i}')
            edits.set_transform(prefab, node.guid, pos=(float(item['pos'][0]), float(item['pos'][1]), 0.0))
            role = art.HINT_ROLES.get(item['sprite']) if is_tag else None
            if role:
                swap_tag(b, node, role)
        bake_new(root, list(group.transform.children))
    write_prefab(path, prefab)


CHARACTER_SELECT_PREFAB = REPO / 'Assets' / 'Prefab' / 'UI' / 'CharacterSelect' / 'CharacterSelectUI.prefab'
# 右から詰める (札の GameObject, ラベルの GameObject, 役割, 文言)。矢印の札は差し替えない
CHARACTER_SELECT_HINTS = [('HintMove', 'HintMoveText', None, '選ぶ'), ('HintConfirm', 'HintConfirmText', 'confirm', '決める'),
                          ('HintCancel', 'HintCancelText', 'cancel', 'やめる')]
CHARACTER_SELECT_HINT_RIGHT = 1856
HINT_LABEL_PX = 26
HINT_LABEL_GAP = 16
HINT_PAIR_GAP = 30


def patch_character_select():
    """このプレハブは生成スクリプトが無いので、札の所だけを直接書き換える"""
    from game_over_prefab import Builder
    from tools.scene import edits, reader

    prefab = reader.read_prefab_file(CHARACTER_SELECT_PREFAB)
    b = Builder(prefab)
    root = prefab.root
    add_screen(b, root, 'CharacterSelect')

    label_font = font(BODY_FONT, HINT_LABEL_PX)
    x = CHARACTER_SELECT_HINT_RIGHT
    moved = []
    for tag_name, label_name, role, label in reversed(CHARACTER_SELECT_HINTS):
        tag, label_node = find_child(root, tag_name), find_child(root, label_name)
        width = ROLES[role]['width'] if role else 68
        _, tag_y, _ = edits._vec3_floats(tag.transform.local_pos)
        _, label_y, _ = edits._vec3_floats(label_node.transform.local_pos)

        label_left = round(x - label_font.getlength(label))
        tag_right = label_left - HINT_LABEL_GAP
        edits.set_transform(prefab, label_node.guid, pos=(float(label_left), label_y, 0.0))
        edits.set_transform(prefab, tag.guid, pos=(float(tag_right - width / 2), tag_y, 0.0))
        if role:
            swap_tag(b, tag, role)
        moved += [tag, label_node]
        x = tag_right - width - HINT_PAIR_GAP
    bake_new(root, moved)
    write_prefab(CHARACTER_SELECT_PREFAB, prefab)


def check_and_write(path, text, problems):
    from game_over_prefab import check
    from tools.common.cereal_json import to_file_bytes

    check(text, problems, path.name)
    path.write_bytes(to_file_bytes(text))
    print(f'wrote {path.relative_to(REPO)}')


def write_prefab(path, prefab):
    from tools.scene import reader, validate, writer

    check_and_write(path, writer.write_prefab(prefab), validate.validate_prefab(prefab))
    reader.read_prefab_file(path)


def write_scene(path, scene):
    from tools.scene import reader, validate, writer

    check_and_write(path, writer.write_scene(scene), validate.validate_scene(scene))
    reader.read_scene_file(path)


def patch():
    patch_shop()
    patch_settings()
    patch_stage_return()
    patch_event_board()
    patch_character_select()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--emit', action='store_true', help='Assets/Art/UI/Hint へパッドの札を書き出す')
    ap.add_argument('--patch', action='store_true', help='既存のプレハブの操作ヒントを書き換える')
    args = ap.parse_args()
    if args.emit:
        emit()
    if args.patch:
        patch()


if __name__ == '__main__':
    main()
