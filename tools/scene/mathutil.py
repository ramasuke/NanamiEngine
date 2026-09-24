"""ワールド変換を求めるための最小限の TRS（平行移動/回転/スケール）演算。

``move_gameobject`` のワールド変換を保つ付け替えでのみ使う。エンジン自身の
:class:`Transform` はロード時に必ず ``worldMatrix_`` を再計算するので、この
不透明な blob には触れず、ローカルの pos/rot/scale だけを扱えばよい。
クォータニオンは素の float の ``(x, y, z, w)`` タプル。

既知の制約（多くのシーングラフエンジンと共通。Unity も警告を出すまではそうだった）:
回転と *非一様* スケールを合成したものを別の回転の下へ付け替えると、一般には
きれいな回転 + 非一様スケールに分解し直せない。ここでのスケールの成分ごとの合成は、
完全に一般的な極分解を試みるのではなく、このエンジン自身の見かけ上の規約
（``Transform::UpdateMatrix`` 参照）に合わせている。
"""

from __future__ import annotations

Vec3 = tuple[float, float, float]
Quat = tuple[float, float, float, float]

IDENTITY_POS: Vec3 = (0.0, 0.0, 0.0)
IDENTITY_ROT: Quat = (0.0, 0.0, 0.0, 1.0)
IDENTITY_SCALE: Vec3 = (1.0, 1.0, 1.0)


def quat_mul(a: Quat, b: Quat) -> Quat:
    """a (*) b - 結果でベクトルを回転すると、b が先、次に a が適用される。"""
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
    # v' = q * (vx,vy,vz,0) * conj(q) を展開したもの
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
        """``self`` を親、``local`` を子のローカル TRS として -> 子のワールド TRS。"""
        scaled = vec3_mul(self.scale, local.pos)
        rotated = quat_rotate_vec(self.rot, scaled)
        return Trs(
            pos=vec3_add(self.pos, rotated),
            rot=quat_mul(self.rot, local.rot),
            scale=vec3_mul(self.scale, local.scale),
        )

    def local_of(self, world: "Trs") -> "Trs":
        """:meth:`then` の逆: ``world``（あるノードのワールド TRS）と
        ``self``（新しい親のワールド TRS）から、そのノードの新しいローカル TRS を返す。"""
        inv_rot = quat_conjugate(self.rot)
        delta = vec3_sub(world.pos, self.pos)
        rotated = quat_rotate_vec(inv_rot, delta)
        return Trs(
            pos=vec3_div(rotated, self.scale),
            rot=quat_mul(inv_rot, world.rot),
            scale=vec3_div(world.scale, self.scale),
        )


IDENTITY = Trs(IDENTITY_POS, IDENTITY_ROT, IDENTITY_SCALE)
