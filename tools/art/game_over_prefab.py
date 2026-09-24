"""tools/art/game_over.py の LAYOUT からゲームオーバーのプレハブ / 常駐シーンを組む。

    python tools/art/game_over.py --emit        # 先にスプライトを書き出す
    python tools/art/game_over_prefab.py        # GameOverButton / GameOverDeathCamera / GameOverUI と GameOverScene を組み直す
    python tools/art/game_over_prefab.py --wire # 加えて GameManage.scene の Game.gameOverSceneFile_ を張る

各 prefab / scene の .meta(asset guid)は既存があれば保つので、組み直しても外からの参照は切れない
(中の GameObject / Component の guid は毎回新しくなる)。

tools.scene の CLI では届かない所をここで埋める:
  * enum(BlendImageRenderer::blendMode_ / TextRenderer::textAlign_)は 0 固定で書かれるので数値で直接書く。
    blendMode_ 0 は NoBlend で、blendRate_ を無視して不透明に描いてしまう
  * CineMachineVirtualCamera::priority_ は R4::SerializableReactiveProperty なので {"value": n} の形で書く
  * worldMatrix_ はエンジンが読み込み時に計算し直さないので、親から合成して書く
  * 最初から出ていないよう、Visual 以下のコンポーネントは無効で書いておく(GameOverScreenUi が有効にする)
"""
import argparse
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.blob import Ver  # noqa: E402
from tools.common.cereal_json import Num, OrderedObj, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, meta as scene_meta, model, reader, validate, writer  # noqa: E402

import game_over as art  # noqa: E402

PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'UI' / 'GameOver'
UI_PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'UI'
SCENE_DIR = REPO / 'Assets' / 'Scene'
GAME_MANAGE_SCENE = SCENE_DIR / 'GameManage.scene'

FONT_PX = 60  # ipam.ttf の TtfFontFile は 60px。TextRenderer は scale で縮める
ORDER_VEIL = 8000
ORDER_DIRT = 8010
ORDER_SLAB = 8020
ORDER_EMBER = 8030
ORDER_PLATE = 8031
ORDER_PLATE_LIT = 8032
ORDER_LABEL_SHADOW = 8033
ORDER_LABEL = 8034
ORDER_HINT = 8040
ORDER_HINT_TEXT = 8041
BLEND_ALPHA = 1
ALIGN_LEFT, ALIGN_CENTER = 0, 1
DEATH_CAMERA_DISABLED_PRIORITY = -1


# ---------------------------------------------------------------- アセット guid
def asset_guid(meta_path):
    text = Path(meta_path).read_bytes().decode('utf-8-sig', errors='replace')
    m = re.search(r'"guid_"\s*:\s*\{[^{}]*?"value_"\s*:\s*"([0-9A-Fa-f-]{36})"', text)
    if not m:
        raise SystemExit(f'no guid_ in {meta_path}')
    return m.group(1).upper()


def sprite_guid(name):
    return asset_guid(art.EMIT_DIR / f'{name}.png.meta')


FONT_BODY = asset_guid(REPO / 'Assets/Art/Font/ipam.ttf.meta')
BLACK_MASK = asset_guid(REPO / 'Assets/Art/UI/BlackMask.png.meta')
HINT_MOVE = asset_guid(str(art.HINT_MOVE_SPRITE) + '.meta')
HINT_CONFIRM = asset_guid(str(art.HINT_CONFIRM_SPRITE) + '.meta')
STING_SOUND = asset_guid(REPO / 'Assets/Audio/Physics/Weather/ThunderFar1.mp3.meta')
SLAB_LAND_SOUND = asset_guid(REPO / 'Assets/Audio/Physics/打撃2.mp3.meta')


