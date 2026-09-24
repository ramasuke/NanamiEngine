"""Build the 紙の航路図 loading screen (LoadingScreenUI.prefab + its copy in StageLoadingScene.scene)
and the .loadingRoute assets from tools/art/loading_map.py's layout() / routes().

    python tools/art/loading_map.py --emit        # 先にスプライトを書き出す
    python tools/art/loading_map_prefab.py        # 航路データ・prefab・StageLoadingScene を組み直す
    python tools/art/loading_map_prefab.py --routes-only   # 航路データを書き、prefab / scene の routes_ に無いものだけ足す

prefab / scene / .loadingRoute の asset guid は既存の .meta を保つので、Game の stageLoadingSceneFile_ などの
外からの参照は切れない(中の GameObject / Component の guid は毎回新しくなる)。

tools.scene の CLI では届かない所をここで埋める(pause_menu_prefab.py と同じ):
  * enum(BlendImageRenderer::blendMode_ / TextRenderer::textAlign_)は 0 固定で書かれるので数値で直接書く
  * add_component は数値を 0 で書くので、ヘッダの初期値を明示する
  * vector<FIELD(T)>(LoadingRouteMap::routeDashes_ など)はカタログが単一の field と見なすので、配列を自前で作る
  * .loadingRoute は ScriptableObject なので、中身ごと .meta に入る(StageData の .meta と同じ形)
"""
import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, meta as scene_meta, model, reader, validate, writer  # noqa: E402

import loading_map as art  # noqa: E402
import pause_menu_prefab as pm  # noqa: E402

PREFAB_DIR = REPO / 'Assets' / 'Prefab' / 'UI'
SCENE_PATH = REPO / 'Assets' / 'Scene' / 'StageLoadingScene.scene'
ROUTE_DIR = REPO / 'Assets' / 'Data' / 'LoadingRoute'

FONT_PX = 60  # KaiseiDecol-Bold.ttf / ZenOldMincho-Bold.ttf の TtfFontFile はどちらも 60px。TextRenderer は scale で縮める
FONT_BODY = pm.asset_guid(REPO / 'Assets/Art/Font/ZenOldMincho-Bold.ttf.meta')
FONT_BRUSH = pm.asset_guid(REPO / 'Assets/Art/Font/KaiseiDecol-Bold.ttf.meta')
BLACK_MASK = pm.asset_guid(REPO / 'Assets/Art/UI/BlackMask.png.meta')
CLEARED_SEAL = pm.asset_guid(REPO / 'Assets/Art/UI/EventBoard/QuestSeal_Cleared.png.meta')
KEY_GLYPH = pm.asset_guid(REPO / 'Assets/Art/UI/ControlGuide/ControlGuide_Key_Shift.png.meta')
PAD_GLYPH = pm.asset_guid(REPO / 'Assets/Art/UI/ControlGuide/ControlGuide_Pad_B.png.meta')

BLEND_ALPHA = 1
TEXT_ALIGN = {'left': 0, 'center': 1, 'right': 2}
SCENE_TYPE = {'GrassLand': 0, 'FirstTouchDownMainIsLand': 1, 'MainIsland': 2, 'Title': 3, 'Desert': 4}

ORDER_DESK = 9000
ORDER_PAPER_SHADOW = 9005
ORDER_PAPER = 9010
ORDER_CLOUD_SHADOW = 9020
ORDER_DASH = 9030
ORDER_DEST_CIRCLE = 9035
ORDER_STAMP = 9040
ORDER_MAP_TEXT = 9045
ORDER_SHIP_SHADOW = 9050
ORDER_TRAIL = 9055
ORDER_SHIP = 9060
ORDER_VIGNETTE = 9070
ORDER_FRONT_CLOUD = 9080
ORDER_CHIP = 9090
ORDER_CHIP_TEXT = 9095
ORDER_COVER = 9900


