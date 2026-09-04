"""Assign non-overlapping editor-canvas positions to a tree.

The engine's graph node is 120x60 (``NODE_SIZE`` in ``Npc_BehaviourNodeBase.cpp``).
Layout mirrors how the trees are drawn by hand in the editor:

* **Selector / RandomSelector** children fan out **horizontally** (siblings side by
  side), and the parent is centred above them.
* **Sequence / OnceExecute / OnceSuccess** children stack **vertically** straight
  down from the parent, one per row.

Each subtree reports its own footprint (width, height) so a wide branch pushes the
next horizontal sibling right, and a tall branch pushes the next stacked sibling
down. Deterministic, so add-node + remove-node still round-trips.
"""

from __future__ import annotations

from . import model

DX = 190.0   # horizontal footprint of one leaf column   (node is 120 wide)
DY = 90.0    # vertical step: parent -> child, and row-to-row inside a sequence
X0 = 120.0
Y0 = 40.0

_HORIZONTAL = (model.Selector, model.RandomSelector)


def auto_layout(tree: model.Tree, *, dx: float = DX, dy: float = DY,
                x0: float = X0, y0: float = Y0) -> None:
    """Reposition every node: selectors branch across, sequences stack down."""
    col_w, row_h = float(dx), float(dy)

    def place(node, x: float, y: float) -> tuple[float, float]:
        """Place ``node`` at (x, y); return the (width, height) its subtree spans."""
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

        # vertical stack (Sequence / OnceExecute / OnceSuccess)
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
