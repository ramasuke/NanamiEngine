"""再エクスポート用シム: タグ付き blob 表現は
:mod:`tools.common.blob` に移動した（BehaviourTree 固有ではなく形式非依存）。
"""

from __future__ import annotations

from tools.common.blob import Obj, Ptr, Ver, fingerprint

__all__ = ["Obj", "Ptr", "Ver", "fingerprint"]
