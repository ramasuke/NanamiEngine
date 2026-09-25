"""tools/engine_dist のセルフテスト (MSBuild 無し): このリポジトリから一時ディレクトリにパッケージを組み立てる。"""

from __future__ import annotations

import json
import sys
import tempfile
from pathlib import Path

_REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(_REPO))

from tools.engine_dist import package as pkg  # noqa: E402


def main() -> int:
    failures: list[str] = []
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "NanamiEngine-test"
        result = pkg.assemble(_REPO, out, "0.0.0-test", allow_missing_libs=True)

        for name in pkg.ROOT_FILES + ("engine.json",):
            if not (out / name).is_file():
                failures.append(f"missing root file: {name}")
        if json.loads((out / "engine.json").read_text(encoding="utf-8"))["version"] != "0.0.0-test":
            failures.append("engine.json version mismatch")
        if not (out / "Engine" / "Module" / "Component" / "ComponentBase.h").is_file():
            failures.append("engine headers not copied")
        if not (out / "Template" / "__PROJECT__.vcxproj").is_file():
            failures.append("template not copied")
        # Editor 構成は DLL: リポジトリにある分は dll / pdb も一緒に配られる
        for configuration in pkg.CONFIGURATIONS:
            for source in pkg.lib_artifacts(_REPO, "Editor", configuration):
                if source.is_file() and not (out / source.relative_to(_REPO)).is_file():
                    failures.append(f"missing Editor artifact: {source.relative_to(_REPO)}")
        for configuration in pkg.CONFIGURATIONS:
            if (out / "lib" / "Game" / configuration / "NanamiEngine.dll").exists():
                failures.append("Game mode must stay a static lib (no dll)")

        sources = [p for d in pkg.SOURCE_DIRECTORIES for p in (out / d).rglob("*")
                   if p.suffix.lower() in (".cpp", ".c", ".cc", ".cxx")]
        if sources:
            failures.append(f"{len(sources)} source files shipped, e.g. {sources[0]}")
        if (out / "Assets").exists():
            failures.append("Assets/ must not be shipped")
        print(f"assembled {result.file_count} files (missing libs: {len(result.missing_libs)})")

    for failure in failures:
        print("FAIL:", failure)
    print("OK" if not failures else f"{len(failures)} failure(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
