"""DxLib 用に、アニメーションクリップを Mixamo キャラクター *自身の FBX の中に* 書き込む。

DxLib の MV1AttachAnim はフレームを階層位置で対応付け (AnimationClipNode は NameCheck オフでアタッチする)、
再生時に *モデル側の* FBX PreRotation を適用する。Mixamo のキャラクターはどちらも異なる (ボーン数 - 目・髪・
武器 - メッシュノードの配置、ボーンごとの PreRotation) ので、あるキャラクターで書き出したクリップは別のキャラクター
ではねじれるか全く動かない。対象キャラクターの FBX (その "mixamo.com" スタック、"Take 001" は削除) にクリップを
書き込むと、`tools.model convert --mode anim` がモデルとフレームの完全に一致するクリップを作れる。

    python tools/art/mixamo_clip.py retarget <character.fbx> <clip.fbx> <out.fbx>

retarget: 各ボーンのリグのレストポーズからのワールド空間回転差分 (Lcl Rotation 0 = PreRotation のみ。
Mixamo のリグはどれも T ポーズがレスト) を対象のレストポーズに適用する。hips の移動差分は腰の高さでスケールする。
クリップにない中指/薬指/小指 (親指と人差し指だけのリグ。例: Mixamo の "Peasant Man") は人差し指に従う。
それ以外のボーンがクリップに欠けていればエラー。クリップ側の余分なボーンは無視する。
"""
from __future__ import annotations

import sys
from pathlib import Path

import numpy as np
from scipy.spatial.transform import Rotation as Rot

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))
from tools.art import fbx_binary as fbxio  # noqa: E402

TICKS_PER_SEC = 46186158000
FPS = 30
TICKS_PER_FRAME = TICKS_PER_SEC // FPS


def s(v):
    return v.split(b'\x00')[0].decode() if isinstance(v, bytes) else v


def p70(node):
    out = {}
    pp = node.first('Properties70')
    if pp:
        for p in pp.children:
            out[s(p.v(0))] = [x[1] for x in p.props[4:]]
    return out


class Scene:
    def __init__(self, path):
        self.doc = fbxio.load(path)
        self.objs = self.doc.first('Objects')
        self.conns = self.doc.first('Connections')
        self.byid = {o.v(0): o for o in self.objs.children}
        self.bones = {}      # 名前 -> モデルノード
        self.parent = {}     # ボーン名 -> 親ボーン名 (ルートは None)
        self.order = []
        id2name = {}
        for m in self.objs.find('Model'):
            if s(m.v(2)) == 'LimbNode':
                n = s(m.v(1)).replace('Model::', '')
                self.bones[n] = m
                id2name[m.v(0)] = n
        for c in self.conns.children:
            if s(c.v(0)) == 'OO' and c.v(1) in id2name:
                child = id2name[c.v(1)]
                par = c.v(2)
                if par in id2name:
                    self.parent[child] = id2name[par]
                elif child not in self.parent:
                    self.parent[child] = None
        for n in self.bones:
            self.parent.setdefault(n, None)
        # トポロジカル順 (親が先)
        done = set()

        def visit(n):
            if n in done:
                return
            if self.parent[n]:
                visit(self.parent[n])
            done.add(n)
            self.order.append(n)
        for n in self.bones:
            visit(n)
        self.props = {n: p70(m) for n, m in self.bones.items()}

    def pre(self, n):
        return Rot.from_euler('xyz', self.props[n].get('PreRotation', [0, 0, 0]), degrees=True)

    def rest_t(self, n):
        return np.array(self.props[n].get('Lcl Translation', [0, 0, 0]), dtype=float)

    def rest_r(self, n):
        return Rot.from_euler('xyz', self.props[n].get('Lcl Rotation', [0, 0, 0]), degrees=True)

    def curve_nodes(self, stack_name):
        """スタックの (唯一の) レイヤーの {(bone, 'T'|'R'): {axis: curve_node}}。"""
        stack = next(st for st in self.objs.find('AnimationStack') if s(st.v(1)).endswith(stack_name))
        children = {}
        for c in self.conns.children:
            children.setdefault(c.v(2), []).append(c)
        layer = next(c.v(1) for c in children[stack.v(0)] if self.byid[c.v(1)].name == 'AnimationLayer')
        result = {}
        for c in children[layer]:
            cn = self.byid[c.v(1)]
            target = None
            for c2 in self.conns.children:
                if c2.v(1) == cn.v(0) and s(c2.v(0)) == 'OP':
                    m = self.byid.get(c2.v(2))
                    if m is not None and m.name == 'Model':
                        target = (s(m.v(1)).replace('Model::', ''), s(c2.v(3)))
            if target is None:
                continue
            kind = {'Lcl Translation': 'T', 'Lcl Rotation': 'R', 'Lcl Scaling': 'S'}.get(target[1])
            if kind is None:
                continue
            curves = {}
            for c3 in children.get(cn.v(0), []):
                cv = self.byid.get(c3.v(1))
                if cv is not None and cv.name == 'AnimationCurve':
                    curves[s(c3.v(3))] = cv
            result[(target[0], kind)] = (cn, curves)
        return stack, layer, result


