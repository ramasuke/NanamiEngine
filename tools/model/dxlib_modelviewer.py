"""``DxLibModelViewer_64bit.exe`` の GUI を ``pywinauto`` で操作して、CLI/CUI の無い
``.fbx`` -> ``.mv1`` 変換を行う。

**2026-09-12 に DxLibModelViewer ver3.24d で確認済み** (タイトルバーは
``DxLibModelViewer [ DxLib ver3.24d ]``)。確認方法は 2 通り: 配布済みの実
``.mv1`` の往復 (Open -> メッシュのみ名前を付けて保存 -> ``MV11`` ヘッダを再確認) と、
PBR テクスチャ埋め込みの ~36MB の実 ``.fbx`` を ``python -m tools.model convert`` で
最初から最後まで変換 (出力: 妥当なサイズの ``MV11`` ヘッダの ``.mv1``)。
2026-09-13 に同じテクスチャ付き・アニメーション付きの ``.fbx`` で 3 つの保存モード
すべて (mesh / anim / full、``SAVE_MODES`` 参照) を再確認: 出力をデコード
(``tools/model/mv1.py``) すると、メッシュのみ = テクスチャ参照ありクリップ名なし、
アニメのみ = クリップ名ありテクスチャ参照なし、full = 両方。将来の
DxLibModelViewer のビルドでメニューのコマンド id やダイアログのコントロール id が
変わったら、このモジュールの元になった調査をやり直すこと:

    from pywinauto import Application
    app = Application(backend="win32").start(r"...\\DxLibModelViewer_64bit.exe")
    dlg = app.window(title_re=".*DxLibModelViewer.*")
    dlg.menu().items()[0].sub_menu().items()  # File submenu: text()/item_id()/state()

元にした調査結果:
  - アプリは本物の ``HMENU`` メニューバーを持つ素のネイティブ Win32 ダイアログで、
    pywinauto の **win32** バックエンドの ``menu()`` API で完全に操作できる
    (``backend="uia"`` でもメニューバーは読めるが、このアプリでは ``uia`` レベルの
    ``click_input()`` でネイティブのポップアップが開かない - ``win32`` を使うこと)。
  - ファイルメニューのコマンド id (位置インデックスと違い、ロケール/テキストが
    変わっても安定。2026-09-13 にファイルメニュー全体をダンプ): Open = 2、
    「名前を付けて保存」(メッシュ + アニメーション) = 5、「名前を付けて
    メッシュのみ保存」= 6、「名前を付けてアニメーション
    のみ保存」= 7。(3 = アニメーションの追加読み込み、4 = 上書き保存、8 = 追加読み込み
    したアニメーションと統合してメッシュを保存、1 = 終了 - ここでは未使用。)
    ``MenuItemWrapper.click()`` はコマンドを直接送る - ポップアップを実際に表示する
    必要は無く、項目が古い無効状態の ``state()`` を返していても動く (後述)。
  - Open/名前を付けて保存 はどちらも標準のモダンな Windows コモンダイアログ
    (クラス ``#32770``) を通る。Open/Save/OK ボタンは常に control_id **1**
    (IDOK)、Cancel は **2**。ただしファイル名エディットの control_id は
    **ダイアログによって違う**: 素の「開く」ダイアログは **1148** だが、このアプリの
    独自タイトルの名前を付けて保存ダイアログは **1001** - 両方試す。
  - 3 つの名前を付けて保存ダイアログ (id 5/6/7) はどれも同じファイル種別コンボを持ち、
    項目は ``['MV1 File(*.MV1)', 'X File(*.x)']`` で MV1 が選択済み。それでも
    明示的に選び直す (``_select_mv1_file_type``) ので、将来のビルドが ``.x`` を
    既定にしても黙って間違った形式を出力することは無い。
  - 「読み込み完了」は **メインウィンドウのタイトルが**
    "DxLibModelViewer [ DxLib ver3.24d ]" から読み込んだファイル自身の名前 (例:
    "Cube.mv1") に **変わること** で検出する - 単純で確実。ファイルサブメニューの項目の
    ``state()`` (グレーアウト/有効) は単独では **信頼できない** シグナル: ポップアップを
    少なくとも一度実際に表示するまで古い値を返すことがあるので、このモジュールは
    それを条件にしない。
"""

