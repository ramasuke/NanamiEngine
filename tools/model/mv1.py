"""Access to DxLib ``.mv1`` model files.

Only what ``tools.model`` needs: decompress/recompress the file body, pull out
the texture file paths it references, and read/patch the material table's
colors (``set-emissive``). There is no ``.mv1`` format spec available, so this
is **not** a structural parser.

File layout (empirically confirmed 2026-09-13 against real shipped assets -
``Assets/Art/Models/Monster/Hyenas/Hyenas_A4_AllMotion.mv1``,
``Assets/Art/Models/Fantasy/DirtyHouse/dirtyHouse.mv1``,
``Assets/Art/Models/Basic/Cube.mv1`` - each decoded to exactly its declared
size)::

    "MV11"                      4-byte magic
    <dst_size u32 LE>           decompressed body size
    <src_size u32 LE>           compressed size, counted from dst_size (so 9 + payload)
    <keycode u8>                escape byte of the LZ stream below
    <payload>                   DxLib's DXArchive LZ stream

Inside the decoded body, texture references are stored as plain NUL-terminated
byte strings holding the path **relative to the .mv1** (e.g.
``textures\\Wall_base.png``, ``Hyenas_A4_AllMotion_DxLib.fbm\\Hyenas_A4_Normal.png``)
- one per texture, so diffuse/normal/roughness maps all survive conversion.
Across all 132 ``.mv1`` files under ``Assets/`` the non-ASCII ones are UTF-8
(e.g. ``道のテクスチャ\\road block.png``), but cp932 is kept as a fallback.
Some models also carry the source FBX's absolute path next to the relative
one (``C:\\Tarisland - Dragon\\X.png`` + ``X.png``).

Material table (empirically confirmed 2026-09-19: every one of the 99 ``.mv1``
files under ``Assets/`` that has materials - 53 of them with several - has
record ``i`` holding index ``i`` and a name that resolves to a NUL-delimited
string, e.g. ``dirtyHouse.mv1`` = Wall / wood_1 / Roof)::

    body+0x38  u32   material count (0 in an animation-only .mv1)
    body+0x3c  u32   offset of the first record; records are 0x2d8 bytes apart
    body+0xac  u32   string table start + 4 (material names are relative to it)

    record+0x04  u32       name offset into the string table
    record+0x08  u32       index
    record+0x0c  COLOR_F   diffuse   (4 x f32, RGBA)
    record+0x1c  COLOR_F   ambient
    record+0x2c  COLOR_F   specular
    record+0x3c  COLOR_F   emissive  (DxLibModelViewer's "自己発光")
    record+0x4c  f32       power

Mesh table (empirically confirmed 2026-09-23 by saving the same model through
DxLib's ``MV1SaveModelToMV1File`` with ``MV1SetMeshBackCulling`` on and off -
the only bytes that changed were one per mesh - and checked against every
``.mv1`` under ``Assets/Art/Models/Fantasy``)::

    body+0x48  u32   mesh count
    body+0x4c  u32   offset of the first record; records are 0x6c bytes apart

    record+0x35  u8   back-face culling (DX_CULLING_NONE 0 / LEFT 1 / RIGHT 2)

The LZ stream DxLib writes (checked on 40 shipped files) uses matches up to
8195 bytes long, overlapping matches (distance < length), and 1-3 byte
distances (index size 0-2, never 3); ``encode`` emits only those forms.
"""

from __future__ import annotations

import math
import re
import struct
from dataclasses import dataclass
from pathlib import Path

MAGIC = b"MV11"
_HEADER = struct.Struct("<IIB")
_MIN_MATCH = 4
_MAX_MATCH = _MIN_MATCH + (0x1F | 0xFF << 5)  # 5 bits in the code byte + one extension byte
_MAX_DISTANCE = 1 << 24                          # 3-byte index
_HASH_CHAIN_DEPTH = 16

_MATERIAL_COUNT = 0x38
_MATERIAL_TABLE = 0x3C
_STRING_TABLE = 0xAC
_MATERIAL_STRIDE = 0x2D8
_MAT_NAME = 0x04
_MAT_INDEX = 0x08
_MAT_DIFFUSE = 0x0C
_MAT_AMBIENT = 0x1C
_MAT_SPECULAR = 0x2C
_MAT_EMISSIVE = 0x3C
_MAT_POWER = 0x4C
_MESH_COUNT = 0x48
_MESH_TABLE = 0x4C
_MESH_STRIDE = 0x6C
_MESH_CULLING = 0x35
CULLING_MODES = {"none": 0, "left": 1, "right": 2}
_COLOR = struct.Struct("<4f")
_RGB = struct.Struct("<3f")
_U32 = struct.Struct("<I")
_F32 = struct.Struct("<f")

# Matches the "textures/" sibling-folder convention already used by real
# shipped assets (e.g. Assets/Art/Models/Fantasy/DirtyHouse/textures/).
TEXTURE_EXTS = {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".dds"}