def eval_curve(curve, t):
    times = np.array(curve.first('KeyTime').v(0), dtype=np.int64)
    vals = np.array(curve.first('KeyValueFloat').v(0), dtype=float)
    if len(times) == 1:
        return np.full_like(t, vals[0], dtype=float)
    # 傾き自動 (Catmull-Rom 風) の3次エルミート
    tt = times.astype(float)
    slopes = np.zeros_like(vals)
    slopes[1:-1] = (vals[2:] - vals[:-2]) / (tt[2:] - tt[:-2])
    slopes[0] = (vals[1] - vals[0]) / (tt[1] - tt[0])
    slopes[-1] = (vals[-1] - vals[-2]) / (tt[-1] - tt[-2])
    out = np.empty(len(t))
    for i, x in enumerate(t):
        if x <= tt[0]:
            out[i] = vals[0]; continue
        if x >= tt[-1]:
            out[i] = vals[-1]; continue
        k = np.searchsorted(tt, x) - 1
        h = tt[k + 1] - tt[k]
        u = (x - tt[k]) / h
        h00 = 2 * u ** 3 - 3 * u ** 2 + 1
        h10 = u ** 3 - 2 * u ** 2 + u
        h01 = -2 * u ** 3 + 3 * u ** 2
        h11 = u ** 3 - u ** 2
        out[i] = h00 * vals[k] + h10 * h * slopes[k] + h01 * vals[k + 1] + h11 * h * slopes[k + 1]
    return out


def _channels(cn, curves, t):
    defaults = p70(cn)
    cols = []
    for ax in 'XYZ':
        key = 'd|' + ax
        if key in curves:
            cols.append(eval_curve(curves[key], t))
        else:
            cols.append(np.full(len(t), float(defaults.get(key, [0.0])[0])))
    return np.stack(cols, axis=1)


def sample(scene, stack_name, nframes):
    _, _, cns = scene.curve_nodes(stack_name)
    t = np.arange(nframes, dtype=float) * TICKS_PER_FRAME
    rot, trans = {}, {}
    for n in scene.bones:
        if (n, 'R') in cns:
            rot[n] = Rot.from_euler('xyz', _channels(*cns[(n, 'R')], t), degrees=True)
        else:
            rot[n] = Rot.from_euler('xyz', np.tile(scene.props[n].get('Lcl Rotation', [0, 0, 0]), (nframes, 1)), degrees=True)
        if (n, 'T') in cns:
            trans[n] = _channels(*cns[(n, 'T')], t)
        else:
            trans[n] = np.tile(scene.rest_t(n), (nframes, 1))
    return rot, trans


