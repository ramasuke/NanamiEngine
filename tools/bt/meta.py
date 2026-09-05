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

ASSET_FQN = "NanamiEngine::Module::Asset::EnemyBehaviourFile"
DATA_EXT = ".enemyBehaviourData"
META_EXT = ".enemyBehaviourData.meta"
DEFAULT_DIR = "Assets/Data/EnemyBehaviour"

_SPEC = MetaSpec(
    asset_fqn=ASSET_FQN,
    data_ext=DATA_EXT,
    meta_ext=META_EXT,
    default_dir=DEFAULT_DIR,
    outer_class_version=0,
)

mint_guid = _base.mint_guid


def content_path_for(name: str, target_dir: Path, repo_root: Path) -> str:
    return _base.content_path_for(_SPEC, name, target_dir, repo_root)


def render_meta(name: str, guid: str, content_path: str) -> str:
    return _base.render_meta(_SPEC, name, guid, content_path)


def write_meta(path: Path, name: str, guid: str, content_path: str) -> None:
    _base.write_meta(_SPEC, path, name, guid, content_path)


def read_meta(path: Path) -> dict:
    return _base.read_meta(_SPEC, path)
