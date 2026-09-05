"""Re-export shim: the tagged-blob representation lives in
:mod:`tools.common.blob` (format-generic, not AnimationTree-specific).
"""

from __future__ import annotations

from tools.common.blob import Obj, Ptr, Ver, fingerprint

__all__ = ["Obj", "Ptr", "Ver", "fingerprint"]
