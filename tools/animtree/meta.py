""".animTree.meta`` sidecar files.

A ``.meta`` is a cereal-JSON ``std::shared_ptr<AssetBase>`` holding the
``AnimationTreeFile`` thin-proxy asset: a stable asset ``guid_`` (what an
``Animator`` component's ``animationTreeFile_`` field references) plus
``contentPath_`` back to the ``.animTree`` data file. Written by
``File::OnSave()`` in the engine; reproduced here so ``new-tree`` yields an
asset the editor and prefabs can bind to immediately.

Thin binding of the generic :mod:`tools.common.meta_base` codec for the
``AnimationTreeFile`` asset type - see that module for the format notes.
"""

from __future__ import annotations

from pathlib import Path

from tools.common import meta_base as _base
from tools.common.meta_base import MetaSpec

ASSET_FQN = "NanamiEngine::Module::Asset::AnimationTreeFile"
DATA_EXT = ".animTree"
META_EXT = ".animTree.meta"
DEFAULT_DIR = "Assets/Animations"

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


# -- a second, private MetaSpec used only to resolve --clip <path> arguments
# (a .mv1/.mv1.meta) to the referenced Mv1File asset's guid - see edits.resolve_clip_arg.
MV1_ASSET_FQN = "NanamiEngine::Module::Asset::Mv1File"
MV1_DATA_EXT = ".mv1"
MV1_META_EXT = ".mv1.meta"
MV1_DEFAULT_DIR = "Assets/Art/Animation"

_MV1_SPEC = MetaSpec(
    asset_fqn=MV1_ASSET_FQN,
    data_ext=MV1_DATA_EXT,
    meta_ext=MV1_META_EXT,
    default_dir=MV1_DEFAULT_DIR,
    outer_class_version=0,
)


def read_mv1_meta(path: Path) -> dict:
    return _base.read_meta(_MV1_SPEC, path)
