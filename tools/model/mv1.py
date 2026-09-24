"""DxLib の ``.mv1`` モデルファイルへのアクセス。

``tools.model`` が必要とするものだけ: ファイル本体の展開/再圧縮、参照している
テクスチャファイルのパスの抽出、マテリアルテーブルの色の読み取り/書き換え
(``set-emissive``)。``.mv1`` 形式の仕様は手に入らないので、これは構造的な
パーサーでは **ない**。

ファイルのレイアウト (2026-09-13 に配布済みの実アセットで経験的に確認 -
``Assets/Art/Models/Monster/Hyenas/Hyenas_A4_AllMotion.mv1``、
``Assets/Art/Models/Fantasy/DirtyHouse/dirtyHouse.mv1``、
``Assets/Art/Models/Basic/Cube.mv1`` - どれも宣言どおりのサイズに正確に
デコードできた)::

    "MV11"                      4 バイトのマジック
    <dst_size u32 LE>           展開後の本体サイズ
    <src_size u32 LE>           圧縮サイズ。dst_size から数える (つまり 9 + ペイロード)
    <keycode u8>                下の LZ ストリームのエスケープバイト
    <payload>                   DxLib の DXArchive LZ ストリーム

デコードした本体の中では、テクスチャ参照は **.mv1 からの相対** パスを持つ
NUL 終端のバイト文字列として保存されている (例:
``textures\\Wall_base.png``、``Hyenas_A4_AllMotion_DxLib.fbm\\Hyenas_A4_Normal.png``)
- テクスチャごとに 1 つなので、ディフューズ/法線/ラフネスマップはすべて変換後も残る。
``Assets/`` 以下の全 132 個の ``.mv1`` で非 ASCII のものは UTF-8
(例: ``道のテクスチャ\\road block.png``) だが、cp932 もフォールバックとして残している。
元の FBX の絶対パスを相対パスと並べて持つモデルもある
(``C:\\Tarisland - Dragon\\X.png`` + ``X.png``)。

マテリアルテーブル (2026-09-19 に経験的に確認: ``Assets/`` 以下でマテリアルを持つ
99 個の ``.mv1`` ファイル - うち 53 個は複数 - のすべてで、レコード ``i`` が
インデックス ``i`` と、NUL 区切りの文字列に解決できる名前を持っていた。例:
``dirtyHouse.mv1`` = Wall / wood_1 / Roof)::

    body+0x38  u32   マテリアル数 (アニメーションのみの .mv1 では 0)
    body+0x3c  u32   最初のレコードのオフセット。レコードは 0x2d8 バイト間隔
    body+0xac  u32   文字列テーブルの先頭 + 4 (マテリアル名はここからの相対)

    record+0x04  u32       文字列テーブル内の名前のオフセット
    record+0x08  u32       インデックス
    record+0x0c  COLOR_F   diffuse   (4 x f32, RGBA)
    record+0x1c  COLOR_F   ambient
    record+0x2c  COLOR_F   specular
    record+0x3c  COLOR_F   emissive  (DxLibModelViewer の「自己発光」)
    record+0x4c  f32       power

メッシュテーブル (2026-09-23 に、同じモデルを DxLib の ``MV1SaveModelToMV1File`` で
``MV1SetMeshBackCulling`` のオン/オフで保存して経験的に確認 - 変わったバイトは
メッシュごとに 1 つだけだった - さらに ``Assets/Art/Models/Fantasy`` 以下の全
``.mv1`` と照合)::

    body+0x48  u32   メッシュ数
    body+0x4c  u32   最初のレコードのオフセット。レコードは 0x6c バイト間隔

    record+0x35  u8   背面カリング (DX_CULLING_NONE 0 / LEFT 1 / RIGHT 2)

DxLib が書く LZ ストリーム (配布済みの 40 ファイルで確認) は、最大 8195 バイトの
一致、重なる一致 (距離 < 長さ)、1-3 バイトの距離 (インデックスサイズ 0-2、3 は無い)
を使う。``encode`` はそれらの形だけを出力する。
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
_MAX_MATCH = _MIN_MATCH + (0x1F | 0xFF << 5)  # コードバイト中の 5 ビット + 拡張バイト 1 つ
_MAX_DISTANCE = 1 << 24                          # 3 バイトのインデックス
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

# 配布済みの実アセットがすでに使っている「textures/」兄弟フォルダの規則に合わせる
# (例: Assets/Art/Models/Fantasy/DirtyHouse/textures/)。
TEXTURE_EXTS = {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".dds"}

_TEXTURE_STRING_RE = re.compile(
    rb"[\x20-\x7e\x80-\xfc]{1,259}\.(?:"
    + b"|".join(re.escape(ext[1:].encode()) for ext in sorted(TEXTURE_EXTS))
    + rb")(?=\x00)",
    re.IGNORECASE,
)


def decode(data: bytes) -> bytes:
    """``.mv1`` ファイル全体のバイト列 (マジック込み) を本体に展開する。

    マジックが違う、ストリームが途中で切れている、デコード後のサイズがヘッダの
    宣言と一致しない、のいずれかなら ``ValueError`` を投げる。"""
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
    # 重なる一致でも入力と比べてよい: デコーダは入力と同じになった出力から
    # 1 バイトずつコピーするので。
    length = 0
    while length + 32 <= limit and data[earlier + length:earlier + length + 32] == data[pos + length:pos + length + 32]:
        length += 32
    while length < limit and data[earlier + length] == data[pos + length]:
        length += 1
    return length


def encode(body: bytes) -> bytes:
    """``body`` を圧縮して、:func:`decode` - と DxLib - が ``body`` として読み戻せる
    ``.mv1`` ファイル全体 (マジック込み) にする。4 バイトのハッシュチェーンによる
    貪欲な LZ。出力は DxLib 自身のものとバイト単位では一致しない。"""
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
    """``path`` が参照するテクスチャファイルのパス。初出順、重複除去、保存されている
    とおり (.mv1 からの相対、バックスラッシュ)。

    ヒューリスティック: マテリアルテーブルをたどるのではなく、デコードした本体から
    既知の画像拡張子 (``TEXTURE_EXTS``) で終わる NUL 終端文字列を探すので、
    たまたま ``.png`` で終わる無関係な文字列も報告される。"""
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
    offset: int  # レコードの、デコードした本体内での位置


def materials(body: bytes) -> list[Material]:
    """デコードした本体のマテリアルテーブル (モジュール docstring 参照)。

    テーブルが上記のレイアウトに見えなければ ``ValueError`` を投げる -
    呼び出し側はこれに失敗するファイルを書き換えてはいけない。"""
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
    """``colors`` にある各マテリアルインデックスの自己発光 RGB を置き換えた ``body``。
    アルファや他のバイトはすべてそのまま。"""
    table = materials(body)
    patched = bytearray(body)
    for index, rgb in colors.items():
        if not 0 <= index < len(table):
            raise ValueError(f"material index {index} out of range (model has {len(table)})")
        _RGB.pack_into(patched, table[index].offset + _MAT_EMISSIVE, *rgb)
    return bytes(patched)


def mesh_culling(body: bytes) -> list[int]:
    """全メッシュの背面カリングモード (モジュール docstring 参照)。"""
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
    """全メッシュの背面カリングを ``mode`` にした ``body``。"""
    count = len(mesh_culling(body))
    table = _U32.unpack_from(body, _MESH_TABLE)[0]
    patched = bytearray(body)
    for i in range(count):
        patched[table + i * _MESH_STRIDE + _MESH_CULLING] = mode
    return bytes(patched)
