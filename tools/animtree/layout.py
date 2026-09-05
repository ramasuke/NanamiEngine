"""Default editor-canvas placement for a freshly-added node.

Unlike ``tools.bt``'s recursive tree auto-layout (which fans a Selector's
children out horizontally / stacks a Sequence's children vertically), an
AnimationTree is a general directed graph - arbitrary node-to-node
transitions, no parent/child relation, potentially cyclic via any-state edges
- so there is no analogous structure to recurse over. Node position is purely
cosmetic (``IAnimationNode::Position()`` only feeds ``OnDrawGraphEditorGui``
rendering - zero gameplay effect), so a real graph-layout algorithm
(force-directed / Sugiyama) is not worth the implementation cost here. This
ships one trivial deterministic grid placement, used only as ``add-clip-node``'s
default when ``--pos`` is omitted; there is no ``layout`` CLI verb that
re-arranges an *existing* tree - use ``move-node --pos`` for manual control.
"""

from __future__ import annotations

DX = 140.0
DY = 90.0
COLS = 6
X0 = 400.0
Y0 = 40.0


def grid_position(index: int, *, x0: float = X0, y0: float = Y0,
                  dx: float = DX, dy: float = DY, cols: int = COLS) -> tuple[float, float]:
    return (x0 + (index % cols) * dx, y0 + (index // cols) * dy)
