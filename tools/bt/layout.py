"""ツリーに重ならないエディタキャンバス上の位置を割り当てる。

エディタの「整列」ボタン（``BehaviourTreeGraphDelegate::AutoLayout``）と同じ左→右のツリー:

* 子は親の右（``親の x + 親の幅 + GAP_X``）に置き、兄弟は上から実行順に縦に並べる。
* 親は最初の子と同じ y（上揃え）。各サブツリーは自身の高さぶん次の兄弟を下へ押し出す。
* 浮きノード（``tree.detached``）は Entry のツリーの下にまとめる。
* 子が全部アクションの Sequence はエディタで畳まれてリスト表示になるので、その子は親の右に
  置くだけで縦の場所は取らない。

ノードの大きさはエディタと同じ式で見積もる（``GraphDelegateBase::MeasureNodeSize``）。
フォントは 13px の MS ゴシック（等幅: 半角 6.5px / 全角 13px）。決定的なので
add-node + remove-node もラウンドトリップする。
"""

from __future__ import annotations

import unicodedata

from . import model

GAP_X = 40.0   # 親の右端 -> 子の左端
GAP_Y = 16.0   # 兄弟のサブツリー同士の縦の隙間
X0 = 120.0
Y0 = 40.0

# 旧 CLI オプション（--dx / --dy）の既定値
DX = GAP_X
DY = GAP_Y

_FONT_SIZE = 13.0
_DETAIL_RATIO = 0.85
_TITLE_PADDING = 6.0
_ROUNDING = 6.0
_HEADER_HEIGHT = 22.0
_MIN_W, _MIN_H = 120.0, 60.0
_BADGE = " #00"

_TITLES = {
    model.Entry: "Entry",
    model.Selector: "Selector",
    model.Sequence: "Sequence",
    model.RandomSelector: "RandomSelector",
    model.OnceExecute: "OnceExecute",
    model.OnceSuccess: "OnceSuccessNode",
    model.BlackBoardGate: "BlackBoardGate",
}


def _text_size(text: str, size: float) -> tuple[float, float]:
    lines = text.split("\n")
    half = size * 0.5
    width = max(sum(size if unicodedata.east_asian_width(c) in "WFA" else half for c in line)
                for line in lines)
    return width, size * len(lines)


def _entries(pairs, op: str) -> str:
    return ", ".join(f"{k}{op}{v}" for k, v in pairs)


def is_list_sequence(node) -> bool:
    """``BehaviourTreeGraphDelegate::IsListSequence``: 子が全部アクションの Sequence。"""
    return (isinstance(node, model.Sequence) and bool(node.children)
            and all(isinstance(k, model.Action) for k in node.children))


def title_and_detail(node) -> tuple[str, str]:
    """エディタのノードの見出しと本文（名前の無いアクションは型名が見出し、リスト表示の Sequence は子の一覧）。"""
    if isinstance(node, model.Action):
        type_name = node.type_name.rsplit("::", 1)[-1] if node.type_fqn else "(no action)"
        return (node.name, type_name) if node.name else (type_name, "")
    if is_list_sequence(node):
        rows = (f"{i + 1}. {title_and_detail(k)[0]}" for i, k in enumerate(node.children))
        return "Sequence", "\n".join(rows)
    detail = ""
    if isinstance(node, model.RandomSelector) and node.weights:
        detail = "weights" + "".join(f" {w}" for w in node.weights)
    elif isinstance(node, model.BlackBoardGate):
        detail = _entries(node.conditions, "==")
        if node.writes_on_start:
            detail += "\nstart: " + _entries(node.writes_on_start, "=")
        if node.writes_on_success:
            detail += "\nok: " + _entries(node.writes_on_success, "=")
        if node.once:
            detail += "\nonce"
    return _TITLES.get(type(node), type(node).__name__), detail


def node_size(node, has_badge: bool) -> tuple[float, float]:
    title, detail = title_and_detail(node)
    detail_size = _FONT_SIZE * _DETAIL_RATIO
    title_w = _text_size(title, _FONT_SIZE)[0] + _TITLE_PADDING * 2
    if has_badge:
        title_w += _text_size(_BADGE, detail_size)[0]
    dw, dh = _text_size(detail, detail_size) if detail else (0.0, 0.0)
    width = max(_MIN_W, title_w, dw + _ROUNDING * 2)
    height = max(_MIN_H, dh + _HEADER_HEIGHT + _ROUNDING * 2)
    return float(-(-width // 1)), float(-(-height // 1))


def auto_layout(tree: model.Tree, *, dx: float = GAP_X, dy: float = GAP_Y,
                x0: float = X0, y0: float = Y0) -> None:
    """全ノードを左→右のツリーに配置し直す。``dx`` = 横の隙間、``dy`` = 縦の隙間。"""
    gap_x, gap_y = float(dx), float(dy)

    def place(node, x: float, y: float, has_badge: bool) -> float:
        """``node`` を (x, y) に置き、次の兄弟を置ける y を返す。"""
        w, h = node_size(node, has_badge)
        node.pos = (float(round(x)), float(round(y)))
        kids = model.children_of(node) if not isinstance(node, model.Entry) else (
            [node.child] if node.child is not None else [])
        next_y = y
        for k in kids:
            next_y = place(k, x + w + gap_x, next_y, len(kids) > 1)
        if is_list_sequence(node):
            return y + h + gap_y
        return max(next_y, y + h + gap_y)

    y = place(tree.entry, x0, y0, False)
    for root in tree.detached:
        y = place(root, x0, y + gap_y * 4, False)