from __future__ import annotations

import tempfile
import time
from pathlib import Path

_POLL_INTERVAL = 0.5

# 標準の Windows コモンダイアログ (クラス "#32770") のコントロール id。
_FILENAME_EDIT_IDS = (1148, 1001)  # 開くダイアログは 1148、このアプリの名前を付けて保存ダイアログは 1001
_OK_BUTTON_ID = 1                  # IDOK - どの標準コモンダイアログでも「開く」/「保存」
_OVERWRITE_YES_ID = 6              # 標準の上書き確認ダイアログの IDYES
_FILENAME_SETTLE_SECS = 0.5        # 入力したパスがこの時間の後もまだ読み返せること

# DxLibModelViewer ver3.24d のファイルメニューのコマンド id - モジュール docstring 参照。
_MENU_ID_OPEN = 2
_MENU_ID_SAVE_AS = 5
_MENU_ID_SAVE_MESH_ONLY = 6
_MENU_ID_SAVE_ANIM_ONLY = 7

# convert() の ``mode`` -> (ファイルメニューのコマンド id, エラー用の人が読める名前)。
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
    """できる範囲の失敗時の記録: 全トップレベルウィンドウのスクリーンショットと
    コントロールツリーのテキストダンプ - GUI 自動化の失敗には見せられる終了コードや
    キャプチャした出力が無いので、サブプロセスの stdout/stderr の代わりになる
    このツールキットの手段。このモジュールが使う win32 バックエンドのラッパーには
    ``print_control_identifiers()`` が無いので、自前の ``_dump_tree`` を使う。"""
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
    """win32 の MenuWrapper ツリーを再帰的に探して ``item_id()`` が一致する項目を返す
    - 位置インデックスと違い、並べ替えや再ローカライズに強い。"""
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
    """ダイアログのファイル名 Edit が今存在して使えるならそれ、無ければ
    ``None`` (複数の id を試す理由は ``_FILENAME_EDIT_IDS`` 参照)。"""
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
    """標準の開く/保存コモンダイアログのファイル名欄に絶対パスを直接入力し、
    フォルダ移動を完全に省く - 最後に使ったフォルダや表示モードに関係なく操作できる
    最も確実な方法。``timeout`` まで、既知のファイル名欄の id をまとめてポーリングする:
    シェルのダイアログは子の生成に数秒かかることがあり (2026-09-13 に観測)、
    id ごとに固定の短い待ち時間を取る方式では足りなかった。

    欄が短い安定待ちの間ずっと ``path`` を読み返せてから初めて戻る: ダイアログは
    テキスト設定の *後に* 初期化を終えて欄を既定値 ("<読み込んだモデル>.mv1") に
    戻すことがあり、そのまま確定するとダイアログの最後に使ったフォルダにその既定名で
    保存してしまう - 2026-09-13 に余計な ``Assets/Art/Models/Basic/Hyena.mv1`` が
    書かれたのはこれが原因。``_confirm_common_dialog`` はクリックのたびに再確認する。"""
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
    """名前を付けて保存ダイアログのファイル種別コンボが "X File(*.x)" ではなく
    "MV1 File(*.MV1)" になっていることを確実にする。コンボには安定した
    コントロール id が無い (0 になる) ので、項目のテキストで探す。"""
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
    """合成したマウスクリック (``click_input()``) ではなく ``BM_CLICK`` メッセージ
    (``click()``) で IDOK を押す: 前者は画面座標に落ちるので、他のウィンドウが
    ダイアログに重なっていると黙ってそちらをクリックしてしまう - 2026-09-13 に
    ブラウザが保存ダイアログを覆っていたときに観測。

    シェルのダイアログがまだ落ち着いていない間に送った ``BM_CLICK`` は捨てられる
    ことがある (2026-09-13、開くダイアログがパスを入力したまま残った) ので、
    ダイアログが実際に閉じるまで送り直す。ダイアログ自体が無効になったら送り直しを
    やめる - それは独自のモーダルメッセージボックス (例: 「フォルダが存在しません」)
    を開いたということで、後の待機がそれを報告する。

    クリックの前には毎回ファイル名欄を読み直す: もう ``path`` を保持していなければ
    (``_set_common_dialog_filename`` 参照)、まず入力し直して再確認し、その回の
    クリックは飛ばす - 直前にファイル名を確認していないダイアログは決して確定しない。"""
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
    """保存がそこを通る場合の、標準の Windows の「<file> は既に存在します。
    置き換えますか?」確認。念のための多重防御にすぎない - ``cli.py`` の
    ``cmd_convert`` はまさにこれにほぼ当たらないよう、既存の出力ファイルを先に
    削除している。"""
    try:
        popup = app.window(class_name="#32770", title_re=".*")
        popup.wait("exists", timeout=timeout)
        yes = popup.child_window(control_id=_OVERWRITE_YES_ID, class_name="Button")
        if yes.exists():
            yes.click()  # BM_CLICK。_confirm_common_dialog 参照
    except Exception:  # noqa: BLE001
        pass


