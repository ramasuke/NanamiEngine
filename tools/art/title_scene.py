"""タイトル画面 (TitleScene.scene) を組み直す。モンハンライズのタイトルのように、3D の場面の上に題字とメニューを置く。

    python tools/art/title_screen.py           # 先に 2D 素材を書き出す
    python tools/art/title_scene.py             # TitleScene.scene を組み直す (.meta の guid は保つ)

場面は飛行船の船尾楼。主人公 (剣士) が船首の方を向いて手すりの前に立ち、帆と浮島を眺めている。
甲板ではクノイチと飛行船の青年が話し込んでいる (CAST)。
- 空・浮島・飛行船・カメラの Brain は序章 (FirstTouchDownMainIsLandScene) から写す (襲撃の前の同じ空)。
  船は乗り降りの当たり判定・NPC・プレイヤーの出現位置を外し、翼帆の羽ばたき (AirShipWingFlap) だけ残す。
- 序章の浮島は遠いので、ショットの奥に中くらいの距離の島 (TitleSkyIslands, sky_islands.add_islands) を足す。
- カメラは TitleShots の子を上から順に映してループする (GamePlay::Title::TitleCameraDirector)。
  各ショットは子の End へゆっくり動き、NoiseCameraBehaviour で手持ちのように少し揺らす。
- 空のドームに SceneFog を付け、遠景の浮島を空の色に霞ませる (ドーム自体はフォグを切って描かれる)。
- 花びらと光の粒 (tools/art/title_effects.py) を船尾楼のまわりに流す。
- UI は TitleScreenUi / TitleScreenPresenter (右寄せの題字、「ボタンを押してください」、右寄せのメニュー)。
  BGM は Title_Dawn、配信アセットの更新 (AssetUpdateUI) は前のタイトルのものを引き継ぐ。
シーンの版キーは、写した木の初出の位置が変わるので、書き出してから検証しつつ足し引きする (fix_versions)。
"""
import copy
import math
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.cereal_json import Num, OrderedObj, dumps, loads, to_file_bytes  # noqa: E402
from tools.scene import catalog as catalog_mod, edits, model, reader, validate, writer  # noqa: E402

from game_over_prefab import Builder, asset_guid, check, guid_of, let_writer_place_versions  # noqa: E402
from grassland_nature_scatter import bake_world_matrices, walk  # noqa: E402
import numpy as np  # noqa: E402
import sky_islands  # noqa: E402

SCENE = REPO / 'Assets' / 'Scene' / 'TitleScene.scene'
SOURCE = REPO / 'Assets' / 'Scene' / 'FirstTouchDownMainIsLandScene.scene'
UI_DIR = REPO / 'Assets' / 'Art' / 'UI'

SCREEN_W, SCREEN_H = 1920, 1080
FONT_PX = 60  # Kaisei Decol / Zen Old Mincho の TtfFontFile は 60px。TextRenderer は scale で縮める
BLEND_ALPHA = 1
ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT = 0, 1, 2

FONT_HEAD = '956ED4F0-FCBF-4709-B98E-64F17B0DD2AF'  # Kaisei Decol Bold
FONT_BODY = '02951627-F120-4EDC-B4F7-C27985C7F643'  # Zen Old Mincho Bold

PETALS_PREFAB = REPO / 'Assets/Prefab/Particle/TitlePetalsParticle.prefab'
MOTES_PREFAB = REPO / 'Assets/Prefab/Particle/TitleMotesParticle.prefab'

