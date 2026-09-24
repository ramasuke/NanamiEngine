"""``EnviroHunter.vcxproj`` / ``.vcxproj.filters`` へのバイト安全な差し込み。

これらのファイルは約 1 MB、UTF-8 **BOM 付き**、**CRLF**、手作業で保守され、既定の
(プレフィックスなしの) MSBuild 名前空間を使う。XML ライブラリで往復させると
全行が整形し直され名前空間も書き換わるため、テキストとして編集する: 一意の
アンカーを探し、要素を1つ挿入し、BOM を付け直し、CRLF を保つ。書き込み後に
再パース (パースのみ) し、要素数がちょうど期待した差分だけ変わったことを検証する。
失敗時は元のバイト列を復元する。

書式に対して汎用 - 呼び出し側が独自の ``anchor`` (正規表現検索のアンカーとして
十分に一意な、既存の ``Include="..."`` パスの部分文字列) を渡す。ここでは
BehaviourTree のアクションや Component など、利用側の内容については何も知らない。
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path
from xml.dom import minidom


class VcxprojError(RuntimeError):
    pass


@dataclass
class Splice:
    kind: str            # "ClCompile" | "ClInclude"
    win_path: str        # Assets\Scripts\...\Foo.cpp
    filter: str = ""     # .filters 専用: "Source Files" / "Header Files"


def _read(path: Path) -> tuple[bytes, str]:
    raw = path.read_bytes()
    if raw[:3] != b"\xef\xbb\xbf":
        raise VcxprojError(f"{path.name}: expected a UTF-8 BOM")
    return raw, raw.decode("utf-8-sig")


def _write(path: Path, text: str) -> None:
    path.write_bytes(b"\xef\xbb\xbf" + text.encode("utf-8"))


def _element_span(text: str, start: int) -> int:
    """``<Tag`` の開始位置のインデックスを受け取り、その要素の終端直後のインデックスを返す
    (``<Tag ... />`` と ``<Tag ...>...</Tag>`` の両方に対応)。"""
    gt = text.index(">", start)
    if text[gt - 1] == "/":
        return gt + 1
    tag = re.match(r"<(\w+)", text[start:]).group(1)
    close = text.index(f"</{tag}>", gt)
    return close + len(f"</{tag}>")


def _insert_after_last(text: str, opener_re: re.Pattern, snippet: str) -> str:
    matches = list(opener_re.finditer(text))
    if not matches:
        raise VcxprojError("anchor element not found")
    end = _element_span(text, matches[-1].start())
    # インデントを保つ: アンカー行の先頭空白をコピーする
    line_start = text.rfind("\n", 0, matches[-1].start()) + 1
    indent = text[line_start:matches[-1].start()]
    return text[:end] + "\r\n" + indent + snippet + text[end:]


def _count(text: str, tag: str) -> int:
    return len(re.findall(rf"<{tag}\b", text))


def apply_splices(vcxproj: Path, filters: Path | None, splices: list[Splice],
                  *, anchor: str, dry_run: bool = False) -> list[str]:
    """プロジェクト (および任意で filters) に ClCompile/ClInclude の差し込みを適用する。

    ``anchor`` は既存の ``Include="..."`` パスの部分文字列。新しい要素は、
    Include がそれで始まる *最後の* 要素の直後に挿入される。
    """
    log: list[str] = []
    raw, text = _read(vcxproj)
    new = text
    for sp in splices:
        if f'Include="{sp.win_path}"' in new:
            log.append(f"vcxproj: {sp.win_path} already present - skipped")
            continue
        opener = re.compile(
            re.escape(f'<{sp.kind} Include="') + re.escape(anchor) + r'[^"]*"'
        )
        snippet = f'<{sp.kind} Include="{sp.win_path}" />'
        new = _insert_after_last(new, opener, snippet)
        log.append(f"vcxproj: + <{sp.kind} Include=\"{sp.win_path}\" />")

    if new != text:
        _verify(text, new, splices)
        if not dry_run:
            _write(vcxproj, new)

    if filters is not None:
        flog = _apply_filters(filters, splices, anchor=anchor, dry_run=dry_run)
        log.extend(flog)
    return log


def _verify(old: str, new: str, splices: list[Splice]) -> None:
    try:
        minidom.parseString(new)
    except Exception as e:  # noqa: BLE001
        raise VcxprojError(f"result is not well-formed XML: {e}") from e
    for tag in ("ClCompile", "ClInclude"):
        want = _count(old, tag) + sum(1 for s in splices if s.kind == tag
                                      and f'Include="{s.win_path}"' not in old)
        got = _count(new, tag)
        if got != want:
            raise VcxprojError(f"<{tag}> count {got} != expected {want} - aborting, no write")


def _apply_filters(filters: Path, splices: list[Splice], *, anchor: str,
                   dry_run: bool) -> list[str]:
    log: list[str] = []
    raw, text = _read(filters)
    new = text
    for sp in splices:
        if f'Include="{sp.win_path}"' in new:
            log.append(f"filters: {sp.win_path} already present - skipped")
            continue
        flt = sp.filter or ("Source Files" if sp.kind == "ClCompile" else "Header Files")
        opener = re.compile(
            re.escape(f'<{sp.kind} Include="') + re.escape(anchor) + r'[^"]*"'
        )
        snippet = (f'<{sp.kind} Include="{sp.win_path}">\r\n'
                   f'      <Filter>{flt}</Filter>\r\n'
                   f'    </{sp.kind}>')
        try:
            new = _insert_after_last(new, opener, snippet)
            log.append(f"filters: + <{sp.kind} Include=\"{sp.win_path}\"> ({flt})")
        except VcxprojError:
            log.append(f"filters: no anchor for {sp.kind}; skipped (not build-critical)")
    if new != text:
        try:
            minidom.parseString(new)
        except Exception as e:  # noqa: BLE001
            raise VcxprojError(f"filters result not well-formed: {e}") from e
        if not dry_run:
            _write(filters, new)
    return log


def remove_splices(vcxproj: Path, filters: Path | None, win_paths: list[str],
                   *, dry_run: bool = False) -> list[str]:
    """指定パスの ClCompile/ClInclude エントリを削除する (add の逆)。"""
    log: list[str] = []
    for path in (vcxproj, filters):
        if path is None:
            continue
        raw, text = _read(path)
        new = text
        for wp in win_paths:
            # この Include の要素行/ブロック全体と、その後ろの CRLF にマッチさせる
            pat = re.compile(
                r"[ \t]*<(ClCompile|ClInclude) Include=\"" + re.escape(wp) + r"\""
                r"(?: />|>.*?</(?:ClCompile|ClInclude)>)\r\n",
                re.S,
            )
            new2 = pat.sub("", new, count=1)
            if new2 != new:
                log.append(f"{path.name}: - {wp}")
            new = new2
        if new != text and not dry_run:
            try:
                minidom.parseString(new)
            except Exception as e:  # noqa: BLE001
                raise VcxprojError(f"{path.name}: removal broke XML: {e}") from e
            _write(path, new)
    return log
