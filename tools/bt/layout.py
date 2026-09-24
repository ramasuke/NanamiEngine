"""ツリーに重ならないエディタキャンバス上の位置を割り当てる。

エンジンのグラフノードは 120x60（``Npc_BehaviourNodeBase.cpp`` の ``NODE_SIZE``）。
レイアウトはエディタで手作業で描かれるツリーの形に合わせる:

* **Selector / RandomSelector** の子は**横に**広がり（兄弟を横並び）、
  親はその上の中央に置く。
* **Sequence / OnceExecute / OnceSuccess** の子は親から真下へ**縦に**
  1行に1つずつ積む。

各サブツリーは自身の占有範囲（幅, 高さ）を返すので、幅の広い分岐は次の横並びの
兄弟を右へ、背の高い分岐は次の縦積みの兄弟を下へ押し出す。決定的なので
add-node + remove-node もラウンドトリップする。
"""

from __future__ import annotations

from . import model

DX = 127.0   # 葉1列分の横方向の占有幅   （ノード幅は 120）
DY = 60.0    # 縦方向の間隔: 親 -> 子、およびシーケンス内の行間
X0 = 120.0
Y0 = 40.0

_HORIZONTAL = (model.Selector, model.RandomSelector)


def auto_layout(tree: model.Tree, *, dx: float = DX, dy: float = DY,
                x0: float = X0, y0: float = Y0) -> None:
    """全ノードを配置し直す: セレクタは横に分岐し、シーケンスは下に積む。"""
    col_w, row_h = float(dx), float(dy)

    def place(node, x: float, y: float) -> tuple[float, float]:
        """``node`` を (x, y) に置き、そのサブツリーが占める (width, height) を返す。"""
        kids = model.children_of(node)
        if not kids:
            node.pos = (float(round(x)), float(round(y)))
            return col_w, row_h

        child_y = y + row_h

        if isinstance(node, _HORIZONTAL):
            cx = x
            below = 0.0
            for k in kids:
                w, h = place(k, cx, child_y)
                cx += w
                below = max(below, h)
            centre = (kids[0].pos[0] + kids[-1].pos[0]) / 2.0
            node.pos = (float(round(centre)), float(round(y)))
            return cx - x, row_h + below

        # 縦積み（Sequence / OnceExecute / OnceSuccess）
        node.pos = (float(round(x)), float(round(y)))
        cy = child_y
        width = col_w
        for k in kids:
            w, h = place(k, x, cy)
            cy += h
            width = max(width, w)
        return width, cy - y

    if tree.entry.child is None:
        tree.entry.pos = (float(round(x0)), float(round(y0)))
        return
    place(tree.entry.child, x0, y0 + row_h)
    tree.entry.pos = (tree.entry.child.pos[0], float(round(y0)))