# 船の上の人たち: (名前, モデル, AnimationTree, 足元の位置, 向き (度。0 で -z を向き、正で -x の側へ回る), 大きさ)
# 船尾楼の床は y 5.8、前の手すりは z -521、甲板の床は y -8.9 あたり (AutoMCP で見た)。舵輪は船尾楼の奥 (36.5, -548)
MAIN_DECK_Y = -8.9
CAST = [
    # 主人公。船尾楼の手すりの前で船首 (+z) を向く
    ('TitleSwordMan', 'Assets/Brute.mv1', 'TitleSwordMan', (40.0, 5.8, -523.2), 180.0, 0.08),
    # 甲板の左舷、メインマストの船首側で向かい合って話す二人 (見上げのカメラの前を空ける)
    ('TitleKunoichi', 'Assets/Art/Models/Woman/Kachujin G Rosales.mv1', 'TitleKunoichi', (31.5, MAIN_DECK_Y, -488.0),
     -90.0, 0.078),
    # NOTE: 青年は女性用アニメ (WomanNpc) で腰が 209 -> 110 に下がり、足が 98 x 0.048 = 4.7 床に沈むのでその分上げる
    ('TitleYoungMan', 'Assets/Art/Models/Man/NaughtyYoung.mv1', 'TitleYoungMan', (37.5, MAIN_DECK_Y + 4.7, -487.5), 90.0,
     0.048),
]
AMBIENCE_POS = (40.0, 8.0, -515.0)

# (名前, 秒, 始めの位置, 終わりの位置, 映す点, 画面上の横位置 (-1 左端 .. 1 右端), 縦位置 (-1 下 .. 1 上), FOV)
# ライズのように人物は左の 1/3 あたり、右側は題字とメニューに空ける。映す点は剣士の胸 (y 14.8)
SUBJECT = (40.0, 14.8, -523.2)
SHOTS = [
    # 船尾楼の後ろ、右肩越しに船首と帆を眺める
    ('OverShoulder', 10.0, (52.0, 20.0, -553.0), (49.5, 19.2, -548.0), SUBJECT, -0.36, -0.2, 50.0),
    # 甲板から見上げる。ランタンと扉の上で手すりの前に立つ
    ('LowAngle', 9.0, (49.0, -3.0, -497.0), (46.5, -2.0, -500.5), SUBJECT, -0.34, 0.15, 48.0),
    # 甲板から、話し込む二人を手前に。奥の船尾楼に剣士
    ('Deck', 9.0, (47.0, 0.5, -472.0), (45.0, 1.0, -475.0), (34.5, 1.5, -488.0), -0.15, -0.1, 45.0),
    # 右舷から横顔。奥は左舷の空
    ('Profile', 8.0, (64.0, 15.0, -512.0), (61.5, 15.5, -514.5), (40.0, 15.5, -523.2), -0.30, -0.08, 40.0),
    # 船の外、右舷の前から船尾楼を見上げる引き
    ('Wide', 11.0, (108.0, 22.0, -468.0), (98.0, 20.0, -480.0), (40.0, 8.0, -518.0), -0.25, -0.05, 50.0),
]
# 序章の浮島は 900..2800 と遠く霞むので、ショットの奥に入る向きへ中くらいの距離の島を足す (sky_islands.py の書式、中心は船)
# (方位 deg: +X から +Z へ, 距離, 天辺の高さ, 幅, 縦の伸び, 向き deg, 島のモデル, 飾りの組)
TITLE_ISLANDS_CENTER = (40.0, -500.0)
TITLE_ISLANDS_SEED = 7373
# NOTE: 近くで見上げる島は灰色の底しか見えないので、近い島は天辺を目の高さ (-40..60) に置いて草と木を見せる。
# 高い島は遠くに丸い島か台地だけ (細い Shard は近いと平たい三角に見える)。序章の Island3 (-143 deg, 984) と重ねない
TITLE_ISLANDS = [
    # 船首の先 (OverShoulder の奥、手すりの上に天辺が出る)
    (98.0, 1100.0, -40.0, 700.0, 1.0, 40.0, 'long_grass', 'forest'),
    (76.0, 1350.0, 40.0, 520.0, 1.0, 210.0, 'mesa_grass', 'ruins'),
    (122.0, 950.0, -30.0, 380.0, 1.0, 120.0, 'twin_grass', 'camp'),
    (88.0, 1900.0, 420.0, 650.0, 1.0, 300.0, 'round', 'forest'),
    # 船尾の上。LowAngle の見上げは剣士の後ろに重なるので空けておき、Deck の奥に見える高さにする
    (-140.0, 1600.0, 560.0, 460.0, 1.0, 160.0, 'mesa_grass', 'lookout'),
    # 左舷の船尾側 (Profile / Wide の奥)
    (-172.0, 1000.0, -10.0, 520.0, 1.0, 250.0, 'slab_grass', 'ruins'),
    (-160.0, 1500.0, 80.0, 420.0, 1.0, 20.0, 'twin_grass', 'forest'),
]
CAMERA_NEAR = 1.5
# 遠景の霞 (GamePlay::Weather::SceneFog)。船と人物 (10..60) はくっきり、浮島 (300..3000) ほど空の色に霞む
FOG_COLOR = (196, 214, 232)
FOG_START = 250.0
FOG_END = 3200.0
SHOT_DISABLED_PRIORITY = -1

