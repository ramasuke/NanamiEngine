"""再エクスポート用シム: タグ付き blob 表現は
:mod:`tools.common.blob` にある（形式非依存で AnimationTree 固有ではない）。
"""

from __future__ import annotations

from tools.common.blob import Obj, Ptr, Ver, fingerprint

__all__ = ["Obj", "Ptr", "Ver", "fingerprint"]
