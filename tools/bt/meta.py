"""``.enemyBehaviourData.meta`` sidecar files.

A ``.meta`` is a cereal-JSON ``std::shared_ptr<AssetBase>`` holding the
``EnemyBehaviourFile`` ScriptableObject: a stable asset ``guid_`` (what an enemy
prefab's ``behaviourData_`` field references) plus ``contentPath_`` back to the
data file. Written by ``File::OnSave()`` in the engine; reproduced here so
``new-tree`` yields an asset the editor and prefabs can bind to immediately.

Thin binding of the generic :mod:`tools.common.meta_base` codec for the
``EnemyBehaviourFile`` asset type - see that module for the format notes.
"""

from __future__ import annotations

from pathlib import Path

from tools.common import meta_base as _base
from tools.common.meta_base import MetaSpec

from . import npc_kind

# backward-compat module-level constants (enemy - the tool's original/default flavor)
ASSET_FQN = npc_kind.ENEMY.asset_fqn
DATA_EXT = npc_kind.ENEMY.data_ext
META_EXT = npc_kind.ENEMY.meta_ext
DEFAULT_DIR = npc_kind.ENEMY.default_dir

_SPECS: dict[str, MetaSpec] = {
    k.name: MetaSpec(
        asset_fqn=k.asset_fqn,
        data_ext=k.data_ext,
        meta_ext=k.meta_ext,
        default_dir=k.default_dir,
        outer_class_version=0,
    )
    for k in npc_kind.BY_NAME.values()
}

mint_guid = _base.mint_guid


def content_path_for(name: str, target_dir: Path, repo_root: Path, *, kind: str = "enemy") -> str:
    return _base.content_path_for(_SPECS[kind], name, target_dir, repo_root)


def render_meta(name: str, guid: str, content_path: str, *, kind: str = "enemy") -> str:
    return _base.render_meta(_SPECS[kind], name, guid, content_path)


def write_meta(path: Path, name: str, guid: str, content_path: str, *, kind: str = "enemy") -> None:
    _base.write_meta(_SPECS[kind], path, name, guid, content_path)


def read_meta(path: Path, *, kind: str = "enemy") -> dict:
    return _base.read_meta(_SPECS[kind], path)
