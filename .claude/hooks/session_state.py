"""Hook: record whether each Claude Code session of this project is in the middle of a turn.

UserPromptSubmit writes <state dir>/<session_id>.json ("busy"); Stop / SessionEnd removes it and releases the
build lock the session holds. Read by .claude/skills/build-run-wait/wait_for_sessions.py.
"""
import json
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]


def project_log_dir():
    # NOTE: Claude Code replaces every non-alphanumeric character of the cwd with '-'.
    slug = "".join(c if c.isascii() and c.isalnum() else "-" for c in str(REPO))
    return Path.home() / ".claude" / "projects" / slug


def state_dir():
    return project_log_dir() / "session-state"


def lock_path():
    return state_dir() / "build.lock"


def marker_path(session_id):
    return state_dir() / f"{session_id}.json"


def write_marker(session_id, state, prompt=None):
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
        # WARNING: a failing hook must never block the session.
        pass
    sys.exit(0)
