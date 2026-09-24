"""エンジン lib の各バリアントをビルドし、インストール可能なエンジンフォルダを組み立てる。

パッケージしたエンジンのレイアウト (NanamiHub が Engines/<version>/ 以下にインストールするもの):

    engine.json                      {"name", "version", "builtAt"}
    NanamiEngine.props               共通のビルド設定 (NanamiEngineDir = このフォルダ)
    NanamiEngine.Game.props          ゲーム exe の設定 (lib/<Mode>/<Config>/NanamiEngine.lib をリンク)
    stdafx.h / stdafx.cpp            プリコンパイル済みヘッダ。各ゲームプロジェクトがコンパイルする
    Engine/ Packages/ Libs/          ヘッダとサードパーティのライブラリ (.c/.cpp は含まない)
    lib/<Editor|Game>/<Debug|Release>/NanamiEngine.lib
    Template/                        新規プロジェクトのテンプレート (トークンは NanamiHub が埋める)
"""

from __future__ import annotations

import datetime as _dt
import json
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path

MODES = ("Editor", "Game")
CONFIGURATIONS = ("Debug", "Release")
SOURCE_DIRECTORIES = ("Engine", "Packages", "Libs")
ROOT_FILES = ("NanamiEngine.props", "NanamiEngine.Game.props", "stdafx.h", "stdafx.cpp")
# NOTE: lib に焼き込まれるので配らない
EXCLUDED_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".vcxproj", ".filters", ".user", ".obj", ".pdb", ".ilk", ".tlog"}
EXCLUDED_NAMES = {"desktop.ini", "thumbs.db", ".ds_store"}
TEMPLATE_DIR = Path(__file__).resolve().parent / "template"
LIB_PROJECT = "NanamiEngine.vcxproj"


class PackageError(RuntimeError):
    pass


@dataclass
class PackageResult:
    out_dir: Path
    file_count: int
    missing_libs: list[str]


def find_msbuild(explicit: str | None = None) -> Path:
    if explicit:
        path = Path(explicit)
        if not path.is_file():
            raise PackageError(f"MSBuild が見つかりません: {path}")
        return path
    vswhere = Path(r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe")
    if not vswhere.is_file():
        raise PackageError("vswhere.exe が見つかりません。--msbuild で MSBuild.exe を指定してください")
    out = subprocess.run(
        [str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.Component.MSBuild",
         "-utf8", "-find", r"MSBuild\**\Bin\MSBuild.exe"],
        capture_output=True, check=False)
    lines = out.stdout.decode("utf-8", errors="replace").splitlines()
    if not lines or not Path(lines[0]).is_file():
        raise PackageError("vswhere で MSBuild が見つかりません。--msbuild で指定してください")
    return Path(lines[0])


def lib_path(root: Path, mode: str, configuration: str) -> Path:
    return root / "lib" / mode / configuration / "NanamiEngine.lib"


def build_libs(repo: Path, msbuild: Path, modes=MODES, configurations=CONFIGURATIONS) -> None:
    for mode in modes:
        for configuration in configurations:
            print(f"[build] {mode} | {configuration}")
            result = subprocess.run(
                [str(msbuild), str(repo / LIB_PROJECT),
                 f"-p:Configuration={configuration}", "-p:Platform=x64", "-p:PreferredToolArchitecture=x64",
                 f"-p:NanamiApplicationMode={mode}", "-m", "-nologo", "-verbosity:minimal"],
                cwd=repo, check=False)
            if result.returncode != 0:
                raise PackageError(f"{mode} | {configuration} のビルドに失敗しました (exit {result.returncode})")


def _is_shipped(path: Path) -> bool:
    return path.suffix.lower() not in EXCLUDED_SUFFIXES and path.name.lower() not in EXCLUDED_NAMES


def _copy_tree(source: Path, destination: Path) -> int:
    count = 0
    for file in source.rglob("*"):
        if not file.is_file() or not _is_shipped(file):
            continue
        target = destination / file.relative_to(source)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(file, target)
        count += 1
    return count


def assemble(repo: Path, out_dir: Path, version: str, *, allow_missing_libs: bool = False) -> PackageResult:
    missing = [f"{m}|{c}" for m in MODES for c in CONFIGURATIONS if not lib_path(repo, m, c).is_file()]
    if missing and not allow_missing_libs:
        raise PackageError("ビルドされていない lib があります: " + ", ".join(missing))
    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True)

    count = 0
    for name in SOURCE_DIRECTORIES:
        count += _copy_tree(repo / name, out_dir / name)
    for name in ROOT_FILES:
        shutil.copy2(repo / name, out_dir / name)
        count += 1
    for mode in MODES:
        for configuration in CONFIGURATIONS:
            source = lib_path(repo, mode, configuration)
            if source.is_file():
                target = lib_path(out_dir, mode, configuration)
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source, target)
                count += 1
    shutil.copytree(TEMPLATE_DIR, out_dir / "Template")
    count += sum(1 for p in TEMPLATE_DIR.rglob("*") if p.is_file())

    info = {
        "name": "NanamiEngine",
        "version": version,
        "builtAt": _dt.datetime.now().astimezone().isoformat(timespec="seconds"),
    }
    (out_dir / "engine.json").write_text(json.dumps(info, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return PackageResult(out_dir, count + 1, missing)


def make_zip(out_dir: Path) -> Path:
    archive = shutil.make_archive(str(out_dir), "zip", root_dir=out_dir)
    return Path(archive)
