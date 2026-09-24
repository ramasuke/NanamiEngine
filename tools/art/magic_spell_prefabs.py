"""MagicCaster の魔法エフェクト (tools/art/magic_spell_effects.py) をプレハブに組み込む。

    python tools/art/magic_spell_effects.py --build-dir <tmp> --install   # 先にエフェクト
    python tools/art/magic_spell_prefabs.py

書き出すもの (既存の .meta は guid を保つので、再実行しても参照は切れない):
* Assets/Prefab/Magic/Cast/Cast_<Motion>.prefab - MagicSpellData.castEffectPrefab_ が出すアニメごとの詠唱エフェクト
* Assets/Prefab/Magic/Effect/*.prefab           - 着弾・爆発・バフ・チャネルのヒット (1回再生して破棄)
* tools/art/magic_sfx.py の音: 詠唱プレハブに <Spell>_Charge、各プレハブに着弾/爆発/魔法陣/隆起/ヒット音
  (GamePlay::Sound::SpawnSound)、魔法の castSound_ に <Spell>_Release (Assets/Data/Magic/*.magicSpell.meta)
* 魔法プレハブ: 弾には新しい飛翔エフェクトと着弾、範囲 / 罠プレハブには子に魔法陣を付ける (以前は root が旧仮
  エフェクト用にスケール 60 を持ち、センサー球も ~190 m に拡大されていた)。RockWall には隆起と崩壊のエフェクト、
  WindCutter / ArcaneRay / FrostBreath は新規。
エフェクトはメートル単位で作り、スケール 8 で再生する (MagicCaster のリグは 200 cm を 0.08 倍)。
"""
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.common.blob import Ver  # noqa: E402
from tools.common.cereal_json import Num, OrderedObj, dumps, loads, to_file_bytes  # noqa: E402
from tools.effect import xmlio  # noqa: E402
from tools.scene import edits, meta as scene_meta, model, reader, validate, writer  # noqa: E402

from event_board_prefab import strip_repeat_versions, vec3_blob  # noqa: E402
from game_over_prefab import Builder, asset_guid, check, new_prefab, prepare  # noqa: E402

EFFECT_DIR = REPO / 'Assets/Art/Effect/Magic'
SOUND_DIR = REPO / 'Assets/Audio/Magic'
SPELL_DATA_DIR = REPO / 'Assets/Data/Magic'
EFFECT_SOURCE_DIR = REPO / 'Assets/Art/Effect/_Source/Magic'
MAGIC_PREFAB_DIR = REPO / 'Assets/Prefab/Magic'
CAST_DIR = MAGIC_PREFAB_DIR / 'Cast'
FX_DIR = MAGIC_PREFAB_DIR / 'Effect'
METRE = 8.0            # エフェクト1単位あたりのワールド単位
LOOP, DESTROY = 0, 1   # ParticleSystem PlayMode
LOOP_SECS = 60.0       # Loop エフェクトはこの後にしか再開しない。オブジェクトはそのずっと前に破棄される
CAST_MOTIONS = ['OneHandThrust', 'OneHandSweep', 'OneHandUppercut', 'OneHandRaise', 'TwoHandRaise', 'TwoHandSlam',
                'TwoHandBurst', 'TwoHandThrow', 'TwoHandSwingPush', 'TwoHandBeam', 'TwoHandPushHold', 'TwoHandPray']
MOTION_SPELL = {'OneHandThrust': 'MagicBolt', 'OneHandSweep': 'WindCutter', 'OneHandUppercut': 'RockWall',
                'OneHandRaise': 'ThunderTrap', 'TwoHandRaise': 'Might', 'TwoHandSlam': 'QuakeBlast',
                'TwoHandBurst': 'ExplosionBlast', 'TwoHandThrow': 'FireBall', 'TwoHandSwingPush': 'VioletFlame',
                'TwoHandBeam': 'ArcaneRay', 'TwoHandPushHold': 'FrostBreath', 'TwoHandPray': 'Heal'}


def fx(effect):
    return asset_guid(EFFECT_DIR / f'{effect}.efkefc.meta')


def fx_secs(effect):
    project = xmlio.read(EFFECT_SOURCE_DIR / f'{effect}.efkproj')
    return round(int(project.child('EndFrame').text) / 60.0 + 0.1, 2)


def prefab_guid(path):
    return asset_guid(Path(str(path) + '.meta'))


def sound(name):
    return asset_guid(SOUND_DIR / f'{name}.mp3.meta')


# ---------------------------------------------------------------- ヘルパー
def find_child(node, name):
    return next((c for c in node.transform.children if c.name == name), None)


