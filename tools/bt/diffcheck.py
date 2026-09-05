"""Re-export shim: structural comparison helpers moved to
:mod:`tools.common.diffcheck` (format-generic, not BehaviourTree-specific).
"""

from __future__ import annotations

from tools.common.diffcheck import (
    SemanticMismatch,
    assert_bookkeeping_equal,
    assert_semantically_equal,
)

__all__ = ["SemanticMismatch", "assert_bookkeeping_equal", "assert_semantically_equal"]