ORDER_VIGNETTE = 3000
ORDER_DIP = 3010
ORDER_BAND = 3100
ORDER_LOGO = 3110
ORDER_TEXT = 3120
ORDER_HINT = 3130
ORDER_HINT_TEXT = 3131
ORDER_VEIL = 3900

LAYOUT = {
    'logo': (1370, 280), 'logo_scale': 0.9,
    'press': (1370, 612), 'press_px': 34, 'press_color': (246, 236, 214),
    'menu_right': 1830, 'menu_rows': (566, 626, 686), 'menu_px': 40, 'menu_color': (250, 242, 224),
    'band_right': 1866,
    'button_half': (150, 26),
    'hint_y': 1030, 'hint_right': 1880, 'hint_px': 26, 'hint_color': (240, 226, 202),
}
# OpenTracks「オープニングオーケストラ「夜明け」」今川彰人オーケストラ (docs/ThirdPartyAssets.md)
TITLE_BGM = asset_guid(REPO / 'Assets/Audio/BGM/Title_Dawn.mp3.meta')
MENU_LABELS = ('はじめから', '設　定', '終 わ る')
MENU_NAMES = ('Start', 'Settings', 'Exit')
SETTINGS_PREFAB = asset_guid(REPO / 'Assets/Prefab/UI/Settings/SettingsScreen.prefab.meta')


# ---------------------------------------------------------------- 計算
def quat_mul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz)


def look_rotation(pos, target):
    """エディタのカメラと同じ向き: yaw (y 軸) のあとに pitch (x 軸、正で下向き)"""
    d = [t - p for p, t in zip(pos, target)]
    n = math.sqrt(sum(c * c for c in d))
    fx, fy, fz = (c / n for c in d)
    yaw = math.atan2(fx, fz)
    pitch = -math.asin(max(-1.0, min(1.0, fy)))
    qy = (0.0, math.sin(yaw / 2), 0.0, math.cos(yaw / 2))
    qx = (math.sin(pitch / 2), 0.0, 0.0, math.cos(pitch / 2))
    return quat_mul(qy, qx)


# ---------------------------------------------------------------- 写す木
def copy_roots(source, names):
    """序章から names の木を写す。木どうしの参照 (SkyDome3D.mainCamera_ -> Brain) も写した先へ付け替える"""
    roots = [copy.deepcopy(next(r for r in source.roots if r.name == name)) for name in names]
    remap = {}
    for root in roots:
        edits._remint_guids(root, remap)
    for root in roots:
        edits._remap_guid_references(root, remap)
    return {r.name: r for r in roots}


def drop_components(node, *suffixes):
    node.components = [c for c in node.components if not c.fqn.endswith(suffixes)]


def trim_airship(ship):
    """見た目だけ残す: 船体・舷梯・翼帆 (羽ばたき)。乗り降りの当たり判定・NPC・出現位置・撃墜の粒子は外す"""
    drop_components(ship, '::BoxCollider', '::RigidBody', '::AirShip')
    keep = ('AirShipGangway', 'AirShipWing')
    ship.transform.children = [c for c in ship.transform.children if c.name.startswith(keep)]
    for node in walk(ship):
        drop_components(node, 'Collider', '::RigidBody')


