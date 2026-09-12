""".mv1.meta`` sidecar files.

A ``.meta`` is a cereal-JSON ``std::shared_ptr<AssetBase>`` holding the
``Mv1File`` asset: a stable ``guid_`` (what a component's model field
references) plus ``contentPath_`` back to the ``.mv1`` binary. Written by
``File::OnSave()`` in the engine; reproduced here so ``install`` yields an
asset the editor/prefabs can bind to immediately.

Thin binding of the generic :mod:`tools.common.meta_base` codec for the
``Mv1File`` asset type - see that module for the format notes, and
``tools/effect/meta.py`` for the same pattern applied to ``ParticleFile``
(same ``base_class_count=2`` shape: ``Mv1File`` also ``base_class<>()``s both
``AssetBase`` and ``LifeCycleCallback::IEnablableAsset`` before its own
``contentPath_``/``guid_`` - see ``Engine/Module/Asset/MV1/MV1File.h``).
"""

from __future__ import annotations

from pathlib import Path

from tools.common import meta_base as _base
from tools.common.meta_base import MetaSpec

ASSET_FQN = "NanamiEngine::Module::Asset::Mv1File"
DATA_EXT = ".mv1"
META_EXT = ".mv1.meta"
# .mv1 assets have no single root the way .efkefc does (Assets/Art/Animation/**,
# Assets/Art/Models/**, Assets/BruteAnimation, ...) - this is only the fallback
# used when a target directory has no sibling .mv1.meta to copy the convention
# from (see content_path_for below).
DEFAULT_DIR = "Assets/Art/Models"

_SPEC = MetaSpec(
    asset_fqn=ASSET_FQN,
    data_ext=DATA_EXT,
    meta_ext=META_EXT,
    default_dir=DEFAULT_DIR,
    outer_class_version=1,
    # Mv1File::save/load base_class<>()s both AssetBase and
    # LifeCycleCallback::IEnablableAsset -> two empty valueN wrappers, not one.
    base_class_count=2,
)

mint_guid = _base.mint_guid


def content_path_for(name: str, target_dir: Path, repo_root: Path) -> str:
    """Match real ``.mv1.meta``'s ``contentPath_`` convention.

    Unlike the other thin-proxy asset types (see ``meta_base.content_path_for``'s
    "forward slash before the file" convention), every real ``.mv1.meta``
    checked in this repo (``Assets/Art/Animation/Man/Death.mv1.meta``,
    ``.../Jump.mv1.meta``, ``Assets/Art/Animation/Hyenas/...``) uses an
    **all-backslash** path with no exception - matches ``tools/effect/meta.py``'s
    ``ParticleFile`` binding for the same reason, so this binding doesn't
    delegate to the generic fallback either.
    """
    for sib in sorted(target_dir.glob("*" + META_EXT)):
        try:
            cp = _base.read_meta(_SPEC, sib)["content_path"]
        except Exception:  # noqa: BLE001
            continue
        prefix = cp.rsplit("\\", 1)[0] if "\\" in cp else cp.rsplit("/", 1)[0]
        return f"{prefix}\\{name}{DATA_EXT}"
    try:
        rel = target_dir.resolve().relative_to(repo_root.resolve())
        dir_bs = str(rel).replace("/", "\\")
    except ValueError:
        dir_bs = DEFAULT_DIR.replace("/", "\\")
    return f"{dir_bs}\\{name}{DATA_EXT}"


def render_meta(name: str, guid: str, content_path: str) -> str:
    return _base.render_meta(_SPEC, name, guid, content_path)


def write_meta(path: Path, name: str, guid: str, content_path: str) -> None:
    _base.write_meta(_SPEC, path, name, guid, content_path)


def read_meta(path: Path) -> dict:
    return _base.read_meta(_SPEC, path)