def sprite(name):
    return pm.asset_guid(art.EMIT_DIR / f'{name}.png.meta')


# ---------------------------------------------------------------- .loadingRoute
def vec2(p):
    return {'value0': float(p[0]), 'value1': float(p[1])}


def write_route(route):
    """StageData の .meta と同じ形の、中身入りの .meta を書く。本体の .loadingRoute は空"""
    ROUTE_DIR.mkdir(parents=True, exist_ok=True)
    name = route['name']
    body = ROUTE_DIR / f'{name}.loadingRoute'
    meta = Path(str(body) + '.meta')
    guid = None
    if meta.exists():
        guid = json.loads(meta.read_text(encoding='utf-8-sig'))['value0']['ptr_wrapper']['data']['value0']['guid_']['value_']
    guid = (guid or scene_meta.mint_guid()).upper()

    hover = route.get('hover')
    pts = route.get('pts') or [hover[0]] * 4
    from_caption = route.get('from_caption') or ('', (0, 0))
    to_caption = route.get('to_caption') or ('', (0, 0))
    circle = route.get('circle')
    data = {
        'cereal_class_version': 0,
        'value0': {
            'cereal_class_version': 0,
            'value0': {'cereal_class_version': 0},
            'contentPath_': f'Assets\\Data\\LoadingRoute/{name}.loadingRoute',
            'guid_': {'cereal_class_version': 0, 'value_': guid},
        },
        'isFromAnywhere_': route['frm'] is None,
        'fromScene_': SCENE_TYPE[route['frm'] or 'MainIsland'],
        'toScene_': SCENE_TYPE[route['to']],
        'isHover_': hover is not None,
        'hoverCenter_': vec2(hover[0] if hover else (495, 600)),
        'hoverRadius_': vec2(hover[1] if hover else (120, 60)),
        'hoverLapSecs_': 7.0,
        'p0_': vec2(pts[0]),
        'p1_': vec2(pts[1]),
        'p2_': vec2(pts[2]),
        'p3_': vec2(pts[3]),
        'kickerText_': route['kicker'],
        'titleText_': route['title'],
        'statusText_': route['status'],
        'fromCaption_': from_caption[0],
        'fromCaptionPosition_': vec2(from_caption[1]),
        'toCaption_': to_caption[0],
        'toCaptionPosition_': vec2(to_caption[1]),
        'hasDestCircle_': circle is not None,
        'destCirclePosition_': vec2(circle[0] if circle else (0, 0)),
        'destCircleScale_': float(circle[1] if circle else 1.0),
        'stampPosition_': vec2(art.layout()['stamp']['center']),
        'cloudDirection_': float(route['cloud']),
        'hasNetworkStep_': bool(route['network']),
    }
    doc = {'value0': {
        'polymorphic_id': 2147483649,
        'polymorphic_name': 'NanamiEngine::Module::Asset::LoadingRouteData',
        'ptr_wrapper': {'id': 2147483649, 'data': data},
    }}
    text = json.dumps(doc, ensure_ascii=False, indent=4).replace('\n', '\r\n')
    meta.write_bytes(text.encode('utf-8'))
    if not body.exists():
        body.write_bytes(b'')
    print(f'  {name}.loadingRoute  guid={guid}')
    return guid