def world_rest(scene):
    g = {}
    for n in scene.order:
        par = scene.parent[n]
        local = scene.pre(n)
        g[n] = (g[par] * local) if par else local
    return g


def world_anim(scene, rot):
    g = {}
    for n in scene.order:
        par = scene.parent[n]
        local = scene.pre(n) * rot[n]
        g[n] = (g[par] * local) if par else local
    return g


def world_positions(scene, rot, trans, frame):
    pos, g = {}, {}
    for n in scene.order:
        par = scene.parent[n]
        r = scene.pre(n) * rot[n][frame]
        t = trans[n][frame]
        if par:
            g[n] = g[par] * r
            pos[n] = pos[par] + g[par].apply(t)
        else:
            g[n] = r
            pos[n] = t
    return pos


def unwrap_euler(e):
    """各オイラーチャネルを連続に保つ (前フレームに最も近い等価な3つ組を選ぶ)。"""
    out = e.copy()
    for i in range(1, len(out)):
        prev = out[i - 1]
        cands = []
        a, b, c = out[i]
        for cand in ((a, b, c), (a + 180, 180 - b, c + 180)):
            cand = np.array(cand)
            cand = cand + 360 * np.round((prev - cand) / 360)
            cands.append(cand)
        out[i] = min(cands, key=lambda x: np.abs(x - prev).sum())
    return out


def set_curve(curve, times, values):
    n = len(times)
    for ch in curve.children:
        if ch.name == 'Default':
            ch.props = [('D', float(values[0]))]
        elif ch.name == 'KeyTime':
            ch.props = [('l', [int(x) for x in times])]
        elif ch.name == 'KeyValueFloat':
            ch.props = [('f', [float(x) for x in values])]
        elif ch.name == 'KeyAttrFlags':
            ch.props = [('i', [8456])]
        elif ch.name == 'KeyAttrDataFloat':
            ch.props = [('f', [0.0, 0.0, 4.099946090724554e-31, 0.0])]
        elif ch.name == 'KeyAttrRefCount':
            ch.props = [('i', [n])]


def remove_stack(scene, stack_name):
    """スタックとそのレイヤー・カーブノード・カーブ、およびそれらに触れる接続をすべて削除する。"""
    stack = next(st for st in scene.objs.find('AnimationStack') if s(st.v(1)).endswith(stack_name))
    kill = {stack.v(0)}
    changed = True
    while changed:
        changed = False
        for c in scene.conns.children:
            if c.v(2) in kill and c.v(1) not in kill and scene.byid.get(c.v(1)) is not None \
                    and scene.byid[c.v(1)].name in ('AnimationLayer', 'AnimationCurveNode', 'AnimationCurve'):
                kill.add(c.v(1)); changed = True
    scene.objs.children = [o for o in scene.objs.children if o.v(0) not in kill]
    scene.conns.children = [c for c in scene.conns.children if c.v(1) not in kill and c.v(2) not in kill]
    takes = scene.doc.first('Takes')
    takes.children = [t for t in takes.children if not (t.name == 'Take' and s(t.v(0)) == stack_name)]
    return len(kill)


def source_bone(n, src_bones):
    if n in src_bones:
        return n
    for finger in ('Middle', 'Ring', 'Pinky'):
        index = n.replace('Hand' + finger, 'HandIndex')
        if index != n and index in src_bones:
            return index
    return None


