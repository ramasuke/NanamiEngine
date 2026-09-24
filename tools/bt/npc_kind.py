"""このツールキットが扱う2つの BehaviourTree「種別」- ``Enemy`` と ``FriendlyNpc`` -
を記述するレジストリ。これにより ``tools/bt`` の他の部分は Enemy のパスを
あちこちにハードコードせず、ファイル/コマンドがどちらを対象にしても汎用に書ける。
エンジン自身の ``BehaviourTreeType`` の区分
（``Editor::BehaviourTreeType::EnemyNpc`` / ``FriendlyNpc``）に対応する。両種別は
同じ複合ノード型（Selector/Sequence/...）、ファイル形式の外枠、``.meta`` の規約を
共有するが、``ActionNode`` の C++ 型、``ActionBase`` 階層、action のファクトリ/登録
マクロ、ディスク上の action ツリーはそれぞれ独自に持つ。

このパッケージ内の「Enemy か Friendly か」に関わる問いは、新たなパスを
ハードコードせず :data:`ENEMY` / :data:`FRIENDLY`（または :func:`by_name`）を
経由すること。そうすれば万一3つ目の種別が現れても、ここに1項目足すだけで済む。
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

from . import model

_REPO = Path(__file__).resolve().parents[2]


@dataclass(frozen=True)
class NpcKind:
    name: str                       # "enemy" | "friendly" - --kind の値
    cpp_namespace_segment: str      # "Enemy" | "Friendly" - GameCore::Npc::<X>::...

    action_node_fqn: str            # Editor::Npc::<X>::Behaviour::ActionNode
    action_fqn_prefix: str          # GameCore::Npc::<X>::Behaviour::Action::
    action_base_fqn: str            # GameCore::Npc::<X>::Behaviour::ActionBase

    asset_fqn: str                  # NanamiEngine::Module::Asset::<X>BehaviourFile
    data_ext: str                   # ".enemyBehaviourData" | ".friendBehaviourData"
    meta_ext: str
    default_dir: str                # Assets/Data/EnemyBehaviour | .../FriendlyNpcBehviour

    action_dir_rel: str             # Assets/Scripts/Core/Game/Npc/<X>/Behaviour/Action
    headers_agg: Path                # .../Editor/Npc/<X>/Behaviour/Action/<X>_Behaviour_ActionHeaders.h
    content_anchor: str              # vcxproj の差し込み位置の目印（Windows 区切り）
    factory_rel_suffix: str          # Editor/Npc/<X>/Behaviour/Action/<X>_Behaviour_ActionFactory.h
    base_include_name: str          # <X>_Behaviour_ActionBase.h
    action_file_prefix: str         # <X>_Behaviour_Action_

    register_macro: str             # REGISTER_ENEMY_ACTION_WITH_NAME | REGISTER_FRIENDLY_ACTION_WITH_NAME
    register_named_re: re.Pattern
    register_bare_re: re.Pattern

    catalog_path: Path
    bind_hint: str                   # new-tree で表示する案内

    @property
    def action_root(self) -> Path:
        return _REPO / self.action_dir_rel

    @property
    def content_root(self) -> Path:
        return self.action_root / "Content"

    @property
    def extra_struct_files(self) -> dict[str, Path]:
        # どちらの種別も埋め込み構造体のヘッダー（例: 単独の Position）を
        # Action/ 直下に置き、互いに 1:1 で対応している。
        return {
            "Position": self.action_root / f"Position/{self.action_file_prefix}Position.h",
        }


ENEMY = NpcKind(
    name="enemy",
    cpp_namespace_segment="Enemy",
    action_node_fqn=model.FQN_ACTION_NODE_ENEMY,
    action_fqn_prefix=model.ACTION_FQN_PREFIX_ENEMY,
    action_base_fqn="GameCore::Npc::Enemy::Behaviour::ActionBase",
    asset_fqn="NanamiEngine::Module::Asset::EnemyBehaviourFile",
    data_ext=".enemyBehaviourData",
    meta_ext=".enemyBehaviourData.meta",
    default_dir="Assets/Data/EnemyBehaviour",
    action_dir_rel="Assets/Scripts/Core/Game/Npc/Enemy/Behaviour/Action",
    headers_agg=_REPO / "Assets/Scripts/Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionHeaders.h",
    content_anchor=r"Assets\Scripts\Core\Game\Npc\Enemy\Behaviour\Action\Content",
    factory_rel_suffix="Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h",
    base_include_name="Enemy_Behaviour_ActionBase.h",
    action_file_prefix="Enemy_Behaviour_Action_",
    register_macro="REGISTER_ENEMY_ACTION_WITH_NAME",
    register_named_re=re.compile(
        r'REGISTER_ENEMY_ACTION_WITH_NAME\s*\(\s*(\w+)\s*,\s*"((?:[^"\\]|\\.)*)"\s*\)'
    ),
    register_bare_re=re.compile(r"REGISTER_ENEMY_ACTION\s*\(\s*(\w+)\s*\)"),
    catalog_path=Path(__file__).with_name("catalog.json"),
    bind_hint=(
        "bind it to an enemy: set the EnemyBase.behaviourData_ field to this asset\n"
        "in the prefab inspector, or edit the prefab JSON so\n"
        '  ...components_.component_N.data...behaviourData_.value0.ptr_wrapper.data.value0.value_'
    ),
)

FRIENDLY = NpcKind(
    name="friendly",
    cpp_namespace_segment="Friendly",
    action_node_fqn=model.FQN_ACTION_NODE_FRIENDLY,
    action_fqn_prefix=model.ACTION_FQN_PREFIX_FRIENDLY,
    action_base_fqn="GameCore::Npc::Friendly::Behaviour::ActionBase",
    asset_fqn="NanamiEngine::Module::Asset::FriendNpcBehaviourFile",
    data_ext=".friendBehaviourData",
    meta_ext=".friendBehaviourData.meta",
    default_dir="Assets/Data/FriendlyNpcBehviour",
    action_dir_rel="Assets/Scripts/Core/Game/Npc/Friendly/Behaviour/Action",
    headers_agg=_REPO / "Assets/Scripts/Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionHeaders.h",
    content_anchor=r"Assets\Scripts\Core\Game\Npc\Friendly\Behaviour\Action\Content",
    factory_rel_suffix="Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h",
    base_include_name="Friendly_Behaviour_ActionBase.h",
    action_file_prefix="Friendly_Behaviour_Action_",
    register_macro="REGISTER_FRIENDLY_ACTION_WITH_NAME",
    register_named_re=re.compile(
        r'REGISTER_FRIENDLY_ACTION_WITH_NAME\s*\(\s*(\w+)\s*,\s*"((?:[^"\\]|\\.)*)"\s*\)'
    ),
    register_bare_re=re.compile(r"REGISTER_FRIENDLY_ACTION\s*\(\s*(\w+)\s*\)"),
    catalog_path=Path(__file__).with_name("catalog_friendly.json"),
    bind_hint=(
        "bind it to a friendly NPC: set the FriendlyNpc.friendlyNpcBehaviourFile_ field to this\n"
        "asset in the prefab inspector, or edit the prefab JSON so\n"
        '  ...components_.component_N.data...friendlyNpcBehaviourFile_.value0.ptr_wrapper.data.value0.value_'
    ),
)

BY_NAME: dict[str, NpcKind] = {"enemy": ENEMY, "friendly": FRIENDLY}
BY_DATA_EXT: dict[str, NpcKind] = {k.data_ext: k for k in BY_NAME.values()}
BY_META_EXT: dict[str, NpcKind] = {k.meta_ext: k for k in BY_NAME.values()}
BY_ACTION_NODE_FQN: dict[str, NpcKind] = {k.action_node_fqn: k for k in BY_NAME.values()}


def by_name(name: str) -> NpcKind:
    try:
        return BY_NAME[name]
    except KeyError:
        raise ValueError(f"unknown npc kind: {name!r} (expected one of {sorted(BY_NAME)})") from None


def kind_for_path(path) -> Optional[NpcKind]:
    """ファイル名からベストエフォートで種別を判定する（より長く具体的に一致する
    ``.meta`` 接尾辞を先に調べる）。"""
    s = str(path)
    for ext, k in BY_META_EXT.items():
        if s.endswith(ext):
            return k
    for ext, k in BY_DATA_EXT.items():
        if s.endswith(ext):
            return k
    return None


def resolve_tree_path(arg: str, repo: Path) -> Path:
    """CLI のパス引数をツリーのデータファイルに解決する。各編集コマンドの ``file``
    位置引数と同じ方法で: ``arg`` が既知の拡張子を持てばそのまま使い、そうでなければ
    各種別の ``data_ext`` を順に付けて試し、実際にディスク上に存在する候補を返す
    （明示パスの解決と同じく cwd 相対とリポジトリ相対の両方を調べる）。どちらの種別も
    ディスク上で一致しなければ Enemy の拡張子（従来からのデフォルト）にフォールバック
    するので、呼び出し側の "not found" エラーでも妥当なパスが示される。
    """

    def _existing(c: Path) -> Optional[Path]:
        if c.is_absolute():
            return c if c.exists() else None
        if c.exists():
            return c
        rc = repo / c
        return rc if rc.exists() else None

    p = Path(arg)
    if kind_for_path(p) is not None:
        found = _existing(p)
        return found if found is not None else (p if p.is_absolute() else repo / p)

    for k in (ENEMY, FRIENDLY):
        cand = Path(str(p) + k.data_ext)
        found = _existing(cand)
        if found is not None:
            return found

    default = Path(str(p) + ENEMY.data_ext)
    return default if default.is_absolute() else repo / default
