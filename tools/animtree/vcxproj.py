"""Re-export shim: the vcxproj/.filters text-splice editor lives in
:mod:`tools.common.vcxproj` (format-generic, not AnimationTree-specific).

Not used by any ``tools.animtree`` command in v1 - there is no C++-scaffolding
command yet (see docs/AnimationTree.md #4: adding a new ``IAnimationNode`` type
is by-hand only, since the "add node" affordance is a hardcoded ImGui menu
branch inside ``AnimationTree.cpp`` itself, not a peripheral generated-content
file the way BehaviourTree Actions are). Kept for structural symmetry with
``tools/bt`` and ``tools/scene`` in case a future scaffold command needs it.
"""

from __future__ import annotations

from tools.common.vcxproj import Splice, VcxprojError, apply_splices, remove_splices

__all__ = ["Splice", "VcxprojError", "apply_splices", "remove_splices"]