_TEXTURE_STRING_RE = re.compile(
    rb"[\x20-\x7e\x80-\xfc]{1,259}\.(?:"
    + b"|".join(re.escape(ext[1:].encode()) for ext in sorted(TEXTURE_EXTS))
    + rb")(?=\x00)",
    re.IGNORECASE,
)


def decode(data: bytes) -> bytes:
    """Decompress a whole ``.mv1`` file's bytes (magic included) into its body.

    Raises ``ValueError`` if the magic is wrong, the stream is truncated, or
    the decoded size doesn't match the header's declared size."""
    if data[:4] != MAGIC:
        raise ValueError(f"header is {data[:4]!r}, expected {MAGIC!r}")
    body = data[4:]
    if len(body) < _HEADER.size:
        raise ValueError("file too short for a compression header")
    dst_size, src_size, key = _HEADER.unpack_from(body, 0)
    if src_size > len(body):
        raise ValueError(f"declared compressed size {src_size} exceeds the file ({len(body)} bytes after magic)")

    out = bytearray()
    sp = _HEADER.size
    try:
        while sp < src_size:
            c = body[sp]
            if c != key:
                out.append(c)
                sp += 1
                continue
            if body[sp + 1] == key:
                out.append(key)
                sp += 2
                continue
            code = body[sp + 1]
            if code > key:
                code -= 1
            sp += 2
            length = code >> 3
            if code & 0x4:
                length |= body[sp] << 5
                sp += 1
            length += _MIN_MATCH
            index_size = code & 0x3
            if index_size == 0:
                index = body[sp]
                sp += 1
            elif index_size == 1:
                index = body[sp] | body[sp + 1] << 8
                sp += 2
            else:
                index = body[sp] | body[sp + 1] << 8 | body[sp + 2] << 16
                sp += 3
            start = len(out) - (index + 1)
            if start < 0:
                raise ValueError(f"back-reference before start of output at compressed offset {sp}")
            for i in range(length):
                out.append(out[start + i])
    except IndexError as e:
        raise ValueError("compressed stream is truncated") from e

    if len(out) != dst_size:
        raise ValueError(f"decoded {len(out)} byte(s), header declares {dst_size}")
    return bytes(out)


def _match_length(data: bytes, earlier: int, pos: int, limit: int) -> int:
    # Comparing against the input is valid for overlapping matches too: the
    # decoder copies byte by byte from output that already equals the input.
    length = 0
    while length + 32 <= limit and data[earlier + length:earlier + length + 32] == data[pos + length:pos + length + 32]:
        length += 32
    while length < limit and data[earlier + length] == data[pos + length]:
        length += 1
    return length


def encode(body: bytes) -> bytes:
    """Compress ``body`` into a whole ``.mv1`` file (magic included) that
    :func:`decode` - and DxLib - reads back as ``body``. Greedy LZ over a
    4-byte hash chain; the output is not byte-identical to DxLib's own."""
    key = min(range(256), key=lambda v: body.count(bytes((v,))))
    n = len(body)
    out = bytearray()
    head: dict[bytes, int] = {}
    chain = [-1] * n

    def insert(p: int) -> None:
        if p + _MIN_MATCH <= n:
            h = body[p:p + _MIN_MATCH]
            chain[p] = head.get(h, -1)
            head[h] = p

    pos = 0
    while pos < n:
        best_len = 0
        best_dist = 0
        limit = min(_MAX_MATCH, n - pos)
        if limit >= _MIN_MATCH:
            candidate = head.get(body[pos:pos + _MIN_MATCH], -1)
            depth = 0
            while candidate >= 0 and depth < _HASH_CHAIN_DEPTH and pos - candidate <= _MAX_DISTANCE:
                length = _match_length(body, candidate, pos, limit)
                if length > best_len:
                    best_len = length
                    best_dist = pos - candidate
                    if length == limit:
                        break
                candidate = chain[candidate]
                depth += 1

        if best_len < _MIN_MATCH:
            c = body[pos]
            out.append(c)
            if c == key:
                out.append(key)
            insert(pos)
            pos += 1
            continue

        stored_len = best_len - _MIN_MATCH
        index = best_dist - 1
        code = (stored_len & 0x1F) << 3
        extension = stored_len >> 5
        if extension:
            code |= 0x4
        if index <= 0xFF:
            index_bytes = index.to_bytes(1, "little")
        elif index <= 0xFFFF:
            code |= 0x1
            index_bytes = index.to_bytes(2, "little")
        else:
            code |= 0x2
            index_bytes = index.to_bytes(3, "little")
        out.append(key)
        out.append(code + 1 if code >= key else code)
        if extension:
            out.append(extension)
        out += index_bytes
        for p in range(pos, pos + best_len):
            insert(p)
        pos += best_len

    return MAGIC + _HEADER.pack(n, _HEADER.size + len(out), key) + bytes(out)