# ---------------------------------------------------------------- prefab
class Builder(pm.PrefabBuilder):
    def image(self, parent, name, pos, sprite_guid, order, scale=1.0):
        node = self.node(parent, name, pos, scale)
        comp = self.component(node, 'ImageRenderer', spriteFile_=sprite_guid, renderPriority_=order)
        return node, comp

    def blend(self, parent, name, pos, sprite_guid, order, rate=255, scale=1.0):
        node = self.node(parent, name, pos, scale)
        comp = self.component(node, 'BlendImageRenderer', spriteFile_=sprite_guid, renderOrder_=order, blendRate_=rate)
        comp.data['blendMode_'] = Num.of_int(BLEND_ALPHA)
        return node, comp

    def gauge(self, parent, name, pos, sprite_guid, order, start, span, fill):
        node = self.node(parent, name, pos)
        comp = self.component(node, 'CircleGaugeRenderer', spriteFile_=sprite_guid, blendRate_='255',
                              renderOrder_=order, startPercent_=start, spanPercent_=span, fillRate_=fill)
        comp.data['blendMode_'] = Num.of_int(BLEND_ALPHA)
        return node, comp

    def label(self, parent, name, spec, order, text=None):
        """spec は layout() の文字の指定。pos は TextRenderer の基準点(左揃えは左上、中央揃えは上辺中央)"""
        node = self.node(parent, name, spec['pos'], spec['px'] / FONT_PX)
        font = FONT_BRUSH if spec.get('font') == 'brush' else FONT_BODY
        comp = self.component(node, 'TextRenderer', fontFile_=font, renderOrder_=order,
                              text_=spec.get('text', '') if text is None else text, isWorldPos_='false',
                              textColor_=','.join(str(c) for c in spec['color']))
        comp.data['textAlign_'] = Num.of_int(TEXT_ALIGN[spec['align']])
        return node, comp


def field_list(type_leaf, guids):
    return [edits.field_blob(type_leaf, g) for g in guids]


