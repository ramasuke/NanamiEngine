"""エンジンの公開クラスに NANAMI_API を機械的に付ける (docs/HotReload.md §6、段階 2)。

対象: Engine/ Packages/ Libs/LibCore/ Libs/Singleton/ の .h にある、名前空間スコープまたはクラス内の
非テンプレート class / struct 定義 (`class Foo {`, `struct Foo final : Base {`)。次は付けない:

- テンプレート (`template<...>` 直後の class / struct) と、テンプレートクラスの中の入れ子クラス
- 関数本体・初期化子・ラムダの中のローカルクラス
- 無名名前空間の中 (登録マクロの registrar)
- 前方宣言 (`class Foo;`)、`enum class`、`friend class`
- 既に NANAMI_API が付いているもの (再実行しても変更なし = 冪等)
- NANAMI_NO_API (空マクロ) が付いているもの = export しない印。dllexport は暗黙のコピー / デストラクタまで実体化するので、
  unique_ptr のコンテナを持つ集成体 (入れ子の Node など) はこれで外す
- 名前空間スコープの非テンプレート関数宣言 (`Type Name(...);`) には先頭 (属性の後) に NANAMI_API を付ける

付けたファイルには `#include "Engine/Core/Api/NanamiApi.h"` を `#pragma once` の直後に足す。
BOM と改行コードは元のまま。

    python tools/engine_api/add_nanami_api.py            # 書き換え
    python tools/engine_api/add_nanami_api.py --dry-run  # 一覧だけ
    python tools/engine_api/add_nanami_api.py --check    # 付いていないものがあれば exit 1 (CI / 追加漏れの検出)
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROOTS = ("Engine", "Packages", "Libs/LibCore", "Libs/Singleton")
API = "NANAMI_API"
API_INCLUDE = '#include "Engine/Core/Api/NanamiApi.h"'
# NANAMI_API を定義するヘッダ自身と、DLL 境界をまたがない純テンプレートは対象外
SKIP_FILES = {
    "Engine/Core/Api/NanamiApi.h",
    "Libs/Singleton/LibCore_SingletonBase.h",
}

DECL = re.compile(r"\b(class|struct)\s+(?:alignas\([^)]*\)\s*)?(?:(NANAMI_API|NANAMI_NO_API)\s+)?([A-Za-z_]\w*)\s*(?:final\s*)?(?::|\{)")


@dataclass
class Edit:
    offset: int  # 挿入位置 (クラスなら class-key の直後、関数なら宣言の先頭)
    name: str
    line: int
    insert: str = " " + API


def mask_comments_and_strings(text: str) -> str:
    """コメント・文字列・文字リテラルを同じ長さの空白に置き換える (位置を保つ)。プリプロセッサ行はそのまま。"""
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            j = text.find("\n", i)
            j = n if j < 0 else j
            for k in range(i, j):
                out[k] = " "
            i = j
        elif c == "/" and i + 1 < n and text[i + 1] == "*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if out[k] != "\n":
                    out[k] = " "
            i = j
        elif c == '"' or c == "'":
            # R"delim( ... )delim" は稀なのでここでは扱わない (ヘッダには無い前提)
            j = i + 1
            while j < n and text[j] != c:
                if text[j] == "\\":
                    j += 1
                elif text[j] == "\n":
                    break
                j += 1
            for k in range(i + 1, min(j, n)):
                out[k] = " "
            i = j + 1
        else:
            i += 1
    return "".join(out)


def opener_kind(masked: str, brace_pos: int) -> str:
    """`{` を開いたものの種類: namespace / anon_namespace / class / template_class / enum / other"""
    start = max(0, brace_pos - 400)
    start = masked.rfind("\n", 0, start) + 1  # 行の途中から始めない (プリプロセッサ行の判定のため)
    head = masked[start:brace_pos]
    # 直前の文の先頭 (; または { または } の後) から brace までを見る
    stmt_start = max(head.rfind(";"), head.rfind("{"), head.rfind("}"))
    stmt = head[stmt_start + 1:]
    # プリプロセッサ行 (#include など。継続行込み) は文の一部ではないので落とす
    kept: list[str] = []
    continuing = False
    for line in stmt.split("\n"):
        if continuing or line.lstrip().startswith("#"):
            continuing = line.rstrip().endswith("\\")
            continue
        kept.append(line)
    stmt = "\n".join(kept)
    stripped = stmt.strip()
    if re.match(r"namespace\s*$", stripped):
        return "anon_namespace"
    if re.match(r"(inline\s+)?namespace\b", stripped):
        return "namespace"
    if re.match(r"extern\s*$", stripped):  # extern "C" (文字列はマスク済み)
        return "namespace"
    if re.match(r"enum\b", stripped):
        return "enum"
    if DECL.search(stmt + "{") and not re.search(r"\)\s*$", stripped):
        # 直前の文が template<...> で始まっていればテンプレートクラス
        if re.match(r"template\s*<", stripped):
            return "template_class"
        # template<...> と class の間に改行だけがある形 (前の文が template で終わる) も見る
        return "class"
    return "other"


# 名前空間スコープの関数宣言 (定義ではなく `;` で終わるもの)。`Type name(params) [const] [noexcept];`
FREE_FUNCTION = re.compile(r"^(?P<head>(?:\[\[[^\]]*\]\]\s*)*)(?P<decl>[^=;{}()]*?\b[A-Za-z_]\w*\s*\([^;{}]*\)\s*(?:const\s*)?(?:noexcept\s*)?(?:->\s*[^;{}]+)?)$", re.S)
NOT_FUNCTION_START = re.compile(r"^(using|typedef|template|friend|extern|static_assert|inline|constexpr|consteval|static|enum|class|struct|namespace|return|NANAMI_API)\b")


def free_function_edit(masked: str, text: str, stmt_start: int, stmt_end: int) -> Edit | None:
    """[stmt_start, stmt_end) が名前空間スコープの関数宣言なら、NANAMI_API を入れる位置を返す。"""
    stmt = masked[stmt_start:stmt_end]
    # 先頭の空白・プリプロセッサ行を飛ばす
    pos = stmt_start
    for line in stmt.split("\n"):
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            pos += len(line) + 1
            continue
        break
    body = masked[pos:stmt_end].strip()
    if not body or NOT_FUNCTION_START.match(body):
        return None
    m = FREE_FUNCTION.match(body)
    if not m:
        return None
    decl = m.group("decl")
    before_paren = decl.split("(", 1)[0].strip()
    if "=" in before_paren:  # 変数の初期化
        return None
    if re.fullmatch(r"[A-Za-z_]\w*", before_paren):  # 戻り値の型が無い = マクロ呼び出し (CEREAL_CLASS_VERSION(...) など)
        return None
    name = re.search(r"\b([A-Za-z_]\w*)\s*\($", before_paren + "(")
    # 挿入位置: 宣言の先頭 (属性 [[nodiscard]] があればその後ろ)
    lead = masked[pos:stmt_end]
    insert_at = pos + (len(lead) - len(lead.lstrip())) + len(m.group("head"))
    return Edit(insert_at, name.group(1) if name else "?", text.count("\n", 0, insert_at) + 1, API + " ")


def find_edits(text: str) -> list[Edit]:
    masked = mask_comments_and_strings(text)
    edits: list[Edit] = []
    stack: list[str] = []
    pending_template = False  # 直前の文が `template<...>` で終わっている
    stmt_start = 0            # 名前空間スコープの文の先頭 (関数宣言の検出用)
    i, n = 0, len(masked)
    while i < n:
        c = masked[i]
        if c == "#":
            # プリプロセッサ行 (継続行込み) は読み飛ばす
            j = i
            while True:
                k = masked.find("\n", j)
                if k < 0:
                    k = n
                if masked[j:k].rstrip().endswith("\\"):
                    j = k + 1
                    continue
                break
            i = k
            continue
        if c == "{":
            kind = opener_kind(masked, i)
            if kind == "class" and pending_template:
                kind = "template_class"
            stack.append(kind)
            pending_template = False
            stmt_start = i + 1
            i += 1
            continue
        if c == "}":
            if stack:
                stack.pop()
            stmt_start = i + 1
            i += 1
            continue
        if c == ";":
            if not pending_template and all(k == "namespace" for k in stack):
                edit = free_function_edit(masked, text, stmt_start, i)
                if edit and API not in masked[stmt_start:i]:
                    edits.append(edit)
            pending_template = False
            stmt_start = i + 1
            i += 1
            continue
        if masked.startswith("template", i) and re.match(r"template\s*<", masked[i:i + 12]):
            # template<...> を読み飛ばし、次の class/struct をテンプレートとして扱う
            depth = 0
            j = masked.find("<", i)
            while j < n:
                if masked[j] == "<":
                    depth += 1
                elif masked[j] == ">":
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            pending_template = True
            i = j + 1
            continue
        m = DECL.match(masked, i) if c in "cs" else None
        if m and (i == 0 or not (masked[i - 1].isalnum() or masked[i - 1] == "_")):
            before = masked[max(0, i - 40):i]
            is_enum = re.search(r"\benum\s*$", before) is not None
            is_friend = re.search(r"\bfriend\s*$", before) is not None
            in_scope = all(k in ("namespace", "class") for k in stack)
            already = m.group(2) is not None  # NANAMI_API 済み、または NANAMI_NO_API (export しない印)
            if not (is_enum or is_friend or pending_template or already) and in_scope:
                edits.append(Edit(m.end(1), m.group(3), text.count("\n", 0, i) + 1))
            if pending_template and not is_friend:
                pass  # `{` でスタックに template_class として積まれる
            i = m.end(1)
            continue
        i += 1
    return edits


def apply(text: str, edits: list[Edit]) -> str:
    out = text
    for e in sorted(edits, key=lambda e: e.offset, reverse=True):
        out = out[:e.offset] + e.insert + out[e.offset:]
    if API_INCLUDE not in out and "NanamiApi.h" not in out:
        nl = "\r\n" if "\r\n" in out else "\n"
        m = re.search(r"#pragma\s+once[^\n]*\n", out)
        if m:
            out = out[:m.end()] + API_INCLUDE + nl + out[m.end():]
        else:
            bom = "﻿" if out.startswith("﻿") else ""
            out = bom + API_INCLUDE + nl + out[len(bom):]
    return out


def iter_headers() -> list[Path]:
    files: list[Path] = []
    for root in ROOTS:
        files.extend(sorted((REPO / root).rglob("*.h")))
    return [f for f in files if f.relative_to(REPO).as_posix() not in SKIP_FILES]


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--verbose", "-v", action="store_true")
    args = parser.parse_args(argv)

    total_files = total_edits = 0
    for path in iter_headers():
        raw = path.read_bytes()
        bom = raw.startswith(b"\xef\xbb\xbf")
        text = raw.decode("utf-8-sig")
        edits = find_edits(text)
        if not edits:
            continue
        total_files += 1
        total_edits += len(edits)
        rel = path.relative_to(REPO).as_posix()
        if args.verbose or args.dry_run or args.check:
            for e in edits:
                print(f"{rel}:{e.line}: {e.name}")
        if not (args.dry_run or args.check):
            new = apply(text, edits)
            path.write_bytes((b"\xef\xbb\xbf" if bom else b"") + new.encode("utf-8"))
    print(f"{total_edits} declaration(s) in {total_files} file(s)" + (" (not written)" if args.dry_run or args.check else ""))
    return 1 if (args.check and total_edits) else 0


if __name__ == "__main__":
    sys.exit(main())
