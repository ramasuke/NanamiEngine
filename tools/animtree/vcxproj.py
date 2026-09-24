"""再エクスポート用シム: vcxproj/.filters のテキスト差し込みエディタは
:mod:`tools.common.vcxproj` にある（形式非依存で AnimationTree 固有ではない）。

v1 の ``tools.animtree`` のコマンドはどれも使っていない。C++ の雛形生成コマンドが
まだ無いため（docs/AnimationTree.md #4 参照: 新しい ``IAnimationNode`` 型の追加は
手作業のみ。「ノード追加」の操作は ``AnimationTree.cpp`` 自体にハードコードされた
ImGui メニューの分岐で、BehaviourTree の Action のような周辺の生成ファイルではない）。
``tools/bt`` / ``tools/scene`` との構造上の対称性のため、将来の雛形生成コマンドに備えて残している。
"""

from __future__ import annotations

from tools.common.vcxproj import Splice, VcxprojError, apply_splices, remove_splices

__all__ = ["Splice", "VcxprojError", "apply_splices", "remove_splices"]