def build_prefab(route_guids):
    geo = art.layout()
    b = Builder('LoadingScreenUI')
    root = b.root

    _, cover = b.blend(root, 'Cover', (960, 540), BLACK_MASK, ORDER_COVER, rate=0, scale=0.47)

    # 地図一式は Visual の下。LoadingScreenUi が黒幕で覆い切ってから有効にする
    visual = b.node(root, 'Visual')
    visual.is_active = False
    camera = b.node(visual, 'MapCamera')

    b.image(camera, 'Desk', geo['desk']['center'], sprite('LoadingMap_Desk'), ORDER_DESK)
    b.image(camera, 'PaperShadow', geo['paper_shadow']['center'], sprite('LoadingMap_PaperShadow'), ORDER_PAPER_SHADOW)
    b.image(camera, 'Paper', geo['paper']['center'], sprite('LoadingMap_Paper'), ORDER_PAPER)

    cloud_shadows = []
    for i, pos in enumerate(geo['cloud_shadows']):
        _, comp = b.blend(camera, f'CloudShadow{i}', pos, sprite('LoadingMap_CloudShadow'), ORDER_CLOUD_SHADOW)
        cloud_shadows.append(pm.guid_of(comp))

    dashes_root = b.node(camera, 'RouteDashes')
    dashes = []
    for i in range(geo['dash_count']):
        _, comp = b.image(dashes_root, f'Dash{i:02d}', art.DEPART_ROUTE[0], sprite('LoadingMap_Dash'), ORDER_DASH)
        dashes.append(pm.guid_of(comp))

    # 〇は LoadingRouteMap が fillRate を 0 -> 1 に動かして、1時あたりから一筆で描く
    _, dest_circle = b.gauge(camera, 'DestCircle', geo['dest_circle']['center'], sprite('LoadingMap_DestCircle'),
                             ORDER_DEST_CIRCLE, start='8', span='100', fill='0')
    _, stamp = b.image(camera, 'ClearedStamp', geo['stamp']['center'], CLEARED_SEAL, ORDER_STAMP)
    _, kicker = b.label(camera, 'Kicker', geo['kicker'], ORDER_MAP_TEXT)
    _, title = b.label(camera, 'Title', geo['title'], ORDER_MAP_TEXT)
    _, from_caption = b.label(camera, 'FromCaption', geo['from_caption'], ORDER_MAP_TEXT)
    _, to_caption = b.label(camera, 'ToCaption', geo['to_caption'], ORDER_MAP_TEXT)

    _, ship_shadow = b.blend(camera, 'ShipShadow', geo['ship']['center'], sprite('LoadingMap_ShipShadow'), ORDER_SHIP_SHADOW)
    trail = []
    for i in range(geo['trail_count']):
        _, comp = b.blend(camera, f'Trail{i}', geo['ship']['center'], sprite('LoadingMap_TrailPuff'), ORDER_TRAIL)
        trail.append(pm.guid_of(comp))
    _, ship = b.image(camera, 'Ship', geo['ship']['center'], sprite('LoadingMap_ShipRight'), ORDER_SHIP)

    # ここから下はカメラの外。レンズの周辺減光と、地図より手前を流れる雲
    b.image(visual, 'Vignette', geo['vignette']['center'], sprite('LoadingMap_Vignette'), ORDER_VIGNETTE)
    front_clouds = []
    for i, (x, y, variant) in enumerate(geo['front_clouds']):
        _, comp = b.blend(visual, f'FrontCloud{i}', (x, y), sprite(f'LoadingMap_Cloud{variant}'), ORDER_FRONT_CLOUD)
        front_clouds.append(pm.guid_of(comp))

    b.image(visual, 'Chip', geo['chip']['center'], sprite('LoadingMap_Chip'), ORDER_CHIP)
    _, status = b.label(visual, 'StatusText', geo['status'], ORDER_CHIP_TEXT)
    _, percent = b.label(visual, 'PercentText', geo['percent'], ORDER_CHIP_TEXT)

    hint = b.node(visual, 'LoadingHintCard')
    _, glyph = b.image(hint, 'Glyph', geo['hint_glyph']['center'], KEY_GLYPH, ORDER_CHIP_TEXT, scale=1.1)
    _, hint_text = b.label(hint, 'Text', geo['hint_text'], ORDER_CHIP_TEXT, text='')
    hint_card = b.component(hint, 'LoadingHintCard', keyboardGlyphSprite_=KEY_GLYPH, padGlyphSprite_=PAD_GLYPH,
                            hintText_='回避の直後は無敵時間が発生する')
    b.field(hint_card, 'text_', pm.guid_of(hint_text))
    b.field(hint_card, 'glyph_', pm.guid_of(glyph))

    route_map = b.component(
        root, 'LoadingRouteMap',
        shipRightSprite_=sprite('LoadingMap_ShipRight'), shipLeftSprite_=sprite('LoadingMap_ShipLeft'),
        dashSprite_=sprite('LoadingMap_Dash'), dashPassedSprite_=sprite('LoadingMap_DashPassed'),
        cameraZoom_='1.16', cameraZoomWobble_='0.012', cameraFollow_='0.6', cameraLag_='0.06',
        cameraMaxPan_='250,140', shipLiftPx_='33', shipBobPx_='3', shipTiltLimitDeg_='14',
        shadowOffset_='12,9', cloudShadowSpeed_='36', frontCloudSpeed_='150', cloudWrapRangeX_='-600,2520',
        destCircleDelay_secs_='0.35', destCircleDraw_secs_='0.5')
    b.field(route_map, 'camera_', camera.guid)
    b.field(route_map, 'ship_', pm.guid_of(ship))
    b.field(route_map, 'shipShadow_', pm.guid_of(ship_shadow))
    route_map.data['trail_'] = field_list('BlendImageRenderer', trail)
    route_map.data['routeDashes_'] = field_list('ImageRenderer', dashes)
    b.field(route_map, 'destCircle_', pm.guid_of(dest_circle))
    b.field(route_map, 'clearedStamp_', pm.guid_of(stamp))
    route_map.data['cloudShadows_'] = field_list('BlendImageRenderer', cloud_shadows)
    route_map.data['frontClouds_'] = field_list('BlendImageRenderer', front_clouds)
    b.field(route_map, 'kickerText_', pm.guid_of(kicker))
    b.field(route_map, 'titleText_', pm.guid_of(title))
    b.field(route_map, 'fromCaptionText_', pm.guid_of(from_caption))
    b.field(route_map, 'toCaptionText_', pm.guid_of(to_caption))

    ui = b.component(root, 'LoadingScreenUi', fadeInSecs_='0.25', fadeOutSecs_='0.3', minShowSecs_='1.6',
                     progressFollowRate_='6', finishSecs_='0.35')
    b.field(ui, 'visualRoot_', visual.guid)
    b.field(ui, 'cover_', pm.guid_of(cover))
    b.field(ui, 'routeMap_', pm.guid_of(route_map))
    ui.data['routes_'] = field_list('LoadingRouteData', route_guids)
    b.field(ui, 'statusText_', pm.guid_of(status))
    b.field(ui, 'percentText_', pm.guid_of(percent))
    b.field(ui, 'hintCard_', pm.guid_of(hint_card))

    pm.PREFAB_DIR = PREFAB_DIR
    return pm.save_prefab(b, 'LoadingScreenUI')