# ---------------------------------------------------------------- 部品
class Builder:
    def __init__(self, target):
        self.cat = catalog_mod.load()
        self.target = target

    def node(self, parent, name, pos=(0.0, 0.0), scale=1.0):
        """pos は親からの相対。scale は BlendImageRenderer / TextRenderer が x を見るので縦横同じにする"""
        return edits.add_gameobject(self.target, parent=parent.guid if parent is not None else None, name=name,
                                    pos=(float(pos[0]), float(pos[1]), 0.0),
                                    scale=(float(scale), float(scale), 1.0))

    def component(self, node, type_name, **params):
        return edits.add_component(self.target, node.guid, type_name, cat=self.cat,
                                   params={k: str(v) for k, v in params.items()})

    def image(self, parent, name, pos, sprite, order, blend, scale=1.0):
        node = self.node(parent, name, pos, scale)
        comp = self.component(node, 'BlendImageRenderer', spriteFile_=sprite, renderOrder_=order, blendRate_=blend)
        comp.data['blendMode_'] = Num.of_int(BLEND_ALPHA)
        return node, comp

    def text(self, parent, name, pos, px, text, color, order, align):
        node = self.node(parent, name, pos, px / FONT_PX)
        comp = self.component(node, 'TextRenderer', fontFile_=FONT_BODY, renderOrder_=order, text_=text,
                              isWorldPos_='false', textColor_=','.join(str(c) for c in color))
        comp.data['textAlign_'] = Num.of_int(align)
        return node, comp

    @staticmethod
    def field(comp, key, guid):
        edits._set_field_guid(comp.data[key], guid)


def guid_of(comp):
    return model.find_component_guid(comp)


def all_nodes(node):
    yield node
    for child in node.transform.children:
        yield from all_nodes(child)


def disable_components(node):
    for n in all_nodes(node):
        for comp in n.components:
            model.set_component_enabled(comp, False)


# ---------------------------------------------------------------- ワールド行列 / クラスバージョン
def world_matrix_blob(scale, pos):
    cols = [(scale[0], 0.0, 0.0, 0.0), (0.0, scale[1], 0.0, 0.0), (0.0, 0.0, scale[2], 0.0),
            (pos[0], pos[1], pos[2], 1.0)]
    obj = OrderedObj()
    for i, col in enumerate(cols):
        obj[f'value{i}'] = OrderedObj((f'value{j}', Num.of_float(float(v))) for j, v in enumerate(col))
    return obj


def bake_world_matrices(node, parent_scale=(1.0, 1.0, 1.0), parent_pos=(0.0, 0.0, 0.0)):
    """回転は使わないので、world = 親の位置 + 親の拡縮 * 自分の位置、拡縮は掛け算で足りる"""
    t = node.transform
    lp = edits._vec3_floats(t.local_pos)
    ls = edits._vec3_floats(t.local_scale)
    ws = tuple(parent_scale[i] * ls[i] for i in range(3))
    wp = tuple(parent_pos[i] + parent_scale[i] * lp[i] for i in range(3))
    t.world_matrix = world_matrix_blob(ws, wp)
    for child in t.children:
        bake_world_matrices(child, ws, wp)


def let_writer_place_versions(blob):
    """新しく作った/複製した Field<T> / Color32 の版キーは、書き出し順で最初の1回にだけ付ける writer の判断に任せる"""
    if isinstance(blob, Ver):
        leaf = blob.key[1] if isinstance(blob.key, tuple) and len(blob.key) > 1 else ''
        if isinstance(leaf, str) and (leaf.startswith(('Field<', 'FieldHolder<')) or leaf == 'Color32'):
            blob.literal_presence = None
        let_writer_place_versions(blob.body)
    elif isinstance(blob, OrderedObj):
        for _, value in blob.items():
            let_writer_place_versions(value)
    elif isinstance(blob, list):
        for value in blob:
            let_writer_place_versions(value)
    elif hasattr(blob, 'data'):
        let_writer_place_versions(blob.data)


def prepare(roots):
    for root in roots:
        for node in all_nodes(root):
            for comp in node.components:
                let_writer_place_versions(comp.data)
        bake_world_matrices(root)


def check(text, problems, label):
    problems = problems + validate.validate_class_versions(text, catalog_mod.load())
    hard = [p for p in problems if not p.startswith('note:')]
    for p in problems:
        print('  ' + (p if p.startswith('note:') else 'FAIL: ' + p))
    if hard:
        raise SystemExit(f'{label}: validation failed - nothing written')


