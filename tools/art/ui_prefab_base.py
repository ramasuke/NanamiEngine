"""ImageRenderer / TextRenderer を並べる UI プレハブを組むための共通の道具。

    import ui_prefab_base as base
    base.PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'UI' / '<Ui>'
    b = base.PrefabBuilder('<Name>')
    ...
    base.save_prefab(b, '<Name>')

.meta(asset guid)は既存があれば保つので、組み直しても外からの参照は切れない
(中の GameObject / Component の guid は毎回新しくなる)。

tools.scene の CLI では届かない所をここで埋める:
  * enum(TextRenderer::textAlign_)は 0 固定で書かれるので、中央/右揃えは数値で直接書く
  * worldMatrix_ はエンジンが読み込み時に計算し直さないので、親から合成して書く
  * 新しく作った Field<T> / Color32 の版キーは、書き出し順で最初の1回だけ付ける
(もとは冒険者の手帳 pause_menu_prefab.py の道具。手帳を消したときにここへ移した)
"""
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))

from tools.common.blob import Ver  # noqa: E402
from tools.common.cereal_json import Num, OrderedObj, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, meta as scene_meta, model, reader, validate, writer  # noqa: E402

# 使う側が書き換える
PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'UI'

FONT_PX = 60  # ipam.ttf / KaiseiDecol-Bold.ttf の TtfFontFile はどちらも 60px。TextRenderer は scale で縮める
ORDER_TEXT = 5030
TEXT_ALIGN = {'left': 0, 'center': 1, 'right': 2}


# ---------------------------------------------------------------- アセット guid
def asset_guid(meta_path):
    text = Path(meta_path).read_text(encoding='utf-8-sig')
    m = re.search(r'"guid_"\s*:\s*\{[^{}]*?"value_"\s*:\s*"([0-9A-Fa-f-]{36})"', text)
    if not m:
        raise SystemExit(f'no guid_ in {meta_path}')
    return m.group(1).upper()


FONT_BODY = asset_guid(REPO / 'Assets/Art/Font/ipam.ttf.meta')
FONT_BRUSH = asset_guid(REPO / 'Assets/Art/Font/KaiseiDecol-Bold.ttf.meta')
FONT_BRUSH_INK = asset_guid(REPO / 'Assets/Art/Font/KaiseiDecol-Bold_Ink.ttf.meta')  # フチなし。紙の上の暗いインク用
FONTS = {'brush': FONT_BRUSH, 'brush_ink': FONT_BRUSH_INK}


# ---------------------------------------------------------------- 部品
class PrefabBuilder:
    def __init__(self, name):
        self.cat = catalog_mod.load()
        root = edits.new_gameobject(name, kind=model.KIND_PREFAB_ROOT)
        self.prefab = model.Prefab(root=root, copied_object_guids=[])

    @property
    def root(self):
        return self.prefab.root

    def node(self, parent, name, pos=(0.0, 0.0), scale=1.0):
        """pos は親からの相対。scale は ImageRenderer / TextRenderer が x を見るので縦横同じにする"""
        return edits.add_gameobject(self.prefab, parent=parent.guid, name=name,
                                    pos=(float(pos[0]), float(pos[1]), 0.0),
                                    scale=(float(scale), float(scale), 1.0))

    def component(self, node, type_name, **params):
        comp = edits.add_component(self.prefab, node.guid, type_name, cat=self.cat,
                                   params={k: str(v) for k, v in params.items()})
        return comp

    def image(self, parent, name, pos, sprite, order, scale=1.0):
        node = self.node(parent, name, pos, scale)
        comp = self.component(node, 'ImageRenderer', spriteFile_=sprite or edits.EMPTY_GUID, renderPriority_=order)
        return node, comp

    def text(self, parent, name, spec, text=''):
        """spec は layout() の文字の指定。pos は TextRenderer の基準点(左上/上辺中央/右上)"""
        node = self.node(parent, name, spec['pos'], spec['px'] / FONT_PX)
        font = FONTS.get(spec.get('font'), FONT_BODY)
        comp = self.component(node, 'TextRenderer', fontFile_=font, renderOrder_=ORDER_TEXT,
                              text_=text or spec.get('text', ''), isWorldPos_='false',
                              textColor_=','.join(str(c) for c in spec['color']))
        comp.data['textAlign_'] = Num.of_int(TEXT_ALIGN[spec['align']])
        return node, comp

    def field(self, comp, key, guid):
        edits._set_field_guid(comp.data[key], guid)


def guid_of(comp):
    return model.find_component_guid(comp)


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
    lp = (edits._f(t.local_pos.x), edits._f(t.local_pos.y), edits._f(t.local_pos.z))
    ls = (edits._f(t.local_scale.x), edits._f(t.local_scale.y), edits._f(t.local_scale.z))
    ws = tuple(parent_scale[i] * ls[i] for i in range(3))
    wp = tuple(parent_pos[i] + parent_scale[i] * lp[i] for i in range(3))
    t.world_matrix = world_matrix_blob(ws, wp)
    for child in t.children:
        bake_world_matrices(child, ws, wp)


def let_writer_place_versions(blob):
    """新しく作った Field<T> / Color32 / 空の基底(IInitRenderable など)は、版キーの有無が固定で作られる。
    ここで作る prefab は丸ごと新規で型の出現順が分かっているので、エンジンと同じく
    「書き出し順で最初の1回だけ付ける」writer の判断に任せる"""
    if isinstance(blob, Ver):
        leaf = blob.key[1] if isinstance(blob.key, tuple) and len(blob.key) > 1 else ''
        is_empty_base = isinstance(blob.body, OrderedObj) and len(blob.body) == 0
        if is_empty_base or (isinstance(leaf, str) and (leaf.startswith(('Field<', 'FieldHolder<')) or leaf == 'Color32')):
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


def all_nodes(node):
    yield node
    for child in node.transform.children:
        yield from all_nodes(child)


def save_prefab(builder, name):
    PREFAB_DIR.mkdir(parents=True, exist_ok=True)
    path = PREFAB_DIR / f'{name}.prefab'
    for node in all_nodes(builder.root):
        for comp in node.components:
            let_writer_place_versions(comp.data)
    bake_world_matrices(builder.root)

    text = writer.write_prefab(builder.prefab)
    problems = validate.validate_prefab(builder.prefab) + validate.validate_class_versions(text, catalog_mod.load())
    hard = [p for p in problems if not p.startswith('note:')]
    for p in problems:
        print('  ' + (p if p.startswith('note:') else 'FAIL: ' + p))
    if hard:
        raise SystemExit(f'{path.name}: validation failed - nothing written')
    path.write_bytes(to_file_bytes(text))

    meta = Path(str(path) + '.meta')
    if meta.exists():
        guid = asset_guid(meta)
    else:
        guid = scene_meta.mint_guid().upper()
        content_path = scene_meta.content_path_for(scene_meta.PREFAB_SPEC, name, PREFAB_DIR, REPO)
        scene_meta.write_meta(scene_meta.PREFAB_SPEC, meta, name, guid, content_path)
    reader.read_prefab_file(path)  # 読み戻せることだけ確かめる
    print(f'wrote {path.relative_to(REPO)}  (asset guid {guid})')
    return guid


# ---------------------------------------------------------------- プレハブ
