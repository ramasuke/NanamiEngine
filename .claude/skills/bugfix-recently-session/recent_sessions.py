"""List what recent Claude Code sessions in this project changed.

    python .claude/skills/bugfix-recently-session/recent_sessions.py [--sessions N] [--include-current]
    python .claude/skills/bugfix-recently-session/recent_sessions.py --grep <text>
    python .claude/skills/bugfix-recently-session/recent_sessions.py --edits <session-id-prefix> [--file <substr>]
"""
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
# NOTE: repo toolkits (tools.scene / tools.bt / ...) rewrite assets without going through Edit/Write.
TOOL_CMD = re.compile(r"python3? (?:-m tools\.|tools/)[^;&|\n]*")
READ_ONLY_CMD = re.compile(r"--help|\b(?:show|validate|check-env|materials|diff)\b|selftest")
EDIT_TOOLS = {"Edit", "Write", "MultiEdit", "NotebookEdit"}


def project_log_dir():
    # NOTE: Claude Code replaces every non-alphanumeric character of the cwd with '-'.
    slug = "".join(c if c.isascii() and c.isalnum() else "-" for c in str(REPO))
    return Path.home() / ".claude" / "projects" / slug


def rel(path):
    try:
        return Path(path).resolve().relative_to(REPO).as_posix()
    except (ValueError, OSError):
        return None


def read_session(path):
    info = {"id": path.stem, "title": "", "prompts": [], "files": {}, "commands": [], "first": "", "last": "", "edits": []}
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            try:
                d = json.loads(line)
            except json.JSONDecodeError:
                continue
            t = d.get("type")
            ts = d.get("timestamp", "")
            if ts:
                info["first"] = info["first"] or ts
                info["last"] = ts
            if t == "ai-title":
                info["title"] = d.get("aiTitle", "")
            elif t == "user" and not d.get("isMeta"):
                content = d.get("message", {}).get("content")
                if isinstance(content, str) and not content.startswith("<"):
                    info["prompts"].append(content.strip())
            elif t == "assistant":
                for c in d.get("message", {}).get("content", []):
                    if not isinstance(c, dict) or c.get("type") != "tool_use":
                        continue
                    inp = c.get("input", {})
                    if c.get("name") in EDIT_TOOLS:
                        p = inp.get("file_path") or inp.get("notebook_path")
                        r = rel(p) if p else None
                        if r:
                            info["files"][r] = info["files"].get(r, 0) + 1
                            info["edits"].append((ts, c["name"], r, inp))
                    elif c.get("name") in ("Bash", "PowerShell"):
                        for m in TOOL_CMD.finditer(inp.get("command", "")):
                            cmd = m.group(0).strip()
                            if not READ_ONLY_CMD.search(cmd):
                                info["commands"].append(cmd[:200])
    return info


def git_dirty():
    try:
        out = subprocess.run(["git", "-c", "core.quotepath=false", "status", "--porcelain"], cwd=REPO,
                             capture_output=True, text=True, encoding="utf-8").stdout
    except OSError:
        return set()
    return {line[3:].strip().strip('"') for line in out.splitlines()}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sessions", type=int, default=3)
    ap.add_argument("--include-current", action="store_true", help="also list the newest (= running) session")
    ap.add_argument("--grep", help="only sessions whose prompts/title/files contain this text")
    ap.add_argument("--edits", help="print the Edit/Write inputs of the session with this id prefix")
    ap.add_argument("--file", help="with --edits: only edits of paths containing this text")
    args = ap.parse_args()

    logs = sorted(project_log_dir().glob("*.jsonl"), key=lambda p: p.stat().st_mtime, reverse=True)
    if not logs:
        sys.exit(f"no session logs in {project_log_dir()}")

    if args.edits:
        match = [p for p in logs if p.stem.startswith(args.edits)]
        if not match:
            sys.exit(f"no session starting with {args.edits}")
        for ts, tool, path, inp in read_session(match[0])["edits"]:
            if args.file and args.file not in path:
                continue
            print(f"=== {ts} {tool} {path}")
            if tool == "Write":
                print(f"(whole file, {len(inp.get('content', ''))} chars)")
            elif tool == "Edit":
                print("--- old\n" + inp.get("old_string", "") + "\n+++ new\n" + inp.get("new_string", ""))
            else:
                print(json.dumps(inp, ensure_ascii=False)[:2000])
        return

    if not args.include_current:
        logs = logs[1:]
    dirty = git_dirty()
    shown = 0
    for p in logs:
        s = read_session(p)
        if not s["files"] and not s["commands"]:
            continue
        if args.grep:
            hay = " ".join([s["title"], *s["prompts"], *s["files"], *s["commands"]])
            if args.grep.lower() not in hay.lower():
                continue
        print(f"## {s['id'][:8]}  {s['first'][:16]} -> {s['last'][:16]}  {s['title']}")
        for prompt in s["prompts"][:3]:
            print("  > " + prompt.replace("\n", " ")[:160])
        for f, n in sorted(s["files"].items()):
            mark = "*" if f in dirty else " "
            print(f"  {mark} {f} ({n})")
        for cmd in s["commands"][:15]:
            print(f"  $ {cmd}")
        print()
        shown += 1
        if shown >= args.sessions:
            break
    print("(* = uncommitted in git)")


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    main()