def save_prefab(prefab, directory, name):
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / f'{name}.prefab'
    prepare([prefab.root])
    text = writer.write_prefab(prefab)
    check(text, validate.validate_prefab(prefab), path.name)
    path.write_bytes(to_file_bytes(text))

    meta = Path(str(path) + '.meta')
    if meta.exists():
        guid = asset_guid(meta)
    else:
        guid = scene_meta.mint_guid().upper()
        content_path = scene_meta.content_path_for(scene_meta.PREFAB_SPEC, name, directory, REPO)
        scene_meta.write_meta(scene_meta.PREFAB_SPEC, meta, name, guid, content_path)
    reader.read_prefab_file(path)  # 読み戻せることだけ確かめる
    print(f'wrote {path.relative_to(REPO)}  (asset guid {guid})')
    return guid, path


def new_prefab(name):
    return model.Prefab(root=edits.new_gameobject(name, kind=model.KIND_PREFAB_ROOT), copied_object_guids=[])


# ---------------------------------------------------------------- プレハブ
def populate_button(b, root, label_text):
    """鉄札1枚を root の下に組む。中心が札の中心"""
    _, ember = b.image(root, 'Ember', (0, 0), sprite_guid('GameOver_PlateEmber'), ORDER_EMBER, 0)
    _, plate = b.image(root, 'Plate', (0, 0), sprite_guid('GameOver_Plate'), ORDER_PLATE, 255)
    _, plate_lit = b.image(root, 'PlateLit', (0, 0), sprite_guid('GameOver_PlateLit'), ORDER_PLATE_LIT, 0)
    L = art.LAYOUT
    _, shadow = b.text(root, 'LabelShadow', L['label_shadow_offset'], L['label_px'], label_text,
                       (6, 8, 10), ORDER_LABEL_SHADOW, ALIGN_CENTER)
    _, label = b.text(root, 'Label', L['label_offset'], L['label_px'], label_text,
                      L['label_color'], ORDER_LABEL, ALIGN_CENTER)

    comp = b.component(root, 'GameOverButton',
                       labelColor_=','.join(map(str, L['label_color'])),
                       labelLitColor_=','.join(map(str, L['label_lit_color'])),
                       highlightFadeSecs_='0.12', emberPulseHz_='0.8',
                       emberMinBlendRate_='110', emberMaxBlendRate_='220')
    b.field(comp, 'plate_', guid_of(plate))
    b.field(comp, 'plateLit_', guid_of(plate_lit))
    b.field(comp, 'ember_', guid_of(ember))
    b.field(comp, 'label_', guid_of(label))
    b.field(comp, 'labelShadow_', guid_of(shadow))
    w, h = L['plate_event_size']
    b.component(root, 'Button', eventAreaSize_=f'{w},{h}')
    return comp


def build_button():
    """書き出した GameOverButton.prefab と、その中身(GameOverUI へ入れ子で置くときに使う)を返す"""
    prefab = new_prefab('GameOverButton')
    populate_button(Builder(prefab), prefab.root, art.LAYOUT['labels'][0])
    save_prefab(prefab, PREFAB_DIR, 'GameOverButton')
    return prefab


def build_death_camera():
    """GrassLand の ArrivalCamera と同じ組み(仮想カメラ + Follow + LookAt + 手持ちの揺れ)に GameOverDeathCamera を足す"""
    prefab = new_prefab('GameOverDeathCamera')
    b = Builder(prefab)
    root = prefab.root

    camera = b.component(root, 'CineMachineVirtualCamera', overrideFov_='false', fov_='60')
    camera.data['priority_'] = OrderedObj([('value', Num.of_int(DEATH_CAMERA_DISABLED_PRIORITY))])
    b.component(root, 'VirtualCameraFollowBehaviour', followOffset_='0,6,-30')
    b.component(root, 'VirtualCameraLookAtBehaviour', lookAtTargetOffset_='0,1.5,0')
    b.component(root, 'NoiseCameraBehaviour', rotationAmplitude_deg_='0.5,0.7,0.25', positionAmplitude_='0,0,0',
                frequency_='0.3', octaves_='3', amplitudeGain_='1', frequencyGain_='1', blendIn_secs_='1.2',
                seed_='31.7,12.9,77.3')
    b.component(root, 'GameOverDeathCamera', priority_='500', shotSecs_='7', orbitDeg_='40', endPitchDeg_='55',
                endDistance_='46', endLookAtOffset_='0,1.5,0')
    return save_prefab(prefab, PREFAB_DIR, 'GameOverDeathCamera')


