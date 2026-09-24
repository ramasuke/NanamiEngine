"""フック: このプロジェクトの各 Claude Code セッションがターンの途中かどうかを記録する。

UserPromptSubmit で <state dir>/<session_id>.json（"busy"）を書き、Stop / SessionEnd でそれを消して
そのセッションが持つビルドロックを解放する。.claude/shared/wait_for_sessions.py が読む。
"""
import json
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]


def project_log_dir():
    # NOTE: Claude Code は cwd の英数字以外の文字をすべて '-' に置き換える。
    slug = "".join(c if c.isascii() and c.isalnum() else "-" for c in str(REPO))
    return Path.home() / ".claude" / "projects" / slug


def state_dir():
    return project_log_dir() / "session-state"


def lock_path():
    return state_dir() / "build.lock"


def marker_path(session_id):
    return state_dir() / f"{session_id}.json"


# NOTE: 待機者のキュー項目（wait_for_sessions.py 参照）。None ならキーを削除する。
CLEAR_WAIT = {"priority": None, "label": None, "wait_since": None}


def write_marker(session_id, state, prompt=None, extra=None):
    d = state_dir()
    d.mkdir(parents=True, exist_ok=True)
    path = marker_path(session_id)
    data = {}
    if path.exists():
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            data = {}
    data.update({"session_id": session_id, "state": state, "updated": time.time()})
    if prompt is not None:
        data["prompt"] = prompt.replace("\n", " ")[:100]
        data["since"] = time.time()
    for k, v in (extra or {}).items():
        if v is None:
            data.pop(k, None)
        else:
            data[k] = v
    tmp = path.with_suffix(".tmp")
    tmp.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")
    tmp.replace(path)


def release_lock(session_id):
    lock = lock_path()
    try:
        if lock.read_text(encoding="utf-8").strip() == session_id:
            lock.unlink()
            return True
    except OSError:
        pass
    return False


def main():
    d = json.loads(sys.stdin.buffer.read().decode("utf-8") or "{}")
    session_id = d.get("session_id")
    if not session_id:
        return
    event = d.get("hook_event_name")
    if event == "UserPromptSubmit":
        write_marker(session_id, "busy", d.get("prompt", ""))
    elif event in ("Stop", "SessionEnd"):
        marker_path(session_id).unlink(missing_ok=True)
        release_lock(session_id)


if __name__ == "__main__":
    try:
        main()
    except Exception:
        # WARNING: フックが失敗してもセッションを止めてはならない。
        pass
    sys.exit(0)
