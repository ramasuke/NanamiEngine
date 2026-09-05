"""Registry describing the two BehaviourTree "flavors" this toolkit supports -
``Enemy`` and ``FriendlyNpc`` - so the rest of ``tools/bt`` can be generic over
which one a given file/command targets instead of hardcoding Enemy paths
everywhere. Mirrors the engine's own ``BehaviourTreeType`` split
(``Editor::BehaviourTreeType::EnemyNpc`` / ``FriendlyNpc``): both flavors share
the same composite node types (Selector/Sequence/...), file-format shell and
``.meta`` convention, but each has its own ``ActionNode`` C++ type, ``ActionBase``
hierarchy, action factory/registration macro, and on-disk action tree.

Every "which Enemy-or-Friendly thing" question in this package should route
through :data:`ENEMY` / :data:`FRIENDLY` (or :func:`by_name`) rather than a new
hardcoded path, so a third flavor - should one ever appear - only needs a new
entry here.
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
    name: str                       # "enemy" | "friendly" - the --kind value
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
    content_anchor: str              # vcxproj splice anchor, Windows-separated
    factory_rel_suffix: str          # Editor/Npc/<X>/Behaviour/Action/<X>_Behaviour_ActionFactory.h
    base_include_name: str          # <X>_Behaviour_ActionBase.h
    action_file_prefix: str         # <X>_Behaviour_Action_

    register_macro: str             # REGISTER_ENEMY_ACTION_WITH_NAME | REGISTER_FRIENDLY_ACTION_WITH_NAME
    register_named_re: re.Pattern
    register_bare_re: re.Pattern

    catalog_path: Path
    bind_hint: str                   # printed guidance for new-tree

    @property
    def action_root(self) -> Path:
        return _REPO / self.action_dir_rel

    @property
    def content_root(self) -> Path:
        return self.action_root / "Content"

    @property
    def extra_struct_files(self) -> dict[str, Path]:
        # Both flavors keep their embedded-struct headers (e.g. a standalone
        # Position) directly under Action/, mirroring each other 1:1.
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
    """Best-effort kind detection from a file's name (checks the ``.meta``
    suffix first since it is the longer/more specific match)."""
    s = str(path)
    for ext, k in BY_META_EXT.items():
        if s.endswith(ext):
            return k
    for ext, k in BY_DATA_EXT.items():
        if s.endswith(ext):
            return k
    return None


def resolve_tree_path(arg: str, repo: Path) -> Path:
    """Resolve a CLI path argument to a tree data file, the way every edit
    verb's ``file`` positional does: if ``arg`` already names a recognised
    extension use it as-is, otherwise try appending each known kind's
    ``data_ext`` in turn and return whichever candidate actually exists on
    disk (checked both cwd-relative and repo-relative, matching how an
    explicit path is resolved). Falls back to the Enemy extension (today's
    long-standing default) when nothing on disk matches either kind, so the
    caller's own "not found" error still names a sensible path.
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
