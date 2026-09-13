"""Read-only access to DxLib ``.mv1`` model files.

Only what ``tools.model`` needs: decompress the file body and pull out the
texture file paths it references. There is no ``.mv1`` format spec available,
so this is **not** a structural parser.

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
"""

from __future__ import annotations

import re
import struct
from pathlib import Path

MAGIC = b"MV11"
_HEADER = struct.Struct("<IIB")
_MIN_MATCH = 4

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
