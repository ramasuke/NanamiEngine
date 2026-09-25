"""ゲームコード (Assets/) が DxLib を直接使っていないことを確かめる (docs/HotReload.md 段階 1)。

Game.dll に DxLib を静的リンクするとハンドル表が二重化して壊れるので、ゲームコードは DxLib を呼ばず
Engine/Core/Platform/* のファサード (Input / Draw2D / Render / AsyncLoad)、Time::NowMilliseconds、SoundFile の再生 API、
Render3D::Shapes などを使う (CLAUDE.md「Game code must not call DxLib」)。

見るもの:
  - #include "DxLib.h" / "DxDataTypeWin.h" / "EffekseerForDXLib.h" / Libs/LibCore/DxLib/* (橋渡しフォルダ)
  - 同梱ヘッダの extern 宣言から集めた DxLib の関数名の呼び出し (DxLib::Foo( / Foo( の形。obj.Foo( は除く)
  - DxLib の型と定数 (VECTOR / MATRIX / COLOR_U8 / XINPUT_STATE / DX_* / KEY_INPUT_* / XINPUT_BUTTON_* / MOUSE_INPUT_* / VGet ...)

    python tools/dxlib_guard/check_game_dxlib.py            # 0 = 何も無い
    python tools/dxlib_guard/check_game_dxlib.py --verbose  # 全行を出す
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DXLIB_DIR = ROOT / "Libs" / "プロジェクトに追加すべきファイル_VC用"
GAME_DIRS = ("Assets",)

BRIDGE_INCLUDES = re.compile(r'#include\s*[<"]([^">]*(?:DxLib\.h|DxDataTypeWin\.h|DxDataType\.h|DxFunctionWin\.h|EffekseerForDXLib\.h|Libs/LibCore/DxLib/[^">]+))[">]')
TYPES_AND_CONSTANTS = re.compile(
    r"\b(VECTOR|MATRIX|COLOR_U8|COLOR_F|FLOAT4|VERTEX3D|VERTEX3DSHADER|VERTEX2D|XINPUT_STATE|MV1_COLL_RESULT_POLY|HITRESULT_LINE"
    r"|DX_[A-Z0-9_]+|KEY_INPUT_[A-Z0-9_]+|XINPUT_BUTTON_[A-Z0-9_]+|MOUSE_INPUT_[A-Z0-9]+|PAD_INPUT_[A-Z0-9_]+"
    r"|VGet|VAdd|VSub|VScale|VNorm|VCross|VDot|VSize|MGetIdent|MMult|MGetTranslate|MGetRotX|MGetRotY|MGetRotZ|GetColorU8|GetColorF)\b")
# ゲーム側で同名の関数を持っているもの、DxLib 以外にもある名前
IGNORED_NAMES = {"PlaySound", "GetWindowSize", "GetColor"}


def dxlib_function_names() -> set[str]:
    names: set[str] = set()
    for header in ("DxLib.h", "DxFunctionWin.h"):
        path = DXLIB_DIR / header
        if not path.exists():
            continue
        text = path.read_bytes().decode("cp932", "replace")
        for m in re.finditer(r"^\s*extern\s+[A-Za-z_ \*]+?\s+([A-Z][A-Za-z0-9_]+)\s*\(", text, re.M):
            names.add(m.group(1))
    return names - IGNORED_NAMES


def strip_comments(line: str) -> str:
    return re.sub(r"//.*", "", line)


def scan(verbose: bool) -> int:
    names = dxlib_function_names()
    if not names:
        print(f"DxLib のヘッダが見つかりません: {DXLIB_DIR}", file=sys.stderr)
        return 2
    call = re.compile(r"(?<![\w.>:])(?:DxLib::)?([A-Z][A-Za-z0-9_]+)\s*\(")
    findings: list[tuple[Path, int, str, str]] = []
    for d in GAME_DIRS:
        for path in sorted((ROOT / d).rglob("*")):
            if path.suffix not in (".cpp", ".h") or not path.is_file():
                continue
            for number, line in enumerate(path.read_text(encoding="utf-8-sig", errors="replace").split("\n"), 1):
                code = strip_comments(line)
                if m := BRIDGE_INCLUDES.search(code):
                    findings.append((path, number, "include", m.group(1)))
                    continue
                for m in call.finditer(code):
                    if m.group(1) in names:
                        findings.append((path, number, "call", m.group(1)))
                for m in TYPES_AND_CONSTANTS.finditer(code):
                    findings.append((path, number, "type/const", m.group(1)))
    files = sorted({f for f, *_ in findings})
    if not findings:
        print("OK: game code does not use DxLib directly")
        return 0
    print(f"NG: {len(findings)} DxLib references in {len(files)} game files")
    shown = 0
    for path, number, kind, what in findings:
        if not verbose and shown >= 40:
            print("  ... (--verbose で全部出す)")
            break
        print(f"  {path.relative_to(ROOT)}:{number}: [{kind}] {what}")
        shown += 1
    return 1


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args(argv)
    return scan(args.verbose)


if __name__ == "__main__":
    sys.exit(main())
