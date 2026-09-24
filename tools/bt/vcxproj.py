"""再エクスポート用シム: vcxproj/.filters のテキスト差し込みエディタは
:mod:`tools.common.vcxproj` に移動した（BehaviourTree 固有ではなく形式非依存）。
BT Action のコンテンツディレクトリの目印である ``CONTENT_ANCHOR`` だけはここに残り、
このパッケージの呼び出し側がそれを ``apply_splices``/``remove_splices`` に明示的に渡す。
"""

from __future__ import annotations

from tools.common.vcxproj import Splice, VcxprojError, apply_splices, remove_splices

CONTENT_ANCHOR = r"Assets\Scripts\Core\Game\Npc\Enemy\Behaviour\Action\Content"

__all__ = ["CONTENT_ANCHOR", "Splice", "VcxprojError", "apply_splices", "remove_splices"]
