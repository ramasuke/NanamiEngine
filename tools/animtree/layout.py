"""新規追加したノードのエディタキャンバス上の既定配置。

``tools.bt`` の再帰的なツリー自動レイアウト（Selector の子を横に広げ、
Sequence の子を縦に積む）と違い、AnimationTree は一般の有向グラフ
（ノード間の任意の遷移、親子関係なし、any-state の辺で循環もありうる）なので、
再帰でたどれる同様の構造が無い。ノード位置は見た目だけのもの
（``IAnimationNode::Position()`` は ``OnDrawGraphEditorGui`` の描画にしか
使われずゲームプレイに影響しない）なので、本格的なグラフレイアウト
（力学モデル / Sugiyama）は実装コストに見合わない。ここでは単純で決定的な
グリッド配置を 1 つだけ用意し、``add-clip-node`` で ``--pos`` を省略したときの
既定値にのみ使う。*既存の* ツリーを並べ直す ``layout`` コマンドは無いので、
手動で調整するには ``move-node --pos`` を使う。
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