def find_comp(node, leaf):
    return next((c for c in node.components if c.fqn.endswith('::' + leaf)), None)


def set_scale(node, s):
    node.transform.local_scale = model.Vec3(Num.of_float(s), Num.of_float(s), Num.of_float(s))


def set_field(comp, key, guid):
    edits._set_field_guid(comp.data[key], guid)


def set_particle(comp, effect, mode, secs):
    set_field(comp, 'particleFile_', fx(effect))
    comp.data['playingDuration_secs_'] = Num.of_float(float(secs))
    comp.data['playMode_'] = Num.of_int(mode)


def set_sound(prefab, node, sound_name, delay=0.0):
    comp = find_comp(node, 'SpawnSound') or Builder(prefab).component(node, 'SpawnSound')
    set_field(comp, 'sound_', sound(sound_name))
    comp.data['delay_secs_'] = Num.of_float(delay)


def add_particle(prefab, node, effect, mode, secs):
    comp = Builder(prefab).component(node, 'ParticleSystem')
    comp.data.pop('isRoop_', None)     # アーカイブしていたのはバージョン 0 だけ
    set_particle(comp, effect, mode, secs)
    return comp


def save(prefab, directory, name):
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / f'{name}.prefab'
    prepare([prefab.root])
    text = strip_repeat_versions(writer.write_prefab(prefab), '')
    check(text, validate.validate_prefab(reader.read_prefab(text)), path.name)
    path.write_bytes(to_file_bytes(text))
    meta = Path(str(path) + '.meta')
    if meta.exists():
        guid = asset_guid(meta)
    else:
        guid = scene_meta.mint_guid().upper()
        content_path = scene_meta.content_path_for(scene_meta.PREFAB_SPEC, name, directory, REPO)
        scene_meta.write_meta(scene_meta.PREFAB_SPEC, meta, name, guid, content_path)
    print(f'wrote {path.relative_to(REPO)}  ({guid})')
    return guid


# ---------------------------------------------------------------- 単発エフェクトのプレハブ
def particle_prefab(directory, name, effect, sound_name=None):
    prefab = new_prefab(name)
    set_scale(prefab.root, METRE)
    add_particle(prefab, prefab.root, effect, DESTROY, fx_secs(effect))
    if sound_name:
        set_sound(prefab, prefab.root, sound_name)
    return save(prefab, directory, name)


# ---------------------------------------------------------------- 魔法プレハブ
def retarget_projectile(path, name, travel, impact_guid, impact_secs, *, source=None, radius=None):
    """飛翔エフェクトは子の TravelEffect に、着弾プレハブは MagicProjectile に付ける。source: 先にそのプレハブをコピーする。
    radius: SphereCollider の radius_ (root のスケールが掛かる)。"""
    prefab = reader.read_prefab_file(source or path)
    if source:
        prefab = edits.copy_prefab(prefab)
    prefab.root.name = name
    if radius is not None:
        find_comp(prefab.root, 'SphereCollider').data['radius_'] = Num.of_float(radius)
    root_scale = float(edits._vec3_floats(prefab.root.transform.local_scale)[0])
    travel_node = find_child(prefab.root, 'TravelEffect')
    set_scale(travel_node, METRE / root_scale)
    set_particle(find_comp(travel_node, 'ParticleSystem'), travel, LOOP, LOOP_SECS)
    projectile = find_comp(prefab.root, 'MagicProjectile')
    set_field(projectile, 'impactPrefab_', impact_guid)
    projectile.data['impactLifeTime_secs_'] = Num.of_float(impact_secs)
    return save(prefab, path.parent, path.stem)


def restructure_blast(path, name, sigil, radius, detonate_guid, detonate_secs, sigil_sound):
    """root はスケール 1 でセンサー半径をワールド単位で持つ。魔法陣エフェクトはスケール 8 の子に付ける。"""
    prefab = reader.read_prefab_file(path)
    root = prefab.root
    root.name = name
    set_scale(root, 1.0)
    find_comp(root, 'SphereCollider').data['radius_'] = Num.of_float(radius)
    sigil_node = find_child(root, 'Sigil')
    if sigil_node is None:
        ps = find_comp(root, 'ParticleSystem')
        root.components.remove(ps)
        sigil_node = edits.add_gameobject(prefab, parent=root.guid, name='Sigil', scale=(METRE, METRE, METRE))
        sigil_node.components.append(ps)
    set_scale(sigil_node, METRE)
    set_particle(find_comp(sigil_node, 'ParticleSystem'), sigil, LOOP, LOOP_SECS)
    set_sound(prefab, sigil_node, sigil_sound)
    blast = find_comp(root, 'MagicBlast')
    set_field(blast, 'detonatePrefab_', detonate_guid)
    blast.data['detonateEffectLifeTime_secs_'] = Num.of_float(detonate_secs)
    return save(prefab, path.parent, path.stem)


