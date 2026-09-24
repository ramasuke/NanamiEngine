""".png.meta`` - ``SpriteFile`` テクスチャアセット用のサイドカーバインディング。

``SpriteFile`` は ``SceneFile``/``PrefabGameObjectFile`` と同じ「薄いプロキシ」系の
アセット（``tools/common/meta_base.py`` 参照）。
``Engine/Module/Asset/Sprite/SpriteFile.h``（``base_class<AssetBase>()``
+ ``base_class<IEnablableAsset>()`` -> ``contentPath_``/``guid_`` の前に空の ``valueN``
ラッパーが 2 つ）と、``Assets/Art/`` 以下にチェックイン済みの実際の ``*.png.meta`` で確認済み。

同じパターンを他のアセット型に適用したものは ``tools/effect/meta.py`` /
``tools/bt/meta.py`` / ``tools/scene/meta.py`` を参照。
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
    """実際の ``*.png.meta`` ファイルの ``contentPath_`` の規約に合わせる。

    このリポジトリにチェックインされている ``*.png.meta``（例:
    ``Assets/Art/UI/KnightStatusUI/StatusBarFrame.png.meta``）はすべて、ファイル名の前に
    スラッシュを含まない **すべてバックスラッシュ** のパスを使う。これは汎用の薄いプロキシ用
    フォールバック ``meta_base.content_path_for``（``bt``/``animtree`` のデータファイル向けに
    そこへスラッシュを入れる）とは異なる。同じ理由による ``tools/effect/meta.py`` の
    オーバーライドと同様。
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
