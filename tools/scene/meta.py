"""``.scene.meta`` / ``.prefab.meta`` sidecar bindings.

Both ``SceneFile`` and ``PrefabGameObjectFile`` are the same "thin proxy, real
data in a sibling content file" asset family as ``EnemyBehaviourFile`` (see
``tools/bt/meta.py`` and ``tools/common/meta_base.py``) - confirmed against
``Engine/Module/Asset/Scene/SceneFile.h`` (``cereal_class_version`` 0) and
``Engine/Module/GameObject/PrefabGameObject/PrefabGameObject.h``
(``CEREAL_CLASS_VERSION(..., 1)``).
"""

from __future__ import annotations

from pathlib import Path

from tools.common import meta_base as _base
from tools.common.meta_base import MetaSpec

SCENE_SPEC = MetaSpec(
    asset_fqn="NanamiEngine::Module::Asset::SceneFile",
    data_ext=".scene",
    meta_ext=".scene.meta",
    default_dir="Assets/Scene",
    outer_class_version=0,
)
PREFAB_SPEC = MetaSpec(
    asset_fqn="NanamiEngine::Module::Asset::PrefabGameObjectFile",
    data_ext=".prefab",
    meta_ext=".prefab.meta",
    default_dir="Assets/Prefab",
    outer_class_version=1,
)

mint_guid = _base.mint_guid


def content_path_for(spec: MetaSpec, name: str, target_dir: Path, repo_root: Path) -> str:
    return _base.content_path_for(spec, name, target_dir, repo_root)


def render_meta(spec: MetaSpec, name: str, guid: str, content_path: str) -> str:
    return _base.render_meta(spec, name, guid, content_path)


def write_meta(spec: MetaSpec, path: Path, name: str, guid: str, content_path: str) -> None:
    _base.write_meta(spec, path, name, guid, content_path)


def read_meta(spec: MetaSpec, path: Path) -> dict:
    return _base.read_meta(spec, path)