def trim_camera_brain(brain):
    # 音はタイトルの UI 側 (BGM) が持つ
    drop_components(brain, '::AudioSource', '::SoundPlayer')
    for comp in brain.components:
        if comp.fqn.endswith('::CinemachineCameraBrain'):
            comp.data['cameraNear_'] = Num.of_float(CAMERA_NEAR)


# ---------------------------------------------------------------- 新しく組む木
def build_cast(b):
    roots = []
    for name, model_path, tree, pos, yaw_deg, scale in CAST:
        node = b.node(None, name)
        t = node.transform
        t.local_pos = edits._vec3_from_floats(pos)
        half = math.radians(yaw_deg) / 2
        t.local_rot = edits._quat_from_floats((0.0, math.sin(half), 0.0, math.cos(half)))
        t.local_scale = edits._vec3_from_floats((scale,) * 3)
        b.component(node, 'ModelRenderer', mv1File_=asset_guid(REPO / f'{model_path}.meta'), useFixedInterpolation_='true')
        b.component(node, 'Animator', animationTreeFile_=asset_guid(REPO / f'Assets/Animations/{tree}.animTree.meta'),
                    timeScale_='1')
        roots.append(node)
    return roots


def build_title_islands(scene, b):
    root = edits.add_gameobject(scene, parent=None, name='TitleSkyIslands')
    counts = sky_islands.add_islands(scene, b, root, TITLE_ISLANDS, TITLE_ISLANDS_CENTER,
                                     np.random.default_rng(TITLE_ISLANDS_SEED), sky_islands.DEBRIS['main'],
                                     sky_islands.DEBRIS_COUNT['main'])
    print(f'  TitleSkyIslands {counts}')
    return root


def build_ambience(scene):
    root = edits.add_gameobject(scene, parent=None, name='TitleAmbience', pos=AMBIENCE_POS)
    for path in (PETALS_PREFAB, MOTES_PREFAB):
        edits.instantiate_prefab(scene, reader.read_prefab_file(path), parent=root.guid)
    return root


def framed_rotation(pos, subject, screen_x, screen_y, fov_deg):
    """subject が画面の (screen_x, screen_y) に来る向き。yaw は +x 側が画面の右 (DxLib は左手系)"""
    d = [t - p for p, t in zip(pos, subject)]
    horizontal = math.hypot(d[0], d[2])
    yaw_s = math.atan2(d[0], d[2])
    pitch_s = -math.atan2(d[1], horizontal)
    half_v = math.radians(fov_deg) / 2
    half_h = math.atan(math.tan(half_v) * (16 / 9))
    yaw = yaw_s - math.atan(screen_x * math.tan(half_h))
    pitch = pitch_s + math.atan(screen_y * math.tan(half_v))
    qy = (0.0, math.sin(yaw / 2), 0.0, math.cos(yaw / 2))
    qx = (math.sin(pitch / 2), 0.0, 0.0, math.cos(pitch / 2))
    return quat_mul(qy, qx)


def build_shots(b, dip_mask):
    root = b.node(None, 'TitleShots')
    for i, (name, _, pos, end_pos, subject, sx, sy, fov) in enumerate(SHOTS):
        rot = framed_rotation(pos, subject, sx, sy, fov)
        end_rot = framed_rotation(end_pos, subject, sx, sy, fov)
        shot = edits.add_gameobject(b.target, parent=root.guid, name=name, pos=pos)
        shot.transform.local_rot = edits._quat_from_floats(rot)
        camera = b.component(shot, 'CineMachineVirtualCamera', overrideFov_='true', fov_=f'{fov}')
        camera.data['priority_'] = OrderedObj([('value', Num.of_int(SHOT_DISABLED_PRIORITY))])
        b.component(shot, 'NoiseCameraBehaviour', rotationAmplitude_deg_='0.35,0.5,0.12',
                    positionAmplitude_='0.04,0.05,0.04', frequency_='0.22', octaves_='2', amplitudeGain_='1',
                    frequencyGain_='1', blendIn_secs_='0.1', seed_=f'{11.3 + i * 7.1},{47.9 + i * 3.3},{83.1 - i * 5.7}')
        # NOTE: End はショットの子なので、ワールドの終わりの姿勢をショットのローカルへ直して置く
        end = edits.add_gameobject(b.target, parent=shot.guid, name='End')
        end.transform.local_pos = edits._vec3_from_floats(to_local(pos, rot, end_pos))
        end.transform.local_rot = edits._quat_from_floats(quat_mul(quat_conj(rot), end_rot))

    director = b.component(root, 'TitleCameraDirector', defaultShotDuration_secs_='9', dipDuration_secs_='0.9',
                           shotPriority_='100')
    director.data['shotDurations_secs_'] = [Num.of_float(s[1]) for s in SHOTS]
    b.field(director, 'shotsRoot_', root.guid)
    b.field(director, 'dipMask_', guid_of(dip_mask))
    return root


