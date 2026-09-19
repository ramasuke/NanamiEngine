"""Small node-building layer over tools.effect.presets for the MagicCaster spell effects.

Conventions (see tools/art/magic_spell_effects.py):
* 1 effect unit = 1 m. The prefabs play the effects at world scale 8 (the MagicCaster is 0.08 x a 200 cm rig).
* Editor axes: +Y up, +Z = the caster's forward, -X = the caster's right. EffekseerForDXLib runs in LH and negates
  Z of every location / velocity / generation position on load, so editor +Z ends up on DxLib's -Z, which is the
  avatar's and the spells' forward. Only positions and velocities say "forward" here; shapes are kept symmetric
  front-to-back so nothing depends on how a renderer lays out its own vertices.
* Frames are Effekseer frames (60 fps). Velocities are per frame.
"""
from __future__ import annotations

from tools.effect import presets as P
from tools.effect.model import Elem

LAYOUT = 'legacy'
OPAQUE, BLEND, ADD = 0, 1, 2
FACE, YAXIS, FIXED = 0, 1, 2          # Sprite/Ring Billboard
NOBIND, ON_CREATE, ALWAYS = 0, 1, 2   # CommonValues Location/Rotation/ScaleEffectType
LIVE_LONG = 1200                      # "as long as the effect is playing" for single-instance layers


def rng(v):
    """number -> fixed PVA dict; (lo, hi) -> random range; dict passes through."""
    if isinstance(v, dict):
        return v
    if isinstance(v, (tuple, list)):
        lo, hi = v
        return {'center': (lo + hi) / 2, 'max': hi, 'min': lo}
    return {'center': v, 'max': v, 'min': v}


def _xyz_pva(tag, xyz):
    x, y, z = xyz
    return P.pva(tag, x=rng(x), y=rng(y), z=rng(z))


def _rgba_pva(tag, rgba, spread=None):
    spread = spread or (0, 0, 0, 0)
    chans = {}
    for key, val, sp in zip('rgba', rgba, spread):
        chans[key] = {'center': val, 'max': min(255, val + sp), 'min': max(0, val - sp)}
    return P.random_color(tag, **chans)


def _fade(v):
    if v is None:
        return None
    if isinstance(v, (tuple, list)):
        frame, s0, s1 = v
        return {'frame': frame, 'start_speed': s0, 'end_speed': s1}
    return {'frame': v}


def _common(*, count, interval, delay, life, infinite, bind, bind_rot, bind_scale):
    return P.common_values(
        layout=LAYOUT, max_generation=count, infinite=True if infinite else None,
        location_effect_type=bind, rotation_effect_type=bind_rot, scale_effect_type=bind_scale,
        remove_when_life_extinct=True, life=rng(life), generation_time=rng(interval),
        generation_time_offset=rng(delay))


def _location(at, at_rand, vel, acc, move):
    if move is not None:
        start, end, s0, s1 = (list(move) + [0, 0])[:4]
        return P.location_values(easing=P.easing('Easing', start=_xyz_pva('Start', start), end=_xyz_pva('End', end),
                                                 start_speed=s0, end_speed=s1))
    if vel is not None or acc is not None or at_rand is not None:
        e = Elem('LocationValues')
        e.children.append(Elem('Type', text='1'))
        pv = Elem('PVA')
        pv.children.append(_xyz_pva('Location', at_rand or at or (0, 0, 0)))
        pv.children.append(_xyz_pva('Velocity', vel or (0, 0, 0)))
        pv.children.append(_xyz_pva('Acceleration', acc or (0, 0, 0)))
        e.children.append(pv)
        return e
    if at is not None:
        x, y, z = at
        return P.location_values(fixed_xyz={'x': x, 'y': y, 'z': z})
    return None


def _rotation(rot, rot_rand, spin):
    if spin is not None or rot_rand is not None:
        e = Elem('RotationValues')
        e.children.append(Elem('Type', text='1'))
        pv = Elem('PVA')
        pv.children.append(_xyz_pva('Rotation', rot_rand or rot or (0, 0, 0)))
        pv.children.append(_xyz_pva('Velocity', spin or (0, 0, 0)))
        e.children.append(pv)
        return e
    if rot is not None:
        x, y, z = rot
        return P.rotation_values(fixed=P.xyz('Rotation', x, y, z))
    return None


def _scaling(size, grow, grow_xyz):
    if grow_xyz is not None:
        start, end, s0, s1 = (list(grow_xyz) + [0, 0])[:4]
        return P.scaling_values(easing=P.easing('Easing', start=_xyz_pva('Start', start), end=_xyz_pva('End', end),
                                                start_speed=s0, end_speed=s1))
    if grow is not None:
        start, end, s0, s1 = (list(grow) + [0, 0])[:4]
        return P.scaling_values(single_easing=P.easing('SingleEasing', start=P.pva('Start', **rng(start)),
                                                       end=P.pva('End', **rng(end)), start_speed=s0, end_speed=s1))
    if size is not None:
        if isinstance(size, (tuple, list)) and len(size) == 2:
            return P.scaling_values(single_pva=rng(size))          # one random uniform size per particle
        if isinstance(size, (tuple, list)) and len(size) == 3:
            if any(isinstance(v, (tuple, list)) for v in size):
                x, y, z = size
                return P.scaling_values(pva_scale=P.pva('Scale', x=rng(x), y=rng(y), z=rng(z)))
            x, y, z = size
        else:
            x = y = z = size
        return P.scaling_values(fixed=P.xyz('Scale', x, y, z))
    return None


