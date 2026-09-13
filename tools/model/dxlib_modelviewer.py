"""Drives ``DxLibModelViewer_64bit.exe``'s GUI via ``pywinauto`` to perform
the ``.fbx`` -> ``.mv1`` conversion it has no CLI/CUI for.

**Verified 2026-09-12 against DxLibModelViewer ver3.24d** (title bar reads
``DxLibModelViewer [ DxLib ver3.24d ]``), two ways: round-tripping a real
shipped ``.mv1`` (Open -> Save As mesh only -> re-verify ``MV11`` header), and
converting a real ~36MB ``.fbx`` with embedded PBR textures end to end via
``python -m tools.model convert`` (output: a plausible-sized ``MV11``-header
``.mv1``). Re-verified 2026-09-13 in all three save modes (mesh / anim /
full, see ``SAVE_MODES``) on the same textured, animated ``.fbx``: the outputs
decode (``tools/model/mv1.py``) to mesh-only = texture refs and no clip names,
anim-only = clip names and no texture refs, full = both. If a future
DxLibModelViewer build changes menu command ids or
dialog control ids, re-run the inspection this module was built from:

    from pywinauto import Application
    app = Application(backend="win32").start(r"...\\DxLibModelViewer_64bit.exe")
    dlg = app.window(title_re=".*DxLibModelViewer.*")
    dlg.menu().items()[0].sub_menu().items()  # File submenu: text()/item_id()/state()

Findings this was built from:
  - The app is a plain native Win32 dialog with a real ``HMENU`` menu bar -
    fully driveable via pywinauto's **win32** backend's ``menu()`` API
    (``backend="uia"`` can read the menu bar but its native popup does not
    open on a ``uia``-level ``click_input()`` in this app - use ``win32``).
  - File menu command ids (stable across a locale/text change, unlike a
    positional index; full File menu dumped 2026-09-13): Open = 2, "Save As"
    ("名前を付けて保存", mesh + animation) = 5, "Save As mesh only" ("名前を付けて
    メッシュのみ保存") = 6, "Save As animation only" ("名前を付けてアニメーション
    のみ保存") = 7. (3 = add-load animation, 4 = overwrite save, 8 = save mesh
    merged with add-loaded animation, 1 = exit - unused here.) A
    ``MenuItemWrapper.click()`` sends the command directly - no need to
    actually show the popup, and works even while the item still reports a
    stale disabled ``state()`` (see below).
  - Open/Save As both go through the standard modern Windows common dialog
    (class ``#32770``). Its Open/Save/OK button is always control_id **1**
    (IDOK), Cancel is **2**. The filename edit's control_id, however,
    **differs by dialog**: the plain "Open" dialog uses **1148**, but this
    app's custom-titled Save As dialogs use **1001** - try both.
  - All three Save As dialogs (ids 5/6/7) carry the same file-type combo,
    items ``['MV1 File(*.MV1)', 'X File(*.x)']`` with MV1 preselected. It is
    re-selected explicitly anyway (``_select_mv1_file_type``) so a future
    build defaulting to ``.x`` can't silently produce the wrong format.
  - "Load finished" is detected by the **main window's title changing** from
    "DxLibModelViewer [ DxLib ver3.24d ]" to the loaded file's own name (e.g.
    "Cube.mv1") - simple and reliable. The File submenu's item ``state()``
    (grayed/enabled) is **not** a trustworthy signal on its own: it can read
    stale until the popup has actually been shown at least once, so this
    module doesn't gate on it.
"""

from __future__ import annotations

import tempfile
import time
from pathlib import Path

_POLL_INTERVAL = 0.5

# Standard Windows common-dialog (class "#32770") control ids.
_FILENAME_EDIT_IDS = (1148, 1001)  # Open dialog uses 1148; this app's Save As dialogs use 1001
_OK_BUTTON_ID = 1                  # IDOK - "Open"/"Save" on every standard common dialog
_OVERWRITE_YES_ID = 6              # IDYES on the standard overwrite-confirm dialog
_FILENAME_SETTLE_SECS = 0.5        # the typed path must still read back after this long

# DxLibModelViewer ver3.24d's File menu command ids - see module docstring.
_MENU_ID_OPEN = 2
_MENU_ID_SAVE_AS = 5
_MENU_ID_SAVE_MESH_ONLY = 6
_MENU_ID_SAVE_ANIM_ONLY = 7

# convert()'s ``mode`` -> (File menu command id, human-readable name for errors).
SAVE_MODES: dict[str, tuple[int, str]] = {
    "mesh": (_MENU_ID_SAVE_MESH_ONLY, "Save As mesh only"),
    "anim": (_MENU_ID_SAVE_ANIM_ONLY, "Save As animation only"),
    "full": (_MENU_ID_SAVE_AS, "Save As (mesh + animation)"),
}

