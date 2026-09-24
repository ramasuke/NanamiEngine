"""型付きプロパティを持つノードツリーをそのまま読み書きできる、最小限のバイナリ FBX (7.x) リーダー/ライター。

tools/art/mixamo_clip.py が Mixamo のキャラ FBX のアニメーションカーブを書き換えるのに使う。
配列は保存時に圧縮し直すので、バイト列は入力と変わるがツリーは同一になる。
"""
import struct, zlib

class Node:
    __slots__ = ('name', 'props', 'children', 'block')

    def __init__(self, name, props=None, children=None, block=False):
        self.name = name
        self.props = props if props is not None else []   # (type_char, value) のリスト
        self.children = children if children is not None else []
        # 空の `{ }` ブロック (Mixamo の AnimationLayer など)。これが無いと DxLib はアニメーションを捨てる
        self.block = block

    def find(self, n):
        return [c for c in self.children if c.name == n]

    def first(self, n):
        for c in self.children:
            if c.name == n:
                return c
        return None

    def v(self, i):
        return self.props[i][1]


_ARR = {'f': 'f', 'd': 'd', 'l': 'q', 'i': 'i', 'b': '?'}


def _read_props(d, o, n):
    out = []
    for _ in range(n):
        t = chr(d[o]); o += 1
        if t == 'Y': v = struct.unpack_from('<h', d, o)[0]; o += 2
        elif t == 'C': v = d[o]; o += 1
        elif t == 'I': v = struct.unpack_from('<i', d, o)[0]; o += 4
        elif t == 'F': v = struct.unpack_from('<f', d, o)[0]; o += 4
        elif t == 'D': v = struct.unpack_from('<d', d, o)[0]; o += 8
        elif t == 'L': v = struct.unpack_from('<q', d, o)[0]; o += 8
        elif t in _ARR:
            ln, enc, cl = struct.unpack_from('<III', d, o); o += 12
            raw = d[o:o + cl]; o += cl
            if enc == 1:
                raw = zlib.decompress(raw)
            v = list(struct.unpack('<%d%s' % (ln, _ARR[t]), raw))
        elif t in 'SR':
            ln = struct.unpack_from('<I', d, o)[0]; o += 4
            v = bytes(d[o:o + ln]); o += ln
        else:
            raise ValueError('bad prop type %r at %d' % (t, o))
        out.append((t, v))
    return out, o


def _read_node(d, o, v64):
    if v64:
        end, np_, pl = struct.unpack_from('<QQQ', d, o); o += 24
    else:
        end, np_, pl = struct.unpack_from('<III', d, o); o += 12
    nl = d[o]; o += 1
    if end == 0:
        return None, o
    name = d[o:o + nl].decode('ascii'); o += nl
    props, _ = _read_props(d, o, np_)
    o += pl
    children = []
    block = False
    sentinel = 25 if v64 else 13
    while o < end:
        if end - o == sentinel:
            o = end
            block = True
            break
        c, o = _read_node(d, o, v64)
        if c is None:
            block = True
            break
        children.append(c)
    return Node(name, props, children, block), end


class Document:
    def __init__(self, header, version, top, footer_id):
        self.header = header
        self.version = version
        self.top = top
        self.footer_id = footer_id

    def first(self, n):
        for c in self.top:
            if c.name == n:
                return c
        return None


def load(path):
    d = open(path, 'rb').read()
    version = struct.unpack_from('<I', d, 23)[0]
    v64 = version >= 7500
    o = 27
    top = []
    while True:
        start = o
        c, o = _read_node(d, o, v64)
        if c is None:
            null_end = start + (25 if v64 else 13)
            break
        top.append(c)
    return Document(d[:27], version, top, d[null_end:null_end + 16])


def _write_props(props):
    out = bytearray()
    for t, v in props:
        out += t.encode()
        if t == 'Y': out += struct.pack('<h', v)
        elif t == 'C': out += struct.pack('<B', v)
        elif t == 'I': out += struct.pack('<i', v)
        elif t == 'F': out += struct.pack('<f', v)
        elif t == 'D': out += struct.pack('<d', v)
        elif t == 'L': out += struct.pack('<q', v)
        elif t in _ARR:
            raw = struct.pack('<%d%s' % (len(v), _ARR[t]), *v)
            comp = zlib.compress(raw) if len(raw) > 128 else None
            if comp is not None:
                out += struct.pack('<III', len(v), 1, len(comp)) + comp
            else:
                out += struct.pack('<III', len(v), 0, len(raw)) + raw
        elif t in 'SR':
            out += struct.pack('<I', len(v)) + v
        else:
            raise ValueError(t)
    return bytes(out)


def _write_node(node, offset, v64):
    head = 24 if v64 else 12
    name = node.name.encode('ascii')
    props = _write_props(node.props)
    body = bytearray()
    pos = offset + head + 1 + len(name) + len(props)
    for c in node.children:
        b = _write_node(c, pos, v64)
        body += b
        pos += len(b)
    if node.children or not node.props or node.block:
        # null レコードで子リストを終える (プロパティも子も無いノードでも書く)
        body += b'\0' * (head + 1)
        pos += head + 1
    end = pos
    fmt = '<QQQ' if v64 else '<III'
    return struct.pack(fmt, end, len(node.props), len(props)) + bytes([len(name)]) + name + props + bytes(body)


def save(doc, path):
    v64 = doc.version >= 7500
    out = bytearray(doc.header)
    for n in doc.top:
        out += _write_node(n, len(out), v64)
    out += b'\0' * (25 if v64 else 13)
    out += doc.footer_id
    out += b'\0' * 4
    pad = ((len(out) + 15) & ~15) - len(out)
    if pad == 0:
        pad = 16
    out += b'\0' * pad
    out += struct.pack('<I', doc.version)
    out += b'\0' * 120
    out += b'\xf8\x5a\x8c\x6a\xde\xf5\xd9\x7e\xec\xe9\x0c\xe3\x75\x8f\x29\x0b'
    open(path, 'wb').write(bytes(out))
