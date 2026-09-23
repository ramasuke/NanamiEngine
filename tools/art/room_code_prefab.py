"""Add the stage select "部屋" row (案B) and the in-game room code HUD from room_code.py's LAYOUT.

    python tools/art/room_code.py --emit       # 先にスプライトを書き出す
    python tools/art/room_code_prefab.py       # StageSelectUI.prefab の RoomRow と
                                               # OtherPlayerStatusUiScene.scene の RoomCodeHud を組み直す

既にある prefab に足すので、作り直すのは RoomRow の下だけ。何度実行しても同じ形になる
(前の RoomRow は消してから作り直す)。StageSelectUi.roomUi_ もここで張る。
文字の色・フォント・renderOrder は同じパネルの HeroTitle / HeroLabel に合わせる。
"""
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, model, reader, validate, writer  # noqa: E402

import room_code as art  # noqa: E402
from game_over_prefab import (ALIGN_CENTER, ALIGN_LEFT, Builder, all_nodes, asset_guid,  # noqa: E402
                              bake_world_matrices, check)

PREFAB = REPO / 'Assets' / 'Prefab' / 'UI' / 'StageSelectUI.prefab'
# 部屋番号の帯は、同じくマルチプレイのときだけ出る「他のプレイヤー」の sub scene に置く
HUD_SCENE = REPO / 'Assets' / 'Scene' / 'OtherPlayerStatusUiScene.scene'
ALIGN_RIGHT = 2
FONT_PX = 60
# パネルの文字と同じ帯。絵は出発するボタン(1600)より下、パネルの地(1500)より上。板はパネルの地と同じ
ORDER_TEXT = 10000100
ORDER_IMAGE = 1550
ORDER_PLATE = 1500

GOLD = (230, 200, 120)        # HeroLabel と同じ
WHITE = (245, 235, 210)       # HeroTitle と同じ
DIM = (170, 185, 200)
HUD_LABEL = (210, 228, 223)   # 系統B の淡色
HUD_CODE = (255, 255, 255)
# パネルの文字と同じ IPA明朝 (HeroTitle / HeroLabel が使っている)
FONT = asset_guid(REPO / 'Assets/Art/Font/ipam.ttf.meta')

# 文言は room_code.py (モックと共有) から。番号で入るときは説明文の代わりに数字の枠を出すので空
MODE_NAMES = list(art.MODE_LONG)
MODE_NOTES = [art.MODE_NOTE[0], art.MODE_NOTE[1], '']


def sprite_guid(name):
    return asset_guid(art.EMIT_DIR / f'{name}.png.meta')


FIELD_VERSION_RE = re.compile(r'(Field|FieldHolder)<[^>]+> (has no|carries a stray) cereal_class_version')


def check_ignoring_field_versions(text, problems, label):
    """validate は Field<T> を JSON の形で見分けるので、T が違っても同じ型だと思ってしまう。
    版キーの要否は apply_field_versions が型で決めているので、その手の指摘だけ外して確かめる"""
    check(text, [p for p in problems if not FIELD_VERSION_RE.search(p)], label)


def node_components(node):
    """writer が書き出す順 (その GameObject の component -> 子の順)"""
    for comp in node.components:
        yield comp
    for child in node.transform.children:
        yield from node_components(child)


def field_blobs_in_order(comps):
    """(型 T, Field<T> の blob) を書き出し順に返す。Field<T> は T が違っても JSON の形が
    まったく同じで、版キーの有無でしか区別が付かないので、型はカタログから引くしかない"""
    cat = catalog_mod.load()
    for comp in comps:
        entry = cat.components.get(comp.fqn)
        if not entry:
            continue
        body = comp.data.body if hasattr(comp.data, 'body') else comp.data
        for param in entry['params']:
            if param.get('shape') != 'field' or param['key'] not in body:
                continue
            value = body[param['key']]
            for field in (value if isinstance(value, list) else [value]):
                yield param['type'], field