_MV1_FILE_TYPE_PREFIX = "MV1"


class AutomationError(RuntimeError):
    def __init__(self, step: str, message: str, debug_path: Path | None = None) -> None:
        super().__init__(message)
        self.step = step
        self.debug_path = debug_path


def _dump_tree(ctrl, lines: list[str], depth: int = 0, max_depth: int = 6) -> None:
    if depth > max_depth:
        return
    pad = "  " * depth
    try:
        cls = ctrl.class_name()
    except Exception:  # noqa: BLE001
        cls = "?"
    try:
        cid = ctrl.control_id()
    except Exception:  # noqa: BLE001
        cid = "?"
    try:
        text = ctrl.window_text()
    except Exception:  # noqa: BLE001
        text = "?"
    lines.append(f"{pad}[{cls}] id={cid} text={text!r}")
    try:
        children = ctrl.children()
    except Exception:  # noqa: BLE001
        return
    for c in children:
        _dump_tree(c, lines, depth + 1, max_depth)


def _dump_debug(app, debug_dir: Path | None) -> Path | None:
    """Best-effort failure capture: a screenshot of every top-level window
    plus a control-tree text dump - this toolkit's substitute for a
    subprocess's stdout/stderr, since a GUI-automation failure has no exit
    code or captured output to show the user. ``print_control_identifiers()``
    isn't available on the win32-backend wrappers this module uses, hence
    the hand-rolled ``_dump_tree``."""
    if debug_dir is None:
        debug_dir = Path(tempfile.mkdtemp(prefix="tools_model_debug_"))
    debug_dir.mkdir(parents=True, exist_ok=True)
    try:
        windows = app.windows()
    except Exception:  # noqa: BLE001
        windows = []
    for w in windows:
        try:
            w.capture_as_image().save(str(debug_dir / f"screenshot_{w.handle}.png"))
        except Exception:  # noqa: BLE001
            pass
    try:
        lines: list[str] = []
        for w in windows:
            try:
                lines.append(f"=== window {w.handle}: {w.window_text()!r} class={w.class_name()!r} ===")
                _dump_tree(w, lines)
            except Exception:  # noqa: BLE001
                continue
        (debug_dir / "control_tree.txt").write_text("\n".join(lines), encoding="utf-8")
    except Exception:  # noqa: BLE001
        pass
    return debug_dir


def _find_menu_item(menu, target_id: int):
    """Recursively search a win32 MenuWrapper tree for the item whose
    ``item_id()`` matches - robust to reordering/relocalization, unlike a
    positional index."""
    for item in menu.items():
        if item.item_id() == target_id:
            return item
        sub = None
        for attr in ("sub_menu", "submenu"):
            if hasattr(item, attr):
                try:
                    sub = getattr(item, attr)()
                except Exception:  # noqa: BLE001
                    sub = None
                if sub is not None:
                    break
        if sub is not None:
            found = _find_menu_item(sub, target_id)
            if found is not None:
                return found
    return None


def _find_filename_edit(dlg):
    """The dialog's filename Edit if it currently exists and is usable, else
    ``None`` (see ``_FILENAME_EDIT_IDS`` for why several ids are tried)."""
    for cid in _FILENAME_EDIT_IDS:
        try:
            edit = dlg.child_window(control_id=cid, class_name="Edit")
            if edit.exists(timeout=0) and edit.is_visible() and edit.is_enabled():
                return edit
        except Exception:  # noqa: BLE001
            continue
    return None


def _filename_matches(edit, path: Path) -> bool:
    try:
        return edit.window_text() == str(path)
    except Exception:  # noqa: BLE001
        return False


def _set_common_dialog_filename(dlg, path: Path, timeout: float) -> None:
    """Type an absolute path directly into a standard Open/Save common
    dialog's filename field, bypassing folder navigation entirely - the most
    reliable way to drive it regardless of the last-used folder or view
    mode. Polls every known filename-field id together until ``timeout``:
    the shell dialog can take several seconds to populate its children
    (observed 2026-09-13), longer than a fixed short per-id wait allowed.

    Only returns once the field has read back ``path`` across a short settle
    period: the dialog can finish initializing *after* the text is set and
    reset the field to its default ("<loaded model>.mv1"), and confirming
    then saves that default name into the dialog's last-used folder - which
    is how a stray ``Assets/Art/Models/Basic/Hyena.mv1`` got written on
    2026-09-13. ``_confirm_common_dialog`` re-checks before every click."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        edit = _find_filename_edit(dlg)
        if edit is not None:
            try:
                if not _filename_matches(edit, path):
                    edit.set_edit_text(str(path))
                time.sleep(_FILENAME_SETTLE_SECS)
                if _filename_matches(edit, path):
                    return
            except Exception:  # noqa: BLE001 - control recreated mid-init; retry
                pass
        time.sleep(0.2)
    raise RuntimeError(f"could not set the filename field in dialog {dlg.window_text()!r} "
                       f"to {str(path)!r} within {timeout}s")


def _select_mv1_file_type(dlg) -> None:
    """Make sure the Save As dialog's file-type combo is on "MV1 File(*.MV1)"
    rather than "X File(*.x)". The combo has no stable control id (reads 0),
    so it's found by its item texts."""
    for combo in dlg.wrapper_object().descendants(class_name="ComboBox"):
        try:
            items = combo.item_texts()
        except Exception:  # noqa: BLE001
            continue
        for i, text in enumerate(items):
            if text.upper().startswith(_MV1_FILE_TYPE_PREFIX):
                if combo.selected_index() != i:
                    combo.select(i)
                return
    raise RuntimeError(f"no file-type combo with an {_MV1_FILE_TYPE_PREFIX!r} entry in dialog "
                       f"{dlg.window_text()!r}")


