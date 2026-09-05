"""Re-export shim: the vcxproj/.filters text-splice editor moved to
:mod:`tools.common.vcxproj` (format-generic, not BehaviourTree-specific). Only
``CONTENT_ANCHOR`` - the BT Action content-directory anchor - stays here; callers
in this package pass it explicitly to ``apply_splices``/``remove_splices``.
"""

from __future__ import annotations

from tools.common.vcxproj import Splice, VcxprojError, apply_splices, remove_splices

CONTENT_ANCHOR = r"Assets\Scripts\Core\Game\Npc\Enemy\Behaviour\Action\Content"

__all__ = ["CONTENT_ANCHOR", "Splice", "VcxprojError", "apply_splices", "remove_splices"]
