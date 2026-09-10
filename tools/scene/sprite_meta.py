""".png.meta`` sidecar bindings for ``SpriteFile`` texture assets.

``SpriteFile`` is the same "thin proxy" asset family as ``SceneFile``/
``PrefabGameObjectFile`` (see ``tools/common/meta_base.py``) - confirmed
against ``Engine/Module/Asset/Sprite/SpriteFile.h`` (``base_class<AssetBase>()``
+ ``base_class<IEnablableAsset>()`` -> two empty ``valueN`` wrappers before
``contentPath_``/``guid_``) and real ``*.png.meta`` files already checked in
under ``Assets/Art/``.

See ``tools/effect/meta.py`` / ``tools/bt/meta.py`` / ``tools/scene/meta.py``
for the same pattern applied to other asset types.
"""

from __future__ import annotations

from pathlib import Path

from tools.common import meta_base as _base
from tools.common.meta_base import MetaSpec

SPRITE_SPEC = MetaSpec(
    asset_fqn="NanamiEngine::Module::Asset::SpriteFile",
    data_ext=".png",
    meta_ext=".png.meta",
    default_dir="Assets/Art/UI",
    outer_class_version=0,
    base_class_count=2,
)

mint_guid = _base.mint_guid


def content_path_for(name: str, target_dir: Path, repo_root: Path) -> str:
    """Match real ``*.png.meta`` files' ``contentPath_`` convention.

    Every ``*.png.meta`` checked in this repo (e.g.
    ``Assets/Art/UI/KnightStatusUI/StatusBarFrame.png.meta``) uses an
    **all-backslash** path with no forward slash before the filename - unlike
    the generic thin-proxy fallback in ``meta_base.content_path_for`` (which
    inserts a forward slash there for ``bt``/``animtree`` data files). Mirrors
    ``tools/effect/meta.py``'s override for the same reason.
    """
    for sib in sorted(target_dir.glob("*" + SPRITE_SPEC.meta_ext)):
        try:
            cp = _base.read_meta(SPRITE_SPEC, sib)["content_path"]
        except Exception:  # noqa: BLE001
            continue
        prefix = cp.rsplit("\\", 1)[0] if "\\" in cp else cp.rsplit("/", 1)[0]
        return f"{prefix}\\{name}{SPRITE_SPEC.data_ext}"
    try:
        rel = target_dir.resolve().relative_to(repo_root.resolve())
        dir_bs = str(rel).replace("/", "\\")
    except ValueError:
        dir_bs = SPRITE_SPEC.default_dir.replace("/", "\\")
    return f"{dir_bs}\\{name}{SPRITE_SPEC.data_ext}"


def render_meta(name: str, guid: str, content_path: str) -> str:
    return _base.render_meta(SPRITE_SPEC, name, guid, content_path)


def write_meta(path: Path, name: str, guid: str, content_path: str) -> None:
    _base.write_meta(SPRITE_SPEC, path, name, guid, content_path)


def read_meta(path: Path) -> dict:
    return _base.read_meta(SPRITE_SPEC, path)