def _wait_for_new_dialog(app, known_handles: set[int], timeout: float):
    """``known_handles`` に無かった新しいトップレベルの ``#32770`` コモンダイアログ
    ウィンドウをポーリングで待つ - 開く/保存を呼んだ直後に使うので、タイトルが何かは
    気にしない (ロケール非依存)。タイトルがあり表示されていることだけを見る:
    作られたばかりでまだ無題/非表示のダイアログは構築中で、ファイル名欄も
    まだ無い。"""
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
    """GUI の状態は信用せずファイルシステムをポーリングする: ``mv1_path`` が
    存在するのを待ち、次にそのサイズが 2 回のチェックで変わらなくなるのを待つ -
    UI が暇そうに見えてもアプリはまだディスクへ書き出し中かもしれないので、
    手に入る中で最も信頼できる「書き込み完了」のシグナル。"""
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
    """DxLibModelViewer を起動して ``fbx_path`` を読み込み、``mode``
    (:data:`SAVE_MODES` のキー) で選んだファイルメニューの項目で ``mv1_path`` として保存する:

    * ``"mesh"`` - 「メッシュのみ名前を付けて保存」: 形状/マテリアル。アニメーションは捨てる。
    * ``"anim"`` - 「アニメーションのみ名前を付けて保存」: アニメーションクリップのみで
      メッシュ無し - 同じスケルトンのモデル間で共有するクリップファイル用。
    * ``"full"`` - 「名前を付けて保存」: メッシュとアニメーションを 1 ファイルにまとめる。

    失敗時は :class:`AutomationError` を投げ、できる範囲のデバッグ一式
    (全ウィンドウのスクリーンショット + コントロールツリーのダンプ) を ``debug_dir``
    (既定: 新しい一時ディレクトリ。パスは常にエラーに付く) に保存する。呼び出しの間は
    本物のウィンドウが作られる - ヘッドレスな処理ではない。``tools/model/README.md``
    を参照。
    """
    if mode not in SAVE_MODES:
        raise ValueError(f"unknown save mode {mode!r}; expected one of {sorted(SAVE_MODES)}")
    save_menu_id, save_menu_name = SAVE_MODES[mode]

    from pywinauto import Application  # 遅延: pywinauto が要るのは convert() だけ

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

        # --- 開く ---
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

        # --- 読み込み待ち (メインウィンドウのタイトルが読み込んだファイル名に変わる) ---
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

        # --- 保存 (メニュー項目は mode で選ぶ) ---
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