def holder_of(field):
    """Field<T> の中の FieldHolder<T>"""
    from tools.common.blob import Ptr, Ver

    body = field.body if isinstance(field, Ver) else field
    inner = body['value0'] if 'value0' in body else None
    return inner.data if isinstance(inner, Ptr) else None


def apply_field_versions(comps, label):
    """cereal は型ごとに最初の 1 回だけ cereal_class_version を読む。Field<T> / FieldHolder<T> は
    T ごとに別の型なので、T の初出にだけ版キーを出す (初出でない物に出すと、次の読みがずれる)"""
    from tools.common.blob import Ver

    seen = set()
    for leaf, field in field_blobs_in_order(comps):
        first = leaf not in seen
        seen.add(leaf)
        if not isinstance(field, Ver):
            if first:
                raise SystemExit(f'{label}: Field<{leaf}> の初出に cereal_class_version を付けられません'
                                 ' (前に出ていた物より先に差し込んでいる)')
            continue
        field.literal_presence = first
        if isinstance(holder := holder_of(field), Ver):
            holder.literal_presence = first


def check_version_model(path, read, comps_of):
    """並び順と型の見立てが合っているかを、手を入れる前のファイルで確かめる。
    元のファイルで版キーが付いている Field<T> と、この見立ての初出が一致するはず"""
    from tools.common.blob import Ver

    comps = list(comps_of(read(path)))
    seen, wrong = set(), []
    for index, (leaf, field) in enumerate(field_blobs_in_order(comps)):
        first = leaf not in seen
        seen.add(leaf)
        has_version = isinstance(field, Ver) and field.literal_presence is not False
        if has_version != first:
            wrong.append((index, leaf, has_version, first))
    return wrong


def find_node(root, name):
    for node in all_nodes(root):
        if node.name == name:
            return node
    raise SystemExit(f'{PREFAB.name}: GameObject "{name}" が見つかりません')


def find_component(root, fqn):
    for node in all_nodes(root):
        for comp in node.components:
            if comp.fqn == fqn:
                return comp
    raise SystemExit(f'{PREFAB.name}: component "{fqn}" が見つかりません')


def find_chain(root, target):
    if root is target:
        return [root]
    for child in root.transform.children:
        if chain := find_chain(child, target):
            return [root] + chain
    return None


def world_of(root, node):
    """node のワールドの (拡縮, 位置)。回転は使っていないので bake_world_matrices と同じ掛け算で足りる"""
    scale, pos = (1.0, 1.0), (0.0, 0.0)
    for n in find_chain(root, node):
        lp = edits._vec3_floats(n.transform.local_pos)
        ls = edits._vec3_floats(n.transform.local_scale)
        pos = tuple(pos[i] + scale[i] * lp[i] for i in range(2))
        scale = tuple(scale[i] * ls[i] for i in range(2))
    return scale, pos


class Placer:
    """LAYOUT の画面 px を、親の拡縮で割ってローカルに直して置く。
    Chrome 以下は縦だけ 1/1.1 に縮む。ImageRenderer は x の拡縮だけ見て縦横同じに描き、
    TextRenderer は x と y を別々に見るので、文字は縦を割り戻さないと潰れる"""

    def __init__(self, b, root):
        self.b = b
        self.root = root

    def node(self, parent, name, world_pos=None, px=None):
        (sx, sy), (px0, py0) = world_of(self.root, parent)
        if world_pos is None:
            pos = (0.0, 0.0)
        else:
            pos = ((world_pos[0] - px0) / sx, (world_pos[1] - py0) / sy)
        k = 1.0 if px is None else px / FONT_PX
        return edits.add_gameobject(self.b.target, parent=parent.guid, name=name,
                                    pos=(pos[0], pos[1], 0.0), scale=(k / sx, k / sy, 1.0))


def text_node(b, parent, name, pos, px, s, color, align=ALIGN_LEFT):
    node = b.placer.node(parent, name, pos, px)
    comp = b.component(node, 'TextRenderer', fontFile_=FONT, renderOrder_=ORDER_TEXT, text_=s,
                       isWorldPos_='false', textColor_=','.join(str(c) for c in color))
    comp.data['textAlign_'] = Num.of_int(align)
    return node, comp