def button_instance(target, button_prefab, parent, name, pos, label_text):
    """GameOverButton.prefab をエディタで置いたのと同じく CopiedPrefab として入れ子にする。
    ファイルから読み直さず手元の中身から複製するのは、読み直すと元 prefab で最初の出現だった
    Field<T> の版キーがそのまま写り、こちらでは2回目以降の出現なのに版キーが付いて読み込みが壊れるため。

    常駐シーンの UI なので実行時の Instantiate は使えない(生成先がその時のメインシーンになり、遷移で一緒に消える)"""
    node = edits.instantiate_prefab(target, button_prefab, parent=parent.guid)
    node.name = name
    node.transform.local_pos = edits._vec3_from_floats((float(pos[0]), float(pos[1]), 0.0))
    for child in node.transform.children:
        if child.name in ('Label', 'LabelShadow'):
            child.components[0].data['text_'] = label_text
    return next(c for c in node.components if c.fqn.endswith('::GameOverButton'))


def build_ui(button_prefab, death_camera_guid):
    prefab = new_prefab('GameOverUI')
    b = Builder(prefab)
    root = prefab.root
    L = art.LAYOUT

    # 表示の切り替えは Visual ごと SetEnable で行う。GameOverScreenUi / Presenter は常駐させるので root に置く
    visual = b.node(root, 'Visual')
    _, veil = b.image(visual, 'Veil', (art.SCREEN_W / 2, art.SCREEN_H / 2), BLACK_MASK, ORDER_VEIL, 0,
                      scale=L['veil_scale'])
    _, dirt = b.image(visual, 'SlabDirt', L['dirt'], sprite_guid('GameOver_SlabDirt'), ORDER_DIRT, 0)
    _, slab = b.image(visual, 'Slab', L['slab'], sprite_guid('GameOver_Slab'), ORDER_SLAB, 0)

    buttons = [button_instance(prefab, button_prefab, visual, name, pos, label)
               for name, pos, label in zip(('RetryButton', 'TitleButton'), L['buttons'], L['labels'])]

    hints = b.node(visual, 'Hints')
    hint_sprites = {'move': HINT_MOVE, 'confirm': HINT_CONFIRM}
    hint_parts = {}
    for item in art.hint_layout():
        if item['kind'] == 'tag':
            _, comp = b.image(hints, f"{item['name'].capitalize()}Tag", item['pos'], hint_sprites[item['name']],
                              ORDER_HINT, 0)
        else:
            _, comp = b.text(hints, f"{item['name'].capitalize()}Text", item['pos'], L['hint_px'], item['text'],
                             L['hint_color'], ORDER_HINT_TEXT, ALIGN_LEFT)
        hint_parts[(item['name'], item['kind'])] = comp

    disable_components(visual)

    ui = b.component(root, 'GameOverScreenUi',
                     veilBlendRate_='165', veilFadeSecs_='1.6', stingDelaySecs_='0.35',
                     slabDelaySecs_='1.9', slabRiseSecs_='0.42', slabRiseDistance_px_='90', slabOvershoot_px_='14',
                     dirtFadeSecs_='0.12', buttonsDelaySecs_='2.55', buttonsRiseSecs_='0.32',
                     buttonStaggerSecs_='0.08', buttonRiseDistance_px_='26', inputGuardSecs_='0.35',
                     curtainCloseSecs_='0.45', curtainOpenSecs_='0.6')
    b.field(ui, 'visualRoot_', visual.guid)
    b.field(ui, 'veil_', guid_of(veil))
    b.field(ui, 'slab_', guid_of(slab))
    b.field(ui, 'slabDirt_', guid_of(dirt))
    b.field(ui, 'retryButton_', guid_of(buttons[0]))
    b.field(ui, 'titleButton_', guid_of(buttons[1]))
    b.field(ui, 'moveHintTag_', guid_of(hint_parts[('move', 'tag')]))
    b.field(ui, 'moveHintText_', guid_of(hint_parts[('move', 'text')]))
    b.field(ui, 'confirmHintTag_', guid_of(hint_parts[('confirm', 'tag')]))
    b.field(ui, 'confirmHintText_', guid_of(hint_parts[('confirm', 'text')]))
    b.field(ui, 'stingSound_', STING_SOUND)
    b.field(ui, 'slabLandSound_', SLAB_LAND_SOUND)

    presenter = b.component(root, 'GameOverPresenter', fallenConfirmSecs_='0.6', curtainHoldSecs_='0.35')
    b.field(presenter, 'deathCameraPrefab_', death_camera_guid)
    return save_prefab(prefab, UI_PREFAB_DIR, 'GameOverUI')


