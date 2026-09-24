""".mv1.meta`` サイドカーファイル。

``.meta`` は ``Mv1File`` アセットを持つ cereal-JSON の ``std::shared_ptr<AssetBase>``:
安定した ``guid_`` (コンポーネントのモデルのフィールドが参照するもの) と、
``.mv1`` バイナリを指す ``contentPath_``。エンジンでは ``File::OnSave()`` が
書き出す。ここで同じものを作るので、``install`` の結果はすぐにエディタ/プレハブから
紐付けられるアセットになる。

汎用の :mod:`tools.common.meta_base` コーデックを ``Mv1File`` アセット型に薄く
結び付けたもの - 形式の説明はそのモジュールを、同じパターンを ``ParticleFile`` に
適用した例は ``tools/effect/meta.py`` を参照 (同じ ``base_class_count=2`` の形:
``Mv1File`` も自身の ``contentPath_``/``guid_`` の前に ``AssetBase`` と
``LifeCycleCallback::IEnablableAsset`` の両方を ``base_class<>()`` する -
``Engine/Module/Asset/MV1/MV1File.h`` 参照)。
"""

from __future__ import annotations

from pathlib import Path

from tools.common import meta_base as _base
from tools.common.meta_base import MetaSpec

ASSET_FQN = "NanamiEngine::Module::Asset::Mv1File"
DATA_EXT = ".mv1"
META_EXT = ".mv1.meta"
# .mv1 アセットには .efkefc のような単一のルートが無い (Assets/Art/Animation/**、
# Assets/Art/Models/**、Assets/BruteAnimation、...) - これは対象ディレクトリに
# 規則を写せる兄弟の .mv1.meta が無いときにだけ使うフォールバック
# (下の content_path_for 参照)。
DEFAULT_DIR = "Assets/Art/Models"

_SPEC = MetaSpec(
    asset_fqn=ASSET_FQN,
    data_ext=DATA_EXT,
    meta_ext=META_EXT,
    default_dir=DEFAULT_DIR,
    outer_class_version=1,
    # Mv1File::save/load は AssetBase と LifeCycleCallback::IEnablableAsset の
    # 両方を base_class<>() する -> 空の valueN ラッパーは 1 つではなく 2 つ。
    base_class_count=2,
)

mint_guid = _base.mint_guid


def content_path_for(name: str, target_dir: Path, repo_root: Path) -> str:
    """実際の ``.mv1.meta`` の ``contentPath_`` 規則に合わせる。

    他の薄いプロキシのアセット型 (``meta_base.content_path_for`` の「ファイル名の
    直前はスラッシュ」規則) と違い、このリポジトリで確認した実際の ``.mv1.meta``
    (``Assets/Art/Animation/Man/Death.mv1.meta``、``.../Jump.mv1.meta``、
    ``Assets/Art/Animation/Hyenas/...``) は例外なく **すべてバックスラッシュ** の
    パスを使っている - 同じ理由で ``tools/effect/meta.py`` の ``ParticleFile``
    バインディングと揃えており、こちらも汎用のフォールバックに委ねない。
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