def image_node(b, parent, name, pos, sprite, order=ORDER_IMAGE):
    node = b.placer.node(parent, name, pos) if hasattr(b, 'placer') else b.node(parent, name, pos)
    comp = b.component(node, 'ImageRenderer', spriteFile_=sprite, renderPriority_=order)
    return node, comp


def build_row(b, parent):
    L = art.LAYOUT
    row = b.placer.node(parent, 'RoomRow')

    image_node(b, row, 'Plate', L['plate'], sprite_guid('Room_Plate'), order=ORDER_PLATE)

    (label_pos, label_px) = L['label']
    text_node(b, row, 'Label', label_pos, label_px, '部 屋', GOLD)

    (mode_pos, mode_px) = L['mode']
    _, mode_text = text_node(b, row, 'ModeText', mode_pos, mode_px, MODE_NAMES[0], WHITE, align=ALIGN_CENTER)

    arrows = []
    for name, key, sprite in (('ArrowLeft', 'arrow_left', 'Room_Arrow_Left'),
                              ('ArrowRight', 'arrow_right', 'Room_Arrow_Right')):
        node, _ = image_node(b, row, name, L[key], sprite_guid(sprite))
        # Button::eventAreaSize_ は中心からの半分の幅と高さ。指で押しやすいよう絵より広めに取る
        arrows.append(b.component(node, 'Button', eventAreaSize_='22,22'))

    image_node(b, row, 'Rule', L['rule'], sprite_guid('Room_Rule'))

    (note_pos, note_px) = L['note']
    _, note_text = text_node(b, row, 'Note', note_pos, note_px, MODE_NOTES[0], DIM)

    digits_root = b.placer.node(row, 'Digits')
    boxes, digit_texts = [], []
    x0, y0 = L['digits']
    text_px = L['digit_text_px']
    for i in range(art.CODE_LENGTH):
        x = x0 + i * L['digit_pitch'] + (L['digit_group_gap'] if i >= 3 else 0)
        digit = b.placer.node(digits_root, f'Digit{i}', (x, y0))
        _, box = image_node(b, digit, 'Box', (x, y0), sprite_guid('Room_Digit'))
        # 文字は上端が座標なので、枠の中心に来るよう半分上げる
        _, value = text_node(b, digit, 'Value', (x, y0 - text_px / 2), text_px, '', WHITE, align=ALIGN_CENTER)
        boxes.append(box)
        digit_texts.append(value)

    _, hint_text = text_node(b, row, 'Hint', (L['hint_right'], L['hint_y']), 20, '', DIM, align=ALIGN_RIGHT)

    room = b.component(row, 'StageSelectRoomUi')
    b.field(room, 'modeText_', model.find_component_guid(mode_text))
    b.field(room, 'noteText_', model.find_component_guid(note_text))
    b.field(room, 'hintText_', model.find_component_guid(hint_text))
    b.field(room, 'leftArrowButton_', model.find_component_guid(arrows[0]))
    b.field(room, 'rightArrowButton_', model.find_component_guid(arrows[1]))
    b.field(room, 'digitsRoot_', digits_root.guid)
    b.field(room, 'digitSprite_', sprite_guid('Room_Digit'))
    b.field(room, 'digitFocusSprite_', sprite_guid('Room_Digit_Focus'))
    room.data['digitBoxes_'] = [edits.field_blob('ImageRenderer', model.find_component_guid(c)) for c in boxes]
    room.data['digitTexts_'] = [edits.field_blob('TextRenderer', model.find_component_guid(c)) for c in digit_texts]
    room.data['codeLength_'] = Num.of_int(art.CODE_LENGTH)
    room.data['modeNames_'] = list(MODE_NAMES)
    room.data['modeNotes_'] = list(MODE_NOTES)
    room.data['modeHints_'] = list(art.MODE_HINTS)
    room.data['codeIncompleteHint_'] = art.CODE_INCOMPLETE_HINT
    return row, room