def main(target_path, source_path, out_path):
    tgt = Scene(target_path)
    src = Scene(source_path)

    src_stack = next(st for st in src.objs.find('AnimationStack'))
    stop = p70(src_stack)['LocalStop'][0]
    nframes = int(round(stop / TICKS_PER_FRAME)) + 1
    print('source frames', nframes, 'stop', stop / TICKS_PER_SEC, 's')

    src_rot, src_trans = sample(src, s(src_stack.v(1)), nframes)
    gs0 = world_rest(src)
    gs = world_anim(src, src_rot)
    gt0 = world_rest(tgt)

    src_of = {n: source_bone(n, src.bones) for n in tgt.bones}
    missing = [n for n, sn in src_of.items() if sn is None]
    if missing:
        raise SystemExit('target bones missing in source: %s' % missing)
    stand_ins = sorted(n for n, sn in src_of.items() if sn != n)
    if stand_ins:
        print('driven by the index finger:', len(stand_ins), 'bones')

    gt = {}
    tgt_rot = {}
    for n in tgt.order:
        sn = src_of[n]
        d = gs[sn] * gs0[sn].inv()
        gt[n] = d * gt0[n]
        par = tgt.parent[n]
        parent_g = gt[par] if par else Rot.identity(nframes)
        local = tgt.pre(n).inv() * parent_g.inv() * gt[n]
        tgt_rot[n] = local

    hips = next(n for n in tgt.bones if n.endswith('Hips'))
    k = tgt.rest_t(hips)[1] / src.rest_t(hips)[1]
    hips_t = tgt.rest_t(hips) + (src_trans[hips] - src.rest_t(hips)) * k
    print('hip scale', k)
    write_clip(tgt, tgt_rot, hips_t, nframes, out_path)
    verify(src, src_rot, src_trans, out_path, nframes)


def write_clip(tgt, tgt_rot, hips_t, nframes, out_path):
    """フレームごとのローカル回転 (ボーンごとの Rotation スタック) と hips の移動を
    対象 FBX 自身の 'mixamo.com' スタックに書き込み、'Take 001' を削除して保存する。"""
    hips = next(n for n in tgt.bones if n.endswith('Hips'))
    # 宙に浮いた接続 (ファイルにないカーブへの接続) は下で実際のカーブに置き換える
    live = {o.v(0) for o in tgt.objs.children}
    before = len(tgt.conns.children)
    tgt.conns.children = [c for c in tgt.conns.children if c.v(1) in live and (c.v(2) in live or c.v(2) == 0)]
    print('dropped dangling connections:', before - len(tgt.conns.children))

    template = next(o for o in tgt.objs.children if o.name == 'AnimationCurve')
    next_id = [max(live) + 1]

    def ensure_curves(cn, cv):
        for ax in 'XYZ':
            key = 'd|' + ax
            if key in cv:
                continue
            nid = next_id[0]; next_id[0] += 1
            node = fbxio.Node('AnimationCurve', [('L', nid), template.props[1], template.props[2]],
                              [fbxio.Node(c.name, list(c.props), []) for c in template.children])
            idx = tgt.objs.children.index(cn)
            tgt.objs.children.insert(idx + 1, node)
            tgt.conns.children.append(fbxio.Node('C', [('S', b'OP'), ('L', nid), ('L', cn.v(0)), ('S', key.encode())], []))
            cv[key] = node

    # 対象自身の "mixamo.com" スタックに書き込む
    stack, layer, cns = tgt.curve_nodes('mixamo.com')
    times = np.arange(nframes, dtype=np.int64) * TICKS_PER_FRAME
    written = 0
    for n in tgt.bones:
        if (n, 'R') not in cns:
            raise SystemExit('no rotation curve node for %s' % n)
        e = unwrap_euler(tgt_rot[n].as_euler('xyz', degrees=True))
        cn, cv = cns[(n, 'R')]
        ensure_curves(cn, cv)
        for i, ax in enumerate('XYZ'):
            set_curve(cv['d|' + ax], times, e[:, i])
        pp = cn.first('Properties70')
        for p in pp.children:
            name = s(p.v(0))
            if name in ('d|X', 'd|Y', 'd|Z'):
                p.props[4] = ('D', float(e[0, 'XYZ'.index(name[-1])]))
        written += 1
    cn, cv = cns[(hips, 'T')]
    ensure_curves(cn, cv)
    for i, ax in enumerate('XYZ'):
        set_curve(cv['d|' + ax], times, hips_t[:, i])
    for p in cn.first('Properties70').children:
        name = s(p.v(0))
        if name in ('d|X', 'd|Y', 'd|Z'):
            p.props[4] = ('D', float(hips_t[0, 'XYZ'.index(name[-1])]))
    print('rotation curve nodes written', written)

    last = int(times[-1])
    for p in stack.first('Properties70').children:
        if s(p.v(0)) in ('LocalStop', 'ReferenceStop'):
            p.props[4] = ('L', last)
    takes = tgt.doc.first('Takes')
    for t in takes.children:
        if t.name == 'Take' and s(t.v(0)) == 'mixamo.com':
            for ch in t.children:
                if ch.name in ('LocalTime', 'ReferenceTime'):
                    ch.props = [('L', 0), ('L', last)]
    print('removed objects of Take 001:', remove_stack(tgt, 'Take 001'))
    for t in takes.children:
        if t.name == 'Current':
            t.props = [('S', b'mixamo.com')]

    fbxio.save(tgt.doc, out_path)


