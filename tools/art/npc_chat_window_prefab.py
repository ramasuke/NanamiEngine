"""Rebuild the NPC chat window in ChattingUiScene.scene from npc_chat_window.py's LAYOUT (案3「黒い板」).

    python tools/art/npc_chat_window.py --emit       # 先にスプライトを書き出す
    python tools/art/npc_chat_window_prefab.py       # ChattingUiScene の NpcChatting の子を組み直す

GameManage の ChattingUISceneContext.npcChatting_ が NpcChatting コンポーネントの guid を指しているので、
ルートの GameObject とコンポーネントはそのまま残し、子だけを作り直す。何度実行しても同じ形になる。
"""
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, to_file_bytes  # noqa: E402
from tools.scene import edits, model, reader, validate, writer  # noqa: E402

import npc_chat_window as art  # noqa: E402
from game_over_prefab import (ALIGN_CENTER, ALIGN_LEFT, BLEND_ALPHA, Builder, all_nodes, asset_guid,  # noqa: E402
                              bake_world_matrices, disable_components)
from room_code_prefab import (ALIGN_RIGHT, FONT_PX, apply_field_versions, check_ignoring_field_versions,  # noqa: E402
                              node_components)

SCENE = REPO / 'Assets' / 'Scene' / 'ChattingUiScene.scene'
ROOT_NAME = 'NpcChatting'
CLASS_VERSION = 3   # pageText_ が増えた版

FONT = asset_guid(REPO / 'Assets/Art/Font/ZenOldMincho-Bold.ttf.meta')
NAIL = asset_guid(str(art.NAIL_SPRITE) + '.meta')
# 他の UI (ゲームオーバーの幕 8000 など) より下、HUD より上
ORDER_BOARD = 7000
ORDER_SLIP = 7001
ORDER_NAIL = 7002
ORDER_TEXT = 7010


def sprite_guid(name):
    return asset_guid(art.EMIT_DIR / f'{name}.png.meta')


def image(b, parent, name, pos, sprite, order):
    node = b.node(parent, name, pos)
    comp = b.component(node, 'BlendImageRenderer', spriteFile_=sprite, renderOrder_=order, blendRate_=255)
    comp.data['blendMode_'] = Num.of_int(BLEND_ALPHA)
    return node, comp


def label(b, parent, name, layout, s, color, align):
    (pos, px) = layout
    node = b.node(parent, name, pos, px / FONT_PX)
    comp = b.component(node, 'TextRenderer', fontFile_=FONT, renderOrder_=ORDER_TEXT, text_=s,
                       isWorldPos_='false', textColor_=','.join(str(c) for c in color))
    comp.data['textAlign_'] = Num.of_int(align)
    return node, comp


def main():
    scene = reader.read_scene_file(SCENE)
    roots = [n for n in scene.roots if n.name == ROOT_NAME]
    if len(roots) != 1:
        raise SystemExit(f'{SCENE.name}: ルートの "{ROOT_NAME}" が {len(roots)} 個あります')
    root = roots[0]
    chat = next((c for c in root.components if c.fqn == 'GamePlay::Ui::NpcChatting'), None)
    if chat is None:
        raise SystemExit(f'{SCENE.name}: {ROOT_NAME} に NpcChatting がありません')

    for child in list(root.transform.children):
        edits.remove_gameobject(scene, child.guid)
    # 子は画面 px でそのまま置けるよう、ルートは原点・等倍にする
    edits.set_transform(scene, root.guid, pos=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0))

    L = art.LAYOUT
    b = Builder(scene)
    image(b, root, 'Board', L['board'], sprite_guid('Chat_Board'), ORDER_BOARD)
    image(b, root, 'NameSlip', L['slip'], sprite_guid('Chat_NameSlip'), ORDER_SLIP)
    image(b, root, 'Nail', L['nail'], NAIL, ORDER_NAIL)
    _, body = label(b, root, 'Text', L['body'], '', art.BODY_COLOR, ALIGN_LEFT)
    _, name = label(b, root, 'Name', L['name'], '', art.NAME_COLOR, ALIGN_CENTER)
    _, pages = label(b, root, 'Pages', L['pages'], '', art.PAGE_COLOR, ALIGN_RIGHT)

    data = chat.data.body if hasattr(chat.data, 'body') else chat.data
    edits._set_field_guid(data['textRenderer_'], model.find_component_guid(body))
    edits._set_field_guid(data['npcNameTextBox_'], model.find_component_guid(name))
    if 'pageText_' in data:
        edits._set_field_guid(data['pageText_'], model.find_component_guid(pages))
    else:
        data['pageText_'] = edits.field_blob('TextRenderer', model.find_component_guid(pages))
    chat.class_version = CLASS_VERSION

    # 元と同じく、会話を出すとき (NpcChatting が Entity を有効にする) まで全部止めておく
    disable_components(root)
    for node in all_nodes(root):
        edits.set_active(scene, node.guid, False)

    bake_world_matrices(root)
    apply_field_versions([c for n in scene.roots for c in node_components(n)], SCENE.name)

    text = writer.write_scene(scene)
    check_ignoring_field_versions(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(SCENE)
    print(f'wrote {SCENE.relative_to(REPO)}  ({ROOT_NAME} {root.guid})')


if __name__ == '__main__':
    main()
