"""配信する ``.efkefc`` / ``.mv1`` が「この PC にしか無いファイル」を参照していないかを調べる。

Effekseer と DxLib はテクスチャやモデルを、それを参照するファイルからの相対パスで引く。参照先が
``Assets/`` の外 (デスクトップの素材フォルダなど) や配信対象外 (``_Source/`` など) にあると、開発 PC では
表示されるのにプレイヤーの PC では見つからない。2026-09-18 には、使用中のエフェクト 8 件が
``../../../../../Effekseer素材/...`` を参照していた。

どこにも実在しない参照は対象にしない。開発 PC でも壊れているので配信の問題ではないうえ、``.mv1`` は
元 FBX の絶対パス (``C:\\Tarisland - Dragon\\X.png``) を相対パスと並べて持っていることがあるため。
"""

from __future__ import annotations

import os
from dataclasses import dataclass, field
from pathlib import Path

from tools.dist.manifest import HashCache, ScanResult, is_excluded
from tools.effect import efkefc
from tools.model import mv1

#: 中の相対パスでほかのファイルを引く形式 (小文字)
REFERRING_SUFFIXES: tuple[str, ...] = (".efkefc", ".mv1")

REASON_OUTSIDE = "Assets の外"
REASON_EXCLUDED = "配信対象外"


@dataclass
class UnshippedRef:
    #: 参照している側 (リポジトリルートからの POSIX 相対)
    path: str
    #: ファイルに書かれているとおりの参照
    ref: str
    reason: str


@dataclass
class RefReport:
    unshipped: list[UnshippedRef] = field(default_factory=list)
    #: 参照を読み出せなかったファイル (壊れている / 知らない形式)
    unreadable: list[str] = field(default_factory=list)


def references_of(path: Path) -> list[str] | None:
    """``path`` の中にある参照を、書かれているとおりに返す。読めなければ None。"""
    try:
        if path.suffix.lower() == ".efkefc":
            return efkefc.asset_paths(path.read_bytes())
        return mv1.texture_paths(path)
    except (ValueError, OSError):  # EfkefcError も ValueError
        return None


def _resolve(owner: Path, ref: str) -> Path:
    target = Path(ref.replace("\\", "/"))
    if not target.is_absolute():
        target = owner.parent / target
    return Path(os.path.normpath(target))


def _relative_to(child: Path, parent: Path) -> str | None:
    """``child`` が ``parent`` の下にあれば POSIX 相対パス、無ければ None (大文字小文字は区別しない)。"""
    c, p = os.path.normcase(str(child)), os.path.normcase(str(parent)).rstrip("\\/")
    if not c.startswith(p + os.sep):
        return None
    return str(child)[len(p) + 1:].replace(os.sep, "/")


def find_unshipped_refs(scan_result: ScanResult, repo_root: Path, assets_root: Path, cache: HashCache) -> RefReport:
    repo_root = Path(os.path.normpath(repo_root.resolve()))
    assets_root = Path(os.path.normpath(assets_root.resolve()))
    report = RefReport()
    for entry in scan_result.entries:
        if not entry.path.lower().endswith(REFERRING_SUFFIXES):
            continue
        owner = repo_root / entry.path
        refs = cache.refs_of(entry.hash, lambda: references_of(owner))
        if refs is None:
            report.unreadable.append(entry.path)
            continue
        for ref in dict.fromkeys(refs):
            if not ref:
                continue
            target = _resolve(owner, ref)
            if not target.is_file():
                continue
            if _relative_to(target, assets_root) is None:
                report.unshipped.append(UnshippedRef(entry.path, ref, REASON_OUTSIDE))
                continue
            rel = _relative_to(target, repo_root)
            if rel is None or is_excluded(rel):
                report.unshipped.append(UnshippedRef(entry.path, ref, REASON_EXCLUDED))
    return report