def emit_point(x, y, z):
    return P.generation_location_point(location={'x': rng(x), 'y': rng(y), 'z': rng(z)})


def emit_sphere(radius, *, upper=False, effects_rotation=True):
    return P.generation_location_sphere(radius=rng(radius), rotation_x=rng((-90, 90) if upper else (-180, 180)),
                                        rotation_y=rng((-180, 180)), effects_rotation=effects_rotation)


def emit_circle(radius, *, axis=1, division=16, ordered=False, effects_rotation=True, angle=None):
    e = P.generation_location_circle(division=division, circle_type=1 if ordered else 0, radius=rng(radius),
                                     angle_start=rng(angle[0]) if angle else None,
                                     angle_end=rng(angle[1]) if angle else None,
                                     effects_rotation=effects_rotation)
    e.child('Circle').children.insert(0, Elem('AxisDirection', text=str(axis)))
    return e


def N(name, *, kind='sprite', tex=None, blend=ADD, life=30, count=1, interval=0, delay=0, infinite=False,
      bind=None, bind_rot=None, bind_scale=None, at=None, at_rand=None, vel=None, acc=None, move=None,
      rot=None, rot_rand=None, spin=None, size=None, grow=None, grow_xyz=None,
      color=(255, 255, 255, 255), color_to=None, color_spread=None, ease=(0, 0),
      billboard=FACE, fade_in=None, fade_out=None, emit=None, gravity=None, attract=None,
      uv_anim=None, uv_scroll=None, ring=None, children=None):
    """One Effekseer node. ``kind`` is 'sprite' | 'ring' | 'group'."""
    common = _common(count=count, interval=interval, delay=delay, life=life, infinite=infinite,
                     bind=bind, bind_rot=bind_rot, bind_scale=bind_scale)
    kw = dict(common=common, location=_location(at, at_rand, vel, acc, move), rotation=_rotation(rot, rot_rand, spin),
              scaling=_scaling(size, grow, grow_xyz), generation_location=emit)
    if gravity is not None:
        gx, gy, gz = gravity
        kw['location_abs'] = P.location_abs_values(gravity={'x': gx, 'y': gy, 'z': gz})
    elif attract is not None:
        kw['location_abs'] = P.location_abs_values(attractive_force=attract)
    kw = {k: v for k, v in kw.items() if v is not None}
    if kind == 'group':
        return P.group_node(name, children=children or [], **kw)

    rc = dict(filter_=1, alpha_blend=blend, fade_in=_fade(fade_in), fade_out=_fade(fade_out))
    if tex:
        rc['color_texture'] = f'Texture/{tex}.png'
    if uv_anim:
        rc['uv_animation'] = uv_anim
    if uv_scroll:
        rc['uv_scroll'] = uv_scroll
    kw['renderer_common'] = P.renderer_common(**rc)

    if kind == 'sprite':
        if color_to is not None:
            block = P.sprite(billboard=billboard, color_all_easing=P.easing(
                'ColorAll_Easing', start=_rgba_pva('Start', color, color_spread), end=_rgba_pva('End', color_to),
                start_speed=ease[0], end_speed=ease[1]))
        elif color_spread is not None:
            block = P.sprite(billboard=billboard, color_all_random=_rgba_pva('ColorAll_Random', color, color_spread))
        else:
            block = P.sprite(billboard=billboard, color_all=P.color('ColorAll_Fixed', *color))
        return P.sprite_node(name, sprite_block=block, children=children, **kw)

    if kind == 'ring':
        ring = dict(ring or {})
        outer_r, outer_h = ring.get('outer', (1.0, 0.0))
        inner_r, inner_h = ring.get('inner', (0.8, 0.0))
        c_outer = ring.get('outer_color', color)
        c_center = ring.get('center_color', color)
        c_inner = ring.get('inner_color', color)
        block = P.ring(vertex_count=ring.get('vertices', 48),
                       outer=P.elem('Location', X=outer_r, Y=outer_h), inner=P.elem('Location', X=inner_r, Y=inner_h),
                       center_ratio=ring.get('center_ratio', 0.5),
                       outer_color=P.color('OuterColor_Fixed', *c_outer),
                       center_color=P.color('CenterColor_Fixed', *c_center),
                       inner_color=P.color('InnerColor_Fixed', *c_inner))
        block.children.insert(0, Elem('Billboard', text=str(billboard)))
        return P.ring_node(name, ring_block=block, children=children, **kw)
    raise ValueError(kind)


def G(name, children, **kw):
    return N(name, kind='group', children=children, **kw)


def flipbook(frames_x, cell_w, cell_h, frame_length, loop=True):
    return {'start': {'x': 0, 'y': 0}, 'size': {'x': cell_w, 'y': cell_h}, 'frame_length': frame_length,
            'frame_count_x': frames_x, 'frame_count_y': 1, 'loop_type': 1 if loop else 0}


def project(children, end_frame, *, loop=False):
    return P.new_project(start_frame=0, end_frame=end_frame, is_loop=loop, tool_version='0.7CTP1',
                         root_children=children)