# ---------------------------------------------------------------- StageLoadingScene
def rebuild_scene():
    """StageLoadingScene の中身を、組み直した prefab の写しに差し替える。シーンの .meta は触らない"""
    scene = reader.read_scene_file(SCENE_PATH)
    scene.roots = []
    prefab = reader.read_prefab_file(PREFAB_DIR / 'LoadingScreenUI.prefab')
    edits.instantiate_prefab(scene, prefab)

    text = writer.write_scene(scene)
    problems = validate.validate_scene(scene) + validate.validate_class_versions(text, catalog_mod.load())
    hard = [p for p in problems if not p.startswith('note:')]
    for p in problems:
        print('  ' + (p if p.startswith('note:') else 'FAIL: ' + p))
    if hard:
        raise SystemExit(f'{SCENE_PATH.name}: validation failed - nothing written')
    SCENE_PATH.write_bytes(to_file_bytes(text))
    reader.read_scene_file(SCENE_PATH)
    print(f'wrote {SCENE_PATH.relative_to(REPO)}')


def append_routes(path, route_guids):
    """組み直さずに、routes_ の配列の末尾へ無い航路だけを足す (手で直した prefab / scene の中身を保つ)"""
    import re
    text = path.read_bytes().decode('utf-8')
    start = text.index('"routes_": [')
    depth, i = 0, text.index('[', start)
    while True:
        if text[i] == '[':
            depth += 1
        elif text[i] == ']':
            depth -= 1
            if depth == 0:
                break
        i += 1
    body = text[start:i]
    missing = [g for g in route_guids if g not in body]
    if not missing:
        return 0
    nl = '\r\n' if '\r\n' in body else '\n'
    indent = re.search(r'\[\r?\n( *)\{', body).group(1)
    next_id = max(int(x) for x in re.findall(r'"id": (\d+)', text)) + 1
    items = []
    for g in missing:
        lines = ('{', '    "value0": {', '        "polymorphic_id": 1073741824,', '        "ptr_wrapper": {',
                 f'            "id": {next_id},', '            "data": {', '                "value0": {',
                 f'                    "value_": "{g}"', '                }', '            }', '        }', '    }', '}')
        items.append(nl.join(indent + line for line in lines))
        next_id += 1
    # 最後の要素の閉じ括弧の直後に足す
    insert_at = start + len(body.rstrip())
    text = text[:insert_at] + ',' + nl + (',' + nl).join(items) + text[insert_at:]
    path.write_bytes(text.encode('utf-8'))
    return len(missing)


def main():
    print('routes:')
    route_guids = [write_route(r) for r in art.routes()]
    if '--routes-only' in sys.argv:
        for path in (PREFAB_DIR / 'LoadingScreenUI.prefab', SCENE_PATH):
            print(f'  {path.name}: +{append_routes(path, route_guids)} route(s)')
        return
    build_prefab(route_guids)
    rebuild_scene()


if __name__ == '__main__':
    main()
