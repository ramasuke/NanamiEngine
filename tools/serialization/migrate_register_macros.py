"""cereal の多相登録マクロを NanamiEngine のマクロに置き換える (docs/HotReload.md 段階 A)。

    CEREAL_REGISTER_TYPE(T)                              -> NANAMI_REGISTER_TYPE(T, Base);
    CEREAL_REGISTER_POLYMORPHIC_RELATION(Base, T)  (対)  -> (NANAMI_REGISTER_TYPE に畳み込んで削除)
    CEREAL_REGISTER_POLYMORPHIC_RELATION(Base, T)  (残り) -> NANAMI_REGISTER_POLYMORPHIC_RELATION(Base, T);

同じファイルの中で、TYPE(T) と「派生が T の最初の RELATION」を 1 組にする。組にならなかった RELATION
(2 つ目以降の基底、中間基底) はそのまま新しい RELATION マクロにする。複数行に分かれた引数は 1 行にまとめる。
型名のトークン列 (= 保存ファイルの polymorphic_name) は 1 文字も変えない。

BOM と改行コードは元のまま保つ。置き換え後に再解析して、型の集合と (基底, 派生) の集合が
置き換え前と同じであることを確かめる。

    python tools/serialization/migrate_register_macros.py [--dry-run] [--verbose] [DIR ...]

DIR を省略すると Engine Packages Libs/LibCore Assets の .cpp を対象にする。マクロを定義しているヘッダ
(ComponentBase.h / AttackArea.h / PlayerAvatarBase.h) は手で直す。
"""
from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

BOM = b"\xef\xbb\xbf"
RE_TYPE = re.compile(r"\b(NANAMI|CEREAL)_REGISTER_TYPE\s*\(")
RE_RELATION = re.compile(r"\b(NANAMI|CEREAL)_REGISTER_POLYMORPHIC_RELATION\s*\(")


@dataclass
class Invocation:
    kind: str          # "TYPE" | "RELATION"
    prefix: str        # "NANAMI" | "CEREAL"
    start: int         # マクロ名の先頭
    end: int           # 閉じ括弧 (と直後の ;) の次
    args: list[str]    # 引数 (前後の空白を除き、内部の空白は 1 個に)


def _norm(text: str) -> str:
    return re.sub(r"\s+", "", text)


def _collapse(text: str) -> str:
    return re.sub(r"\s+", " ", text).strip()


def _split_args(inner: str) -> list[str]:
    """テンプレートの < > の中のカンマでは区切らない。"""
    args, depth, current = [], 0, []
    for ch in inner:
        if ch == "<":
            depth += 1
        elif ch == ">":
            depth -= 1
        if ch == "," and depth == 0:
            args.append("".join(current))
            current = []
        else:
            current.append(ch)
    args.append("".join(current))
    return [_collapse(a) for a in args]


def _scan(text: str) -> list[Invocation]:
    found: list[Invocation] = []
    for kind, regex in (("TYPE", RE_TYPE), ("RELATION", RE_RELATION)):
        for m in regex.finditer(text):
            # #define の中 (行継続 \ で終わる行) は対象外
            line_start = text.rfind("\n", 0, m.start()) + 1
            if text[line_start:m.start()].lstrip().startswith("#define"):
                continue
            open_idx = m.end() - 1
            depth, i = 0, open_idx
            while i < len(text):
                if text[i] == "(":
                    depth += 1
                elif text[i] == ")":
                    depth -= 1
                    if depth == 0:
                        break
                i += 1
            if depth != 0:
                raise ValueError(f"unbalanced parentheses at offset {m.start()}")
            close_idx = i
            end = close_idx + 1
            tail = re.match(r"[ \t]*;", text[end:])
            if tail:
                end += tail.end()
            found.append(Invocation(kind, m.group(1), m.start(), end, _split_args(text[open_idx + 1:close_idx])))
    found.sort(key=lambda inv: inv.start)
    return found