def quat_conj(q):
    return (-q[0], -q[1], -q[2], q[3])


def quat_rotate(q, v):
    qv = (v[0], v[1], v[2], 0.0)
    r = quat_mul(quat_mul(q, qv), quat_conj(q))
    return r[:3]


def to_local(parent_pos, parent_rot, world_pos):
    d = tuple(w - p for w, p in zip(world_pos, parent_pos))
    return quat_rotate(quat_conj(parent_rot), d)


def build_ui(b, old_canvas):
    L = LAYOUT
    root = b.node(None, 'TitleUI')
    black = asset_guid(UI_DIR / 'BlackMask.png.meta')
    veil_scale = max(SCREEN_W / 4096, SCREEN_H / 2894) * 1.02

    def sprite(name):
        return asset_guid(UI_DIR / 'Title' / f'{name}.png.meta')

    center = (SCREEN_W / 2, SCREEN_H / 2)
    _, vignette = b.image(root, 'Vignette', center, sprite('Title_Vignette'), ORDER_VIGNETTE, 255)
    _, dip = b.image(root, 'DipMask', center, black, ORDER_DIP, 0, scale=veil_scale)
    _, logo = b.image(root, 'Logo', L['logo'], sprite('Title_Logo'), ORDER_LOGO, 0, scale=L['logo_scale'])
    _, deco = b.image(root, 'PressDeco', L['press'], sprite('Title_PressDeco'), ORDER_LOGO, 0)
    px = L['press_px']
    _, press = b.text(root, 'PressText', (L['press'][0], L['press'][1] - px * 0.62), px, 'ボタンを押してください',
                      L['press_color'], ORDER_TEXT, ALIGN_CENTER)
    b.field(press, 'fontFile_', FONT_HEAD)

    band_x = L['band_right'] - 280
    _, band = b.image(root, 'SelectBand', (band_x, L['menu_rows'][0]), sprite('Title_SelectBand'), ORDER_BAND, 0)

    texts, buttons = [], []
    mpx = L['menu_px']
    for name, label, y in zip(MENU_NAMES, MENU_LABELS, L['menu_rows']):
        _, text = b.text(root, f'{name}Text', (L['menu_right'], y - mpx * 0.62), mpx, label, L['menu_color'],
                         ORDER_TEXT, ALIGN_RIGHT)
        b.field(text, 'fontFile_', FONT_HEAD)
        texts.append(text)
        hw, hh = L['button_half']
        button_node = b.node(root, f'{name}Button', (L['menu_right'] - hw + 20, y))
        buttons.append(b.component(button_node, 'Button', eventAreaSize_=f'{hw},{hh}'))

    hints = b.node(root, 'Hints')
    hint_parts = {}
    x = L['hint_right']
    hy = L['hint_y']
    hint_font = b.text  # 文字は Zen Old
    for key, tag, tag_w, label in reversed([
            ('move', UI_DIR / 'StageReturn/StageReturn_Hint_UpDown.png.meta', 68, '選ぶ'),
            ('confirm', UI_DIR / 'CharacterSelect/HintTag_Confirm.png.meta', 46, '決める')]):
        hpx = L['hint_px']
        _, text = hint_font(hints, f'{key.capitalize()}Text', (x, hy - hpx * 0.62), hpx, label, L['hint_color'],
                            ORDER_HINT_TEXT, ALIGN_RIGHT)
        b.field(text, 'fontFile_', FONT_BODY)
        x -= hpx * len(label) + 16
        _, image = b.image(hints, f'{key.capitalize()}Tag', (x - tag_w / 2, hy), asset_guid(tag), ORDER_HINT, 0)
        x -= tag_w + 30
        hint_parts[key] = (image, text)

    _, veil = b.image(root, 'Veil', center, black, ORDER_VEIL, 255, scale=veil_scale)

    ui = b.component(root, 'TitleScreenUi', veilOpen_secs_='2.2', logoDelay_secs_='1.4', logoFade_secs_='1.8',
                     pressDelay_secs_='3.2', pressFade_secs_='0.8', pressPulsePeriod_secs_='2.6',
                     pressPulseMinRate_='0.35', menuFade_secs_='0.3', menuStagger_secs_='0.07', menuSlide_px_='24',
                     menuInputGuard_secs_='0.2', bandFollowRate_='18', unselectedTextRate_='0.62',
                     coverFade_secs_='0.2')
    for key, comp in (('veil_', veil), ('logo_', logo), ('pressText_', press), ('pressDeco_', deco),
                      ('startText_', texts[0]), ('settingsText_', texts[1]), ('exitText_', texts[2]),
                      ('startButton_', buttons[0]), ('settingsButton_', buttons[1]), ('exitButton_', buttons[2]),
                      ('selectBand_', band),
                      ('moveHintTag_', hint_parts['move'][0]), ('moveHintText_', hint_parts['move'][1]),
                      ('confirmHintTag_', hint_parts['confirm'][0]), ('confirmHintText_', hint_parts['confirm'][1])):
        b.field(ui, key, guid_of(comp))

    old_title = next(c for c in old_canvas.components if c.fqn.endswith(('::SampleTitleScene', '::TitleScreenPresenter')))
    presenter = b.component(root, 'TitleScreenPresenter')
    presenter.data['assetUpdatePrefab_'] = copy.deepcopy(old_title.data['assetUpdatePrefab_'])
    presenter.data['uiSounds_'] = copy.deepcopy(old_title.data['uiSounds_'])
    b.field(presenter, 'settingsPrefab_', SETTINGS_PREFAB)

    # BGM は前のタイトルの Canvas から引き継ぐ
    for comp in old_canvas.components:
        if comp.fqn.endswith(('::AudioSource', '::BgmPlayObject', '::SoundPlayer')):
            moved = copy.deepcopy(comp)
            model.set_component_guid(moved, edits.mint_guid())
            if comp.fqn.endswith('::BgmPlayObject'):
                edits._set_field_guid(moved.data['bgm_'], TITLE_BGM)
            root.components.append(moved)
    return root, dip


