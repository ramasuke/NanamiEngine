"""Byte-safe splices into ``NanamiEngine.vcxproj`` / ``.vcxproj.filters``.

These files are ~1 MB, UTF-8 **with BOM**, **CRLF**, hand-maintained, and use the
default (unprefixed) MSBuild namespace. Round-tripping them through an XML library
reflows every line and rewrites the namespace, so we edit them as text: locate a
unique anchor, insert one element, re-add the BOM, keep CRLF. After writing we
re-parse (parse only) and assert the element counts moved by exactly the expected
delta; on any failure the original bytes are restored.

Format-generic - callers pass their own ``anchor`` (a substring of an existing
``Include="..."`` path unique enough to anchor a regex search); nothing here
knows about BehaviourTree actions, Components, or any other consumer's content.
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
    filter: str = ""     # for .filters only: "Source Files" / "Header Files"


def _read(path: Path) -> tuple[bytes, str]:
    raw = path.read_bytes()
    if raw[:3] != b"\xef\xbb\xbf":
        raise VcxprojError(f"{path.name}: expected a UTF-8 BOM")
    return raw, raw.decode("utf-8-sig")


def _write(path: Path, text: str) -> None:
    path.write_bytes(b"\xef\xbb\xbf" + text.encode("utf-8"))


def _element_span(text: str, start: int) -> int:
    """Given the index of a ``<Tag`` opener, return the index just past its end
    (handles both ``<Tag ... />`` and ``<Tag ...>...</Tag>``)."""
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
    # keep indentation: copy the leading whitespace of the anchor line
    line_start = text.rfind("\n", 0, matches[-1].start()) + 1
    indent = text[line_start:matches[-1].start()]
    return text[:end] + "\r\n" + indent + snippet + text[end:]


def _count(text: str, tag: str) -> int:
    return len(re.findall(rf"<{tag}\b", text))


def apply_splices(vcxproj: Path, filters: Path | None, splices: list[Splice],
                  *, anchor: str, dry_run: bool = False) -> list[str]:
    """Apply ClCompile/ClInclude splices to the project (and optionally filters).

    ``anchor`` is a substring of an existing ``Include="..."`` path; the new
    element is inserted right after the *last* element whose Include starts
    with it.
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
    """Remove ClCompile/ClInclude entries for the given paths (reverse of add)."""
    log: list[str] = []
    for path in (vcxproj, filters):
        if path is None:
            continue
        raw, text = _read(path)
        new = text
        for wp in win_paths:
            # match a whole element line/block for this Include, plus its trailing CRLF
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