def _decode_string(raw: bytes) -> str:
    try:
        return raw.decode("utf-8")
    except UnicodeDecodeError:
        return raw.decode("cp932", errors="replace")


def texture_paths(path: Path) -> list[str]:
    """Texture file paths referenced by ``path``, in order of first appearance,
    duplicates removed, exactly as stored (relative to the .mv1, backslashes).

    Heuristic: scans the decoded body for NUL-terminated strings ending in a
    known image extension (``TEXTURE_EXTS``) rather than walking the material
    table, so an unrelated string that happens to end in ``.png`` would also
    be reported."""
    body = decode(path.read_bytes())
    found: list[str] = []
    for m in _TEXTURE_STRING_RE.finditer(body):
        text = _decode_string(m.group(0))
        if text not in found:
            found.append(text)
    return found


Color = tuple[float, float, float, float]


@dataclass(frozen=True)
class Material:
    index: int
    name: str
    diffuse: Color
    ambient: Color
    specular: Color
    emissive: Color
    power: float
    offset: int  # of the record, within the decoded body


def materials(body: bytes) -> list[Material]:
    """The material table of a decoded body (see the module docstring).

    Raises ``ValueError`` when the table doesn't look like the layout above -
    callers must not patch a file that fails this."""
    if len(body) < _STRING_TABLE + _U32.size:
        raise ValueError(f"body is only {len(body)} byte(s), too short for the model header")
    count = _U32.unpack_from(body, _MATERIAL_COUNT)[0]
    table = _U32.unpack_from(body, _MATERIAL_TABLE)[0]
    if count == 0:
        return []
    if table + count * _MATERIAL_STRIDE > len(body):
        raise ValueError(f"{count} material record(s) at 0x{table:x} run past the end of the body")
    strings = _U32.unpack_from(body, _STRING_TABLE)[0] - 4
    if not 0 <= strings < len(body):
        raise ValueError(f"string table offset 0x{strings:x} is outside the body")

    result: list[Material] = []
    for i in range(count):
        rec = table + i * _MATERIAL_STRIDE
        index = _U32.unpack_from(body, rec + _MAT_INDEX)[0]
        if index != i:
            raise ValueError(f"material record {i} at 0x{rec:x} holds index {index}")
        name_at = strings + _U32.unpack_from(body, rec + _MAT_NAME)[0]
        name_end = body.find(b"\x00", name_at) if name_at < len(body) else -1
        if name_end <= name_at or (name_at > 0 and body[name_at - 1] != 0):
            raise ValueError(f"material {i}'s name offset does not point at a string")
        colors = [_COLOR.unpack_from(body, rec + off)
                  for off in (_MAT_DIFFUSE, _MAT_AMBIENT, _MAT_SPECULAR, _MAT_EMISSIVE)]
        power = _F32.unpack_from(body, rec + _MAT_POWER)[0]
        if not all(math.isfinite(v) for v in (*colors[0], *colors[1], *colors[2], *colors[3], power)):
            raise ValueError(f"material {i} has non-finite colors")
        result.append(Material(i, _decode_string(body[name_at:name_end]), *colors, power, rec))
    return result


def with_emissive(body: bytes, colors: dict[int, tuple[float, float, float]]) -> bytes:
    """``body`` with the emissive RGB of each material index in ``colors``
    replaced; alpha and every other byte are left as they were."""
    table = materials(body)
    patched = bytearray(body)
    for index, rgb in colors.items():
        if not 0 <= index < len(table):
            raise ValueError(f"material index {index} out of range (model has {len(table)})")
        _RGB.pack_into(patched, table[index].offset + _MAT_EMISSIVE, *rgb)
    return bytes(patched)


def mesh_culling(body: bytes) -> list[int]:
    """Back-face culling mode of every mesh (see the module docstring)."""
    if len(body) < _MESH_TABLE + _U32.size:
        raise ValueError(f"body is only {len(body)} byte(s), too short for the model header")
    count = _U32.unpack_from(body, _MESH_COUNT)[0]
    table = _U32.unpack_from(body, _MESH_TABLE)[0]
    if count and table + count * _MESH_STRIDE > len(body):
        raise ValueError(f"{count} mesh record(s) at 0x{table:x} run past the end of the body")
    modes = [body[table + i * _MESH_STRIDE + _MESH_CULLING] for i in range(count)]
    if any(m not in CULLING_MODES.values() for m in modes):
        raise ValueError(f"mesh culling bytes {modes} are not all culling modes; the mesh table layout does not match")
    return modes


def with_mesh_culling(body: bytes, mode: int) -> bytes:
    """``body`` with every mesh's back-face culling set to ``mode``."""
    count = len(mesh_culling(body))
    table = _U32.unpack_from(body, _MESH_TABLE)[0]
    patched = bytearray(body)
    for i in range(count):
        patched[table + i * _MESH_STRIDE + _MESH_CULLING] = mode
    return bytes(patched)