# ---------------------------------------------------------------- 版キー
class VersionFixer(validate._ClassVersionAudit):
    """初出に版キーが無ければ known の値を足し、2回目以降にあれば消す"""

    def __init__(self, cat, known):
        super().__init__(cat)
        self.known = known
        self.added = 0
        self.removed = 0
        self.unknown = set()

    def _check(self, type_key, node, where):
        if type_key not in self._first:
            if validate._VER not in node:
                if type_key in self.known:
                    node.insert(0, validate._VER, Num.of_int(self.known[type_key]))
                    self.added += 1
                elif len(node) == 0:
                    # 中身の無い基底 (IUpdatable 等) は版 0。古い版のコンポーネント (Brain v3) は別の基底として読むので、
                    # カタログの名前に頼らず版キーを付けておく (空の基底の余分な版キーは害が無い)
                    node.insert(0, validate._VER, Num.of_int(0))
                    self.added += 1
                else:
                    self.unknown.add(type_key)
        elif validate._VER in node:
            node.pop(validate._VER)
            self.removed += 1
        super()._check(type_key, node, where)

    def _check_component(self, fqn, data, where):
        is_first = fqn not in self._first
        super()._check_component(fqn, data, where)
        # NOTE: 版がカタログと違うコンポーネント (Brain v3, ModelRenderer v4) は基底を監査しないので、
        #       写した木で初出がずれると基底の版キーが抜ける。初出の基底スロットには版 0 を付けておく
        #       (基底は ComponentBase と空のインターフェースだけで、2回目以降の余分な版キーは害が無い)
        if not is_first or fqn not in self.version_skew:
            return
        for key in list(data.keys()):
            slot = data[key]
            if not key.startswith('value') or not isinstance(slot, OrderedObj) or validate._VER in slot:
                continue
            if len(slot) == 0 or 'guid_' in slot:
                slot.insert(0, validate._VER, Num.of_int(0))
                self.added += 1