def build_scene(ui_path):
    """常駐させるシーン。中身は GameOverUI 1つだけ"""
    name = 'GameOverScene'
    path = SCENE_DIR / f'{name}.scene'
    scene = model.Scene(name=name, roots=[])
    edits.instantiate_prefab(scene, reader.read_prefab_file(ui_path), parent=None)
    prepare(scene.roots)
    text = writer.write_scene(scene)
    check(text, validate.validate_scene(scene), path.name)
    path.write_bytes(to_file_bytes(text))

    meta = Path(str(path) + '.meta')
    if meta.exists():
        guid = asset_guid(meta)
    else:
        guid = scene_meta.mint_guid().upper()
        content_path = scene_meta.content_path_for(scene_meta.SCENE_SPEC, name, SCENE_DIR, REPO)
        scene_meta.write_meta(scene_meta.SCENE_SPEC, meta, name, guid, content_path)
    reader.read_scene_file(path)
    print(f'wrote {path.relative_to(REPO)}  (asset guid {guid})')
    return guid


# ---------------------------------------------------------------- GameManage.scene
def wire_game(scene_guid):
    """Game v3 -> v4: gameOverSceneFile_ を末尾に足す(save() の順)"""
    scene = reader.read_scene_file(GAME_MANAGE_SCENE)
    target = None
    for root in scene.roots:
        for node in all_nodes(root):
            for comp in node.components:
                if comp.fqn == 'GameCore::Game':
                    target = comp
    if target is None:
        raise SystemExit('GameCore::Game not found in GameManage.scene')

    if 'gameOverSceneFile_' in target.data:
        edits._set_field_guid(target.data['gameOverSceneFile_'], scene_guid)
    else:
        if target.class_version != 3:
            raise SystemExit(f'expected Game v3, found v{target.class_version}')
        target.class_version = 4
        # stageLoadingSceneFile_ が先にあるので Field<SceneFile> の2回目以降。版キーは付けない
        target.data.append('gameOverSceneFile_', edits.field_blob('SceneFile', scene_guid))

    text = writer.write_scene(scene)
    check(text, validate.validate_scene(scene), GAME_MANAGE_SCENE.name)
    GAME_MANAGE_SCENE.write_bytes(to_file_bytes(text))
    print(f'wrote {GAME_MANAGE_SCENE.relative_to(REPO)}  (gameOverSceneFile_ -> {scene_guid})')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--wire', action='store_true', help='GameManage.scene の Game.gameOverSceneFile_ を張る')
    args = ap.parse_args()

    button_prefab = build_button()
    death_camera_guid, _ = build_death_camera()
    _, ui_path = build_ui(button_prefab, death_camera_guid)
    scene_guid = build_scene(ui_path)
    if args.wire:
        wire_game(scene_guid)


if __name__ == '__main__':
    main()