def verify(src, src_rot, src_trans, out_path, nframes):
    # --- 検証: ワールド空間での手足の向きを比べる (元 vs リターゲット後) ---
    chk = Scene(out_path)
    hips = next(n for n in chk.bones if n.endswith('Hips'))
    rot2, trans2 = sample(chk, 'mixamo.com', nframes)
    worst = 0.0
    report = []
    for f in (0, nframes // 3, 2 * nframes // 3, nframes - 1):
        ps = world_positions(src, src_rot, src_trans, f)
        pt = world_positions(chk, rot2, trans2, f)
        for n in chk.bones:
            par = chk.parent[n]
            if not par or n.endswith('_End') or n not in ps or par not in ps:
                continue
            vs = ps[n] - ps[par]; vt = pt[n] - pt[par]
            if np.linalg.norm(vs) < 1e-3 or np.linalg.norm(vt) < 1e-3:
                continue
            ang = np.degrees(np.arccos(np.clip(np.dot(vs, vt) / np.linalg.norm(vs) / np.linalg.norm(vt), -1, 1)))
            worst = max(worst, ang)
            report.append((ang, f, n))
    report.sort(reverse=True)
    print('largest bone-direction differences (deg, frame, bone):')
    for r in report[:8]:
        print('  %.1f  f%d  %s' % r)
    # 参考としてレストポーズの差
    ps0 = world_positions(src, {n: Rot.identity(1) for n in src.bones}, {n: src.rest_t(n)[None] for n in src.bones}, 0)
    pt0 = world_positions(chk, {n: Rot.identity(1) for n in chk.bones}, {n: chk.rest_t(n)[None] for n in chk.bones}, 0)
    rest = []
    for n in chk.bones:
        par = chk.parent[n]
        if not par or n not in ps0 or par not in ps0:
            continue
        vs = ps0[n] - ps0[par]; vt = pt0[n] - pt0[par]
        if np.linalg.norm(vs) < 1e-3 or np.linalg.norm(vt) < 1e-3:
            continue
        rest.append((np.degrees(np.arccos(np.clip(np.dot(vs, vt) / np.linalg.norm(vs) / np.linalg.norm(vt), -1, 1))), n))
    rest.sort(reverse=True)
    print('largest rest-pose differences:', [('%.1f' % a, n) for a, n in rest[:6]])
    lf = world_positions(chk, rot2, trans2, 0)
    print('frame0 target hand heights L/R:', round(lf['mixamorig:LeftHand'][1], 1), round(lf['mixamorig:RightHand'][1], 1),
          'hips', np.round(lf[hips], 1))


if __name__ == '__main__':
    if len(sys.argv) != 5 or sys.argv[1] != 'retarget':
        raise SystemExit('usage: python tools/art/mixamo_clip.py retarget <character.fbx> <clip.fbx> <out.fbx>')
    main(*sys.argv[2:5])
