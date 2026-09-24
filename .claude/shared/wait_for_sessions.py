"""Wait until no other Claude Code session of this project is mid-turn, then take the build lock.

    python .claude/shared/wait_for_sessions.py [--self <session_id>] [--priority N] [--label build]
                                                                [--timeout 3600] [--poll 10]
    python .claude/shared/wait_for_sessions.py [--self <session_id>] --release
    python .claude/shared/wait_for_sessions.py --status

--self defaults to $CLAUDE_CODE_SESSION_ID (set by Claude Code in its shells), then $CLAUDE_SESSION_ID.

Exit 0 = lock taken (release it with --release; the Stop hook also does), 2 = timed out.
Busy/idle comes from the markers written by .claude/hooks/session_state.py.
Waiters (/build, /build-run, /commit-push with -w; see wait.md) share one lock and go in order of priority (higher first, negative
allowed, default 0), then of when they started waiting.
"""
import argparse
import json
import os
import sys
import time
from datetime import datetime
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "hooks"))
import session_state as ss  # noqa: E402


def jsonl_age(session_id):
    p = ss.project_log_dir() / f"{session_id}.jsonl"
    try:
        return time.time() - p.stat().st_mtime
    except OSError:
        return None


def read_markers():
    out = []
    d = ss.state_dir()
    if not d.exists():
        return out
    for p in d.glob("*.json"):
        try:
            out.append(json.loads(p.read_text(encoding="utf-8")))
        except (OSError, json.JSONDecodeError):
            continue
    return out


def title_of(session_id):
    p = ss.project_log_dir() / f"{session_id}.jsonl"
    title = ""
    try:
        with open(p, encoding="utf-8", errors="replace") as f:
            for line in f:
                if '"ai-title"' in line:
                    try:
                        title = json.loads(line).get("aiTitle", title)
                    except json.JSONDecodeError:
                        pass
    except OSError:
        pass
    return title


def is_stale(session_id, stale_secs):
    # NOTE: an Esc-interrupted turn or a crashed session never sends Stop, so fall back to log activity.
    age = jsonl_age(session_id)
    return age is None or age > stale_secs


def busy_others(self_id, stale_secs):
    return [m for m in read_markers()
            if m.get("session_id") != self_id and m.get("state") == "busy"
            and not is_stale(m["session_id"], stale_secs)]


def live_waiters(poll):
    # NOTE: a waiter rewrites its marker every poll; one that stopped doing so was killed.
    fresh = max(poll * 3, 60)
    now = time.time()
    return [m for m in read_markers()
            if m.get("state") == "waiting" and now - m.get("updated", 0) <= fresh]


def queue_key(m):
    return (-m.get("priority", 0), m.get("wait_since", m.get("updated", 0)), m.get("session_id", ""))


def lock_owner():
    try:
        return ss.lock_path().read_text(encoding="utf-8").strip() or None
    except OSError:
        return None


def try_lock(self_id, stale_secs):
    owner = lock_owner()
    if owner == self_id:
        return True
    if owner and is_stale(owner, stale_secs):
        ss.lock_path().unlink(missing_ok=True)
    try:
        ss.state_dir().mkdir(parents=True, exist_ok=True)
        with open(ss.lock_path(), "x", encoding="utf-8") as f:
            f.write(self_id)
        return True
    except FileExistsError:
        return False


def describe(m):
    if m.get("state") == "waiting":
        since = datetime.fromtimestamp(m.get("wait_since", m.get("updated", 0))).strftime("%H:%M")
        return (f"{m['session_id'][:8]} ({m.get('label', '?')}, priority {m.get('priority', 0)}, since {since}) "
                f"{title_of(m['session_id'])}")
    since = datetime.fromtimestamp(m.get("since", m.get("updated", 0))).strftime("%H:%M")
    return f"{m['session_id'][:8]} (since {since}) {title_of(m['session_id'])} > {m.get('prompt', '')}"


def status(stale_secs):
    markers = read_markers()
    if not markers:
        print("no session markers")
    for m in markers:
        stale = " [stale]" if is_stale(m["session_id"], stale_secs) else ""
        print(f"{m.get('state', '?'):8} {describe(m)}{stale}")
    print(f"build lock: {lock_owner() or '(free)'}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--self", dest="self_id",
                    help="this session's id (default: $CLAUDE_CODE_SESSION_ID, then $CLAUDE_SESSION_ID)")
    ap.add_argument("--priority", type=int, default=0, help="higher goes first among waiters; may be negative")
    ap.add_argument("--label", default="build", help="the skill this waiter will run (shown to the others)")
    ap.add_argument("--timeout", type=float, default=3600)
    ap.add_argument("--poll", type=float, default=10)
    ap.add_argument("--stale-mins", type=float, default=45,
                    help="ignore markers/locks of sessions whose log has not changed for this long")
    ap.add_argument("--release", action="store_true")
    ap.add_argument("--status", action="store_true")
    args = ap.parse_args()
    stale_secs = args.stale_mins * 60

    if args.status:
        status(stale_secs)
        return 0
    # NOTE: an unexpanded "${CLAUDE_SESSION_ID}" or an empty --self falls back to the environment.
    if not args.self_id or args.self_id.startswith("$"):
        args.self_id = os.environ.get("CLAUDE_CODE_SESSION_ID") or os.environ.get("CLAUDE_SESSION_ID")
    if not args.self_id:
        print("no session id: pass --self <session_id> (the last folder of the scratchpad path)", file=sys.stderr)
        return 1
    if args.release:
        print("released" if ss.release_lock(args.self_id) else "lock not held by this session")
        return 0

    start = time.time()
    last_seen = None
    while True:
        # NOTE: waiters never count as busy for each other; they only queue by priority for the lock.
        ss.write_marker(args.self_id, "waiting",
                        extra={"priority": args.priority, "label": args.label, "wait_since": start})
        others = busy_others(args.self_id, stale_secs)
        ahead = [m for m in live_waiters(args.poll) if m["session_id"] != args.self_id
                 and queue_key(m) < queue_key({"session_id": args.self_id, "priority": args.priority,
                                               "wait_since": start})]
        if not others and ahead:
            seen = ("queue", tuple(sorted(m["session_id"] for m in ahead)))
            if seen != last_seen:
                print(f"waiting behind {len(ahead)} waiter(s) (priority {args.priority}):", flush=True)
                for m in sorted(ahead, key=queue_key):
                    print("  " + describe(m), flush=True)
        elif not others:
            if try_lock(args.self_id, stale_secs):
                ss.write_marker(args.self_id, "busy", extra=ss.CLEAR_WAIT)
                print(f"ready after {(time.time() - start) / 60:.1f} min; build lock taken")
                return 0
            seen = ("lock", lock_owner())
            if seen != last_seen:
                print(f"waiting for build lock held by {(lock_owner() or '?')[:8]} {title_of(lock_owner() or '')}", flush=True)
        else:
            seen = tuple(sorted(m["session_id"] for m in others))
            if seen != last_seen:
                print(f"waiting for {len(others)} session(s):", flush=True)
                for m in others:
                    print("  " + describe(m), flush=True)
        last_seen = seen
        if time.time() - start > args.timeout:
            print(f"timed out after {args.timeout / 60:.0f} min")
            ss.write_marker(args.self_id, "busy", extra=ss.CLEAR_WAIT)
            return 2
        time.sleep(args.poll)


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.exit(main())