def _confirm_common_dialog(dlg, path: Path, timeout: float) -> None:
    """Press IDOK via a ``BM_CLICK`` message (``click()``), not a synthesized
    mouse click (``click_input()``): the latter lands at screen coordinates,
    so if any other window overlaps the dialog it silently clicks that window
    instead - observed 2026-09-13 when a browser covered the Save dialog.

    A ``BM_CLICK`` sent while the shell dialog is still settling is sometimes
    dropped (the Open dialog stayed up with its path filled in, 2026-09-13),
    so keep re-sending until the dialog actually closes. Stop re-sending once
    the dialog itself is disabled - that means it opened a modal message box
    of its own (e.g. "folder does not exist"), which a later wait reports.

    Every click is preceded by re-reading the filename field: if it no longer
    holds ``path`` (see ``_set_common_dialog_filename``), it is re-typed and
    re-verified first, and the click is skipped this round - never confirm a
    dialog whose filename we haven't just seen."""
    btn = dlg.child_window(control_id=_OK_BUTTON_ID, class_name="Button")
    btn.wait("exists enabled visible", timeout=10)
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            if not dlg.exists(timeout=0):
                return
            if not dlg.is_enabled():
                return
            edit = _find_filename_edit(dlg)
            if edit is None or not _filename_matches(edit, path):
                _set_common_dialog_filename(dlg, path, max(1.0, deadline - time.monotonic()))
                continue
            btn.click()
        except Exception:  # noqa: BLE001 - dialog closed between the checks
            if not dlg.exists(timeout=0):
                return
        settle = time.monotonic() + 2.0
        while time.monotonic() < settle:
            if not dlg.exists(timeout=0):
                return
            time.sleep(0.2)
    raise RuntimeError(f"dialog {dlg.window_text()!r} did not close after pressing OK for {timeout}s")


def _dismiss_overwrite_prompt_if_any(app, timeout: float = 2.0) -> None:
    """The standard Windows "<file> already exists. Do you want to replace
    it?" confirmation, if Save routes through it. Defense in depth only -
    ``cli.py``'s ``cmd_convert`` deletes any existing output file up front
    specifically so this is rarely reached."""
    try:
        popup = app.window(class_name="#32770", title_re=".*")
        popup.wait("exists", timeout=timeout)
        yes = popup.child_window(control_id=_OVERWRITE_YES_ID, class_name="Button")
        if yes.exists():
            yes.click()  # BM_CLICK, see _confirm_common_dialog
    except Exception:  # noqa: BLE001
        pass