def _summary(invocations: list[Invocation]) -> tuple[set[str], list[tuple[str, str]]]:
    """(型の集合, (基底, 派生) の多重集合)。NANAMI_REGISTER_TYPE(T, Base) は関係 (Base, T) も含む。"""
    types: set[str] = set()
    relations: list[tuple[str, str]] = []
    for inv in invocations:
        if inv.kind == "TYPE":
            types.add(_norm(inv.args[0]))
            if inv.prefix == "NANAMI":
                relations.append((_norm(inv.args[1]), _norm(inv.args[0])))
        else:
            relations.append((_norm(inv.args[0]), _norm(inv.args[1])))
    return types, sorted(relations)


def _delete_span(text: str, start: int, end: int) -> str:
    """span を消す。その行が空白だけになったら行ごと消す。"""
    line_start = text.rfind("\n", 0, start) + 1
    line_end = text.find("\n", end)
    line_end = len(text) if line_end == -1 else line_end + 1
    remainder = text[line_start:start] + text[end:line_end]
    if remainder.strip() == "":
        return text[:line_start] + text[line_end:]
    return text[:start] + text[end:]


def migrate_text(text: str) -> tuple[str, int]:
    invocations = _scan(text)
    cereal = [inv for inv in invocations if inv.prefix == "CEREAL"]
    if not cereal:
        return text, 0
    before = _summary(invocations)

    relations = [inv for inv in cereal if inv.kind == "RELATION"]
    consumed: set[int] = set()
    replacements: list[tuple[int, int, str | None]] = []  # (start, end, new text | None = delete)
    for inv in cereal:
        if inv.kind != "TYPE":
            continue
        derived = _norm(inv.args[0])
        pair = next((r for r in relations if id(r) not in consumed and _norm(r.args[1]) == derived), None)
        if pair is None:
            raise ValueError(f"CEREAL_REGISTER_TYPE({inv.args[0]}) has no CEREAL_REGISTER_POLYMORPHIC_RELATION in the same file")
        consumed.add(id(pair))
        replacements.append((inv.start, inv.end, f"NANAMI_REGISTER_TYPE({inv.args[0]}, {pair.args[0]});"))
        replacements.append((pair.start, pair.end, None))
    for r in relations:
        if id(r) not in consumed:
            replacements.append((r.start, r.end, f"NANAMI_REGISTER_POLYMORPHIC_RELATION({r.args[0]}, {r.args[1]});"))

    for start, end, new in sorted(replacements, key=lambda t: t[0], reverse=True):
        text = _delete_span(text, start, end) if new is None else text[:start] + new + text[end:]

    after = _summary(_scan(text))
    if before != after:
        raise ValueError(f"registration set changed: before={before} after={after}")
    if any(inv.prefix == "CEREAL" for inv in _scan(text)):
        raise ValueError("CEREAL_ macro left behind")
    return text, len(replacements)


def migrate_file(path: Path, dry_run: bool) -> int:
    raw = path.read_bytes()
    bom = raw.startswith(BOM)
    body = raw[len(BOM):] if bom else raw
    try:
        text = body.decode("utf-8")
    except UnicodeDecodeError:
        print(f"SKIP (not UTF-8): {path}", file=sys.stderr)
        return 0
    new_text, count = migrate_text(text)
    if count and not dry_run:
        path.write_bytes((BOM if bom else b"") + new_text.encode("utf-8"))
    return count


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("dirs", nargs="*", default=["Engine", "Packages", "Libs/LibCore", "Assets"])
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args(argv)

    root = Path(__file__).resolve().parents[2]
    files_changed = total = 0
    for d in args.dirs:
        for path in sorted((root / d).rglob("*.cpp")):
            count = migrate_file(path, args.dry_run)
            if count:
                files_changed += 1
                total += count
                if args.verbose:
                    print(f"{count:3d}  {path.relative_to(root)}")
    print(f"{'would change' if args.dry_run else 'changed'} {files_changed} files, {total} macro sites")
    return 0


if __name__ == "__main__":
    sys.exit(main())