def known_versions(texts):
    class Probe(validate._ClassVersionAudit):
        def __init__(self):
            super().__init__(catalog_mod.load())
            self.versions = {}

        def _check(self, type_key, node, where):
            version = node.get(validate._VER)
            if version is not None and type_key not in self.versions:
                self.versions[type_key] = int(version.value)
            super()._check(type_key, node, where)

    probe = Probe()
    for text in texts:
        probe._first.clear()
        probe._polymap.clear()
        probe.run(loads(text), '')
    return probe.versions


# ---------------------------------------------------------------- 組み立て
def main():
    source = reader.read_scene_file(SOURCE)
    old = reader.read_scene_file(SCENE)
    old_canvas = next(r for r in old.roots if r.name in ('Canvas', 'TitleUI'))
    # NOTE: 2回目以降は、前に組んだ TitleUI から BGM と更新の設定を拾う

    copied = copy_roots(source, ['CameraBrain', 'SkyDome', 'SkyIslands', 'AirShip'])
    trim_camera_brain(copied['CameraBrain'])
    drop_components(copied['SkyDome'], '::WeatherService')
    # NOTE: 序章の空にも SceneFog が付いているので、タイトルの霞の値で付け直す
    drop_components(copied['SkyDome'], '::SceneFog')
    trim_airship(copied['AirShip'])

    scene = model.Scene(name='TitleScene', roots=list(copied.values()))
    b = Builder(scene)
    b.component(copied['SkyDome'], 'SceneFog', fogColor_=','.join(map(str, FOG_COLOR)), fogStart_=str(FOG_START),
                fogEnd_=str(FOG_END))
    new_roots = build_cast(b)
    new_roots.append(build_title_islands(scene, b))
    new_roots.append(build_ambience(scene))
    ui_root, dip = build_ui(b, old_canvas)
    new_roots.append(build_shots(b, dip))
    new_roots.append(ui_root)
    # UI は shots の後ろ (描画順は renderOrder_ で決まるが、読み込み順を揃えておく)
    scene.roots = list(copied.values()) + new_roots

    for root in new_roots:
        for node in walk(root):
            for comp in node.components:
                let_writer_place_versions(comp.data)
        bake_world_matrices(root)

    text = writer.write_scene(scene)
    known = known_versions([SOURCE.read_bytes().decode('utf-8-sig'), SCENE.read_bytes().decode('utf-8-sig'),
                            PETALS_PREFAB.read_bytes().decode('utf-8-sig'), text])
    tree = loads(text)
    fixer = VersionFixer(catalog_mod.load(), known)
    fixer.run(tree, '')
    if fixer.unknown:
        raise SystemExit(f'no known cereal_class_version for {sorted(fixer.unknown)} - nothing written')
    text = dumps(tree)
    print(f'  versions: +{fixer.added} -{fixer.removed}')

    check(text, validate.validate_scene(scene), SCENE.name)
    SCENE.write_bytes(to_file_bytes(text))
    reader.read_scene_file(SCENE)
    print(f'wrote {SCENE.relative_to(REPO)}')


if __name__ == '__main__':
    main()