def _wait_for_new_dialog(app, known_handles: set[int], timeout: float):
    """Poll for a new top-level ``#32770`` common-dialog window that wasn't
    present in ``known_handles`` - used right after invoking Open/Save so we
    don't care what its title is (locale-independent), only that it has one
    and is visible: a just-created dialog that's still untitled/hidden is
    still being constructed and has no filename field yet."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            windows = app.windows()
        except Exception:  # noqa: BLE001
            windows = []
        for w in windows:
            try:
                if (w.class_name() == "#32770" and w.handle not in known_handles
                        and w.is_visible() and w.window_text()):
                    return app.window(handle=w.handle)
            except Exception:  # noqa: BLE001
                continue
        time.sleep(0.2)
    return None


def _wait_for_output_settled(app, mv1_path: Path, timeout: float, debug_dir: Path | None) -> None:
    """Poll the filesystem rather than trusting any GUI state: wait for
    ``mv1_path`` to exist, then for its size to stop changing across two
    checks - the most trustworthy "write finished" signal available, since
    the app could still be flushing to disk after its UI looks idle."""
    deadline = time.monotonic() + timeout
    last_size = -1
    while time.monotonic() < deadline:
        if mv1_path.exists():
            size = mv1_path.stat().st_size
            if size > 0 and size == last_size:
                return
            last_size = size
        time.sleep(_POLL_INTERVAL)
    raise AutomationError(
        "wait-for-output",
        f"{mv1_path} did not appear or stabilize within {timeout}s",
        _dump_debug(app, debug_dir),
    )


def convert(fbx_path: Path, mv1_path: Path, exe_path: Path, *, mode: str,
            timeout: float = 60.0, debug_dir: Path | None = None) -> None:
    """Launch DxLibModelViewer, load ``fbx_path``, and save it as ``mv1_path``
    using the File menu entry selected by ``mode`` (a :data:`SAVE_MODES` key):

    * ``"mesh"`` - "Save As mesh only": geometry/materials, animations dropped.
    * ``"anim"`` - "Save As animation only": animation clips, no mesh - for
      clip files shared across models with the same skeleton.
    * ``"full"`` - "Save As": mesh and animations together in one file.

    Raises :class:`AutomationError` on any failure, with a best-effort debug
    bundle (screenshots + control-tree dump of every window) saved under
    ``debug_dir`` (default: a fresh temp directory, path always attached to
    the error). A real window is created for the duration of the call - this
    is not a headless operation, see ``tools/model/README.md``.
    """
    if mode not in SAVE_MODES:
        raise ValueError(f"unknown save mode {mode!r}; expected one of {sorted(SAVE_MODES)}")
    save_menu_id, save_menu_name = SAVE_MODES[mode]

    from pywinauto import Application  # lazy: only convert() needs pywinauto

    app = None
    try:
        app = Application(backend="win32").start(f'"{exe_path}"')
        try:
            main = app.window(title_re=r".*DxLibModelViewer.*")
            main.wait("exists enabled visible ready", timeout=timeout)
        except Exception as e:  # noqa: BLE001
            raise AutomationError("launch", f"DxLibModelViewer did not become ready: {e}",
                                   _dump_debug(app, debug_dir)) from e

        main_handle = main.handle
        orig_title = main.window_text()

        def by_handle():
            return app.window(handle=main_handle)

        # --- Open ---
        try:
            known = {w.handle for w in app.windows()}
            file_menu = by_handle().menu().items()[0].sub_menu()
            open_item = _find_menu_item(file_menu, _MENU_ID_OPEN)
            if open_item is None:
                raise RuntimeError(f"File menu has no item with the expected Open command id "
                                    f"({_MENU_ID_OPEN}) - DxLibModelViewer version mismatch?")
            open_item.click()
            open_dlg = _wait_for_new_dialog(app, known, timeout)
            if open_dlg is None:
                raise RuntimeError("no dialog appeared after invoking File > Open")
            _set_common_dialog_filename(open_dlg, fbx_path, timeout)
            _confirm_common_dialog(open_dlg, fbx_path, timeout)
        except AutomationError:
            raise
        except Exception as e:  # noqa: BLE001
            raise AutomationError("open", str(e), _dump_debug(app, debug_dir)) from e

        # --- wait for load (main window retitles to the loaded file's name) ---
        deadline = time.monotonic() + timeout
        loaded = False
        while time.monotonic() < deadline:
            try:
                if by_handle().window_text() != orig_title:
                    loaded = True
                    break
            except Exception:  # noqa: BLE001
                pass
            time.sleep(_POLL_INTERVAL)
        if not loaded:
            raise AutomationError(
                "open",
                f"main window title never changed from {orig_title!r} within {timeout}s "
                "- load likely failed, hung, or an error dialog appeared",
                _dump_debug(app, debug_dir),
            )

        # --- Save (menu entry chosen by mode) ---
        try:
            known = {w.handle for w in app.windows()}
            file_menu = by_handle().menu().items()[0].sub_menu()
            save_item = _find_menu_item(file_menu, save_menu_id)
            if save_item is None:
                raise RuntimeError(f"File menu has no item with the expected '{save_menu_name}' "
                                    f"command id ({save_menu_id}) - version mismatch?")
            save_item.click()
            save_dlg = _wait_for_new_dialog(app, known, timeout)
            if save_dlg is None:
                raise RuntimeError(f"no dialog appeared after invoking File > {save_menu_name}")
            _select_mv1_file_type(save_dlg)
            _set_common_dialog_filename(save_dlg, mv1_path, timeout)
            _confirm_common_dialog(save_dlg, mv1_path, timeout)
            _dismiss_overwrite_prompt_if_any(app)
        except AutomationError:
            raise
        except Exception as e:  # noqa: BLE001
            raise AutomationError("save", str(e), _dump_debug(app, debug_dir)) from e

        _wait_for_output_settled(app, Path(mv1_path), timeout, debug_dir)
    finally:
        if app is not None:
            try:
                app.kill()
            except Exception:  # noqa: BLE001
                pass