def rock_wall(path, crumble_guid, crumble_secs):
    prefab = reader.read_prefab_file(path)
    root = prefab.root
    rise = find_child(root, 'RiseEffect')
    if rise is None:
        rise = edits.add_gameobject(prefab, parent=root.guid, name='RiseEffect', scale=(METRE, METRE, METRE))
        add_particle(prefab, rise, 'RockWall_Rise', DESTROY, fx_secs('RockWall_Rise'))
    else:
        set_particle(find_comp(rise, 'ParticleSystem'), 'RockWall_Rise', DESTROY, fx_secs('RockWall_Rise'))
    set_sound(prefab, rise, 'RockWall_Rise')
    placement = find_comp(root, 'MagicPlacement')
    set_field(placement, 'vanishPrefab_', crumble_guid)
    placement.data['vanishEffectLifeTime_secs_'] = Num.of_float(crumble_secs)
    return save(prefab, path.parent, path.stem)


def channel(template_path, out_name, root_name, effect, size, offset, hit_guid, hit_secs, visual_offset):
    """組み直した範囲プレハブから: SphereCollider -> 前方 (-Z) に伸びる BoxCollider、MagicBlast ->
    MagicChannel。2つのコライダー型は同じ基底をアーカイブし、最後のメンバーだけが違う。"""
    prefab = edits.copy_prefab(reader.read_prefab_file(template_path))
    root = prefab.root
    root.name = root_name
    collider = find_comp(root, 'SphereCollider')
    collider.fqn = 'NanamiEngine::Module::Component::BoxCollider'
    collider.class_version = 6
    collider.data.pop('radius_')
    collider.data.append('size_', vec3_blob(size))
    base = collider.data['value0']
    (base.body if isinstance(base, Ver) else base)['offset_'] = vec3_blob(offset)

    blast = find_comp(root, 'MagicBlast')
    blast.fqn = 'GamePlay::Magic::MagicChannel'
    blast.class_version = 0
    blast.data = OrderedObj([
        ('value0', blast.data['value0']),
        ('hitPrefab_', blast.data['detonatePrefab_']),
        ('hitEffectLifeTime_secs_', Num.of_float(hit_secs)),
        ('linger_secs_', Num.of_float(0.5)),
    ])
    set_field(blast, 'hitPrefab_', hit_guid)

    visual = find_child(root, 'Sigil')
    visual.name = 'Visual'
    # CastPoint は胸の 0.3 m 前だが、クリップでは手が ~1.1 m 前にあるので見た目はそこから始める
    visual.transform.local_pos = model.Vec3(*(Num.of_float(v) for v in visual_offset))
    visual.components = [c for c in visual.components if not c.fqn.endswith('::SpawnSound')]  # 魔法陣のもの
    set_particle(find_comp(visual, 'ParticleSystem'), effect, LOOP, LOOP_SECS)
    return save(prefab, MAGIC_PREFAB_DIR, out_name)


def wire_cast_sounds():
    """castSound_ (ファイル内で最初の Field<SoundFile> なのでバージョンキーを持つ) -> <Spell>_Release。"""
    for spell in MOTION_SPELL.values():
        path = SPELL_DATA_DIR / f'{spell}.magicSpell.meta'
        text = path.read_bytes().decode('utf-8-sig')
        tree = loads(text)
        field = tree['value0']['ptr_wrapper']['data']['castSound_']
        field['value0']['ptr_wrapper']['data']['value0']['value_'] = sound(f'{spell}_Release')
        path.write_bytes(dumps(tree, newline='\r\n').encode('utf-8'))
        print(f'castSound_ {spell:14s} -> {spell}_Release')


