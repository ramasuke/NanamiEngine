"""Minimal TRS (translate/rotate/scale) math for computing world transforms.

Used only by ``move_gameobject``'s world-transform-preserving reparent - the
engine's own :class:`Transform` recomputes ``worldMatrix_`` at load time
regardless, so this never needs to touch that opaque blob, only local
pos/rot/scale. Quaternions are ``(x, y, z, w)`` tuples of plain floats.

Known limitation (shared with most scene-graph engines, including Unity before
it started warning about it): composing a rotation with a *non-uniform* scale
is not, in general, decomposable back into a clean rotation + non-uniform
scale after reparenting under another rotation - the componentwise scale
composition here matches this engine's own apparent convention (see
``Transform::UpdateMatrix``) rather than attempting a fully general polar
decomposition.
"""

from __future__ import annotations

Vec3 = tuple[float, float, float]
Quat = tuple[float, float, float, float]

IDENTITY_POS: Vec3 = (0.0, 0.0, 0.0)
IDENTITY_ROT: Quat = (0.0, 0.0, 0.0, 1.0)
IDENTITY_SCALE: Vec3 = (1.0, 1.0, 1.0)


def quat_mul(a: Quat, b: Quat) -> Quat:
    """a (*) b - rotating a vector by the result applies b first, then a."""
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (
        aw * bx + ax * bw + ay * bz - az * by,
        aw * by - ax * bz + ay * bw + az * bx,
        aw * bz + ax * by - ay * bx + az * bw,
        aw * bw - ax * bx - ay * by - az * bz,
    )


def quat_conjugate(q: Quat) -> Quat:
    x, y, z, w = q
    return (-x, -y, -z, w)


def quat_rotate_vec(q: Quat, v: Vec3) -> Vec3:
    x, y, z, w = q
    vx, vy, vz = v
    # v' = q * (vx,vy,vz,0) * conj(q), expanded
    uvx = y * vz - z * vy
    uvy = z * vx - x * vz
    uvz = x * vy - y * vx
    uuvx = y * uvz - z * uvy
    uuvy = z * uvx - x * uvz
    uuvz = x * uvy - y * uvx
    return (
        vx + 2.0 * (w * uvx + uuvx),
        vy + 2.0 * (w * uvy + uuvy),
        vz + 2.0 * (w * uvz + uuvz),
    )


def vec3_add(a: Vec3, b: Vec3) -> Vec3:
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def vec3_sub(a: Vec3, b: Vec3) -> Vec3:
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def vec3_mul(a: Vec3, b: Vec3) -> Vec3:
    return (a[0] * b[0], a[1] * b[1], a[2] * b[2])


def vec3_div(a: Vec3, b: Vec3) -> Vec3:
    return tuple(av / bv if bv != 0.0 else 0.0 for av, bv in zip(a, b))  # type: ignore[return-value]


class Trs:
    __slots__ = ("pos", "rot", "scale")

    def __init__(self, pos: Vec3, rot: Quat, scale: Vec3) -> None:
        self.pos = pos
        self.rot = rot
        self.scale = scale

    def then(self, local: "Trs") -> "Trs":
        """``self`` as parent, ``local`` as a child's local TRS -> child's world TRS."""
        scaled = vec3_mul(self.scale, local.pos)
        rotated = quat_rotate_vec(self.rot, scaled)
        return Trs(
            pos=vec3_add(self.pos, rotated),
            rot=quat_mul(self.rot, local.rot),
            scale=vec3_mul(self.scale, local.scale),
        )

    def local_of(self, world: "Trs") -> "Trs":
        """Inverse of :meth:`then`: given ``world`` (some node's world TRS) and
        ``self`` as the (new) parent's world TRS, return the node's new local TRS."""
        inv_rot = quat_conjugate(self.rot)
        delta = vec3_sub(world.pos, self.pos)
        rotated = quat_rotate_vec(inv_rot, delta)
        return Trs(
            pos=vec3_div(rotated, self.scale),
            rot=quat_mul(inv_rot, world.rot),
            scale=vec3_div(world.scale, self.scale),
        )


IDENTITY = Trs(IDENTITY_POS, IDENTITY_ROT, IDENTITY_SCALE)