def build_hud(scene):
    """OtherPlayerStatusUiScene に「部 屋 番 号 / 482 913」の帯を置く"""
    L = art.HUD_LAYOUT
    for old in [n for n in scene.roots if n.name == 'RoomCodeHud']:
        edits.remove_gameobject(scene, old.guid)

    b = Builder(scene)
    root = b.node(None, 'RoomCodeHud')
    b.placer = Placer(b, root)
    visual = b.node(root, 'Visual')
    image_node(b, visual, 'Plate', L['plate'], sprite_guid('Room_HudPlate'), order=ORDER_IMAGE)

    (label_pos, label_px) = L['label']
    text_node(b, visual, 'Label', label_pos, label_px, '部 屋 番 号', HUD_LABEL)
    (code_pos, code_px) = L['code']
    _, code_text = text_node(b, visual, 'Code', code_pos, code_px, '', HUD_CODE)

    hud = b.component(root, 'RoomCodeHud')
    b.field(hud, 'visualRoot_', visual.guid)
    b.field(hud, 'codeText_', model.find_component_guid(code_text))
    # 番号が来るまでは出さない (RoomCodeHud が有効にする)
    edits.set_active(scene, visual.guid, False)
    return root


def write_hud():
    scene = reader.read_scene_file(HUD_SCENE)
    root = build_hud(scene)

    for node in scene.roots:
        bake_world_matrices(node)
    apply_field_versions([c for n in scene.roots for c in node_components(n)], HUD_SCENE.name)

    text = writer.write_scene(scene)
    check_ignoring_field_versions(text, validate.validate_scene(scene), HUD_SCENE.name)
    HUD_SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(HUD_SCENE)
    print(f'wrote {HUD_SCENE.relative_to(REPO)}  (RoomCodeHud {root.guid})')


def main():
    prefab = reader.read_prefab_file(PREFAB)
    root = prefab.root

    panel = find_node(root, 'HeroPanel')
    for old in [c for c in panel.transform.children if c.name == 'RoomRow']:
        edits.remove_gameobject(prefab, old.guid)

    b = Builder(prefab)
    b.placer = Placer(b, root)
    row, room = build_row(b, panel)
    # 相席が既定なので、番号の枠は StageSelectRoomUi が出すまで隠しておく
    edits.set_active(prefab, find_node(row, 'Digits').guid, False)

    # 出発するボタンは部屋の板の下へ (元はパネルの下端に重ねていた)
    button = find_node(root, 'WorldEnterButton')
    parent_chain = find_chain(root, button)[:-1]
    (_, sy), (_, py) = world_of(root, parent_chain[-1])
    local = edits._vec3_floats(button.transform.local_pos)
    edits.set_transform(prefab, button.guid, pos=(local[0], (art.LAYOUT['depart_button_y'] - py) / sy, local[2]))

    # StageSelectUi.roomUi_ (v7 で増えた欄) を張る
    ui = find_component(root, 'GamePlay::Ui::StageSelectUi')
    body = ui.data.body if hasattr(ui.data, 'body') else ui.data
    if 'roomUi_' in body:
        edits._set_field_guid(body['roomUi_'], model.find_component_guid(room))
    else:
        body['roomUi_'] = edits.field_blob('StageSelectRoomUi', model.find_component_guid(room))
    ui.class_version = 7

    bake_world_matrices(root)
    apply_field_versions(node_components(root), PREFAB.name)

    text = writer.write_prefab(prefab)
    check_ignoring_field_versions(text, validate.validate_prefab(prefab), PREFAB.name)
    PREFAB.write_bytes(to_file_bytes(text))
    reader.read_prefab_file(PREFAB)
    print(f'wrote {PREFAB.relative_to(REPO)}  (RoomRow {row.guid})')

    write_hud()


if __name__ == '__main__':
    main()