def main():
    cast = {m: particle_prefab(CAST_DIR, f'Cast_{m}', f'Cast_{m}', f'{MOTION_SPELL[m]}_Charge') for m in CAST_MOTIONS}

    one_shots = {
        'MagicBoltImpact': 'MagicBolt_Impact', 'FireBallImpact': 'FireBall_Impact',
        'VioletFlameImpact': 'VioletFlame_Impact', 'WindCutterImpact': 'WindCutter_Impact',
        'ExplosionBlastBurst': 'ExplosionBlast_Burst', 'QuakeBlastBurst': 'QuakeBlast_Burst',
        'ThunderTrapStrike': 'ThunderTrap_Strike', 'RockWallCrumble': 'RockWall_Crumble',
        'HealBloom': 'Heal_Bloom', 'MightSurge': 'Might_Surge',
        'ArcaneRayHit': 'ArcaneRay_Hit', 'FrostBreathHit': 'FrostBreath_Hit',
    }
    one_shot_sounds = {
        'MagicBoltImpact': 'MagicBolt_Impact', 'FireBallImpact': 'FireBall_Impact',
        'VioletFlameImpact': 'VioletFlame_Impact', 'WindCutterImpact': 'WindCutter_Impact',
        'ExplosionBlastBurst': 'ExplosionBlast_Burst', 'QuakeBlastBurst': 'QuakeBlast_Burst',
        'ThunderTrapStrike': 'ThunderTrap_Strike', 'RockWallCrumble': 'RockWall_Crumble',
        'ArcaneRayHit': 'ArcaneRay_Hit', 'FrostBreathHit': 'FrostBreath_Hit',
    }   # HealBloom / MightSurge は魔法の castSound_ で鳴らす
    fxp = {name: particle_prefab(FX_DIR, name, effect, one_shot_sounds.get(name)) for name, effect in one_shots.items()}
    secs = {name: fx_secs(effect) for name, effect in one_shots.items()}

    retarget_projectile(REPO / 'Assets/Prefab/Bullet/MagicBolt.prefab', 'MagicBolt', 'MagicBolt_Travel',
                        fxp['MagicBoltImpact'], secs['MagicBoltImpact'])
    retarget_projectile(MAGIC_PREFAB_DIR / 'FireBallSpell.prefab', 'FireBall', 'FireBall_Travel',
                        fxp['FireBallImpact'], secs['FireBallImpact'])
    retarget_projectile(MAGIC_PREFAB_DIR / 'VioletFlameSpell.prefab', 'VioletFlame', 'VioletFlame_Travel',
                        fxp['VioletFlameImpact'], secs['VioletFlameImpact'])
    wind = MAGIC_PREFAB_DIR / 'WindCutterSpell.prefab'
    # 刃の幅は 2.6 m: 0.75 m の球 (root x0.12) なら 1.2 m の発動点から撃っても地面に当たらない
    retarget_projectile(wind, 'WindCutter', 'WindCutter_Travel', fxp['WindCutterImpact'], secs['WindCutterImpact'],
                        source=None if wind.exists() else MAGIC_PREFAB_DIR / 'VioletFlameSpell.prefab', radius=50.0)

    restructure_blast(MAGIC_PREFAB_DIR / 'ExplosionBlastSpell.prefab', 'ExplosionBlast', 'ExplosionBlast_Sigil',
                      28.0, fxp['ExplosionBlastBurst'], secs['ExplosionBlastBurst'], 'ExplosionBlast_Sigil')
    restructure_blast(MAGIC_PREFAB_DIR / 'QuakeBlastSpell.prefab', 'QuakeBlast', 'QuakeBlast_Sigil',
                      34.0, fxp['QuakeBlastBurst'], secs['QuakeBlastBurst'], 'QuakeBlast_Sigil')
    restructure_blast(MAGIC_PREFAB_DIR / 'ThunderTrapSpell.prefab', 'ThunderTrap', 'ThunderTrap_Circle',
                      20.0, fxp['ThunderTrapStrike'], secs['ThunderTrapStrike'], 'ThunderTrap_Set')
    rock_wall(MAGIC_PREFAB_DIR / 'RockWallSpell.prefab', fxp['RockWallCrumble'], secs['RockWallCrumble'])

    template = MAGIC_PREFAB_DIR / 'ExplosionBlastSpell.prefab'
    # ビーム: 太さ 1.6 m、長さ 20 m。ブレス: 幅 4 m、高さ 2.6 m、長さ 7 m で底面が地面に届く箱
    channel(template, 'ArcaneRaySpell', 'ArcaneRay', 'ArcaneRay_Beam', (13.0, 13.0, 160.0), (0.0, 0.0, -80.0),
            fxp['ArcaneRayHit'], secs['ArcaneRayHit'], (0.0, 1.3, -6.4))
    channel(template, 'FrostBreathSpell', 'FrostBreath', 'FrostBreath_Cone', (32.0, 21.0, 56.0), (0.0, -2.5, -28.0),
            fxp['FrostBreathHit'], secs['FrostBreathHit'], (0.0, 0.0, -6.4))
    wire_cast_sounds()

    print('\ncast effect prefabs:')
    for m, g in cast.items():
        print(f'  {m:18s} {g}')
    print('one-shot prefabs:')
    for n, g in fxp.items():
        print(f'  {n:20s} {g}  {secs[n]}s')


if __name__ == '__main__':
    main()
