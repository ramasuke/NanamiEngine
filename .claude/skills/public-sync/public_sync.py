"""Sync the private NanamiEngine repo into the public snapshot repo without any AI trace.

  python .claude/skills/public-sync/public_sync.py plan            # what would be copied / deleted
  python .claude/skills/public-sync/public_sync.py apply           # copy + stage in the public repo, scan for AI traces
  python .claude/skills/public-sync/public_sync.py commit -F msg   # check message, commit, push, record the synced commit
  python .claude/skills/public-sync/public_sync.py audit           # every difference between the two HEAD trees
  python .claude/skills/public-sync/public_sync.py mark <commit>   # set the last synced private commit by hand

The last synced private commit is kept in the public repo's .git dir (never pushed).
"""
from __future__ import annotations

import argparse
import fnmatch
import re
import shutil
import subprocess
import sys
from pathlib import Path

PRIVATE = Path(__file__).resolve().parents[3]
DEFAULT_PUBLIC = PRIVATE.parent / "NanamiEngine-Public"
STATE_NAME = "nanami_private_synced"

# Never copied from private, never deleted in public.
EXCLUDE = [
    ".claude/*", ".codex/*", "tools/*", ".mcp.json", "CLAUDE.md", "README.md",
    "Packages/*/README.md", "*/_Source/*.py", "Log.txt", "imgui.ini",
]
# Rewritten for public (tools / AI references removed): changes must be ported by hand.
MANUAL = [".gitignore", "docs/*.md"]
# Gitignored in private but tracked in public: synced from the private working tree.
WORKTREE_DIRS = ["Assets/Scripts/GamePlay/Debug/DebugSheet"]

TRACE = re.compile(r"claude|anthropic|codex|co-authored|chatgpt|copilot|\.mcp\.json|python -m tools|\btools/", re.I)
MESSAGE_TRACE = re.compile(r"claude|anthropic|codex|co-authored|chatgpt|copilot|\bAI\b|generated with|noreply@", re.I)


def git(repo: Path, *args: str, check: bool = True) -> str:
    r = subprocess.run(["git", "-c", "core.quotepath=false", *args], cwd=repo, capture_output=True,
                       text=True, encoding="utf-8", errors="replace")
    if check and r.returncode != 0:
        sys.exit(f"git {' '.join(args)} failed in {repo}:\n{r.stderr}")
    return r.stdout


def matches(path: str, patterns: list[str]) -> bool:
    return any(fnmatch.fnmatch(path, p) for p in patterns)


def state_file(public: Path) -> Path:
    return public / git(public, "rev-parse", "--git-path", STATE_NAME).strip()


def last_synced(public: Path) -> str:
    f = state_file(public)
    if not f.exists():
        sys.exit(f"{f} がありません。前回同期した private のコミットを `mark <commit>` で記録してください")
    return f.read_text(encoding="utf-8").strip()


def same(a: Path, b: Path) -> bool:
    return a.exists() and b.exists() and a.read_bytes() == b.read_bytes()


def build_plan(public: Path):
    base = last_synced(public)
    head = git(PRIVATE, "rev-parse", "HEAD").strip()
    copy, delete, manual, skipped = [], [], [], []
    for line in git(PRIVATE, "diff", "--name-status", "--no-renames", f"{base}..{head}").splitlines():
        status, path = line.split("\t", 1)
        if matches(path, EXCLUDE):
            skipped.append(path)
        elif matches(path, MANUAL):
            manual.append(f"{status} {path}")
        elif status == "D":
            if (public / path).exists():
                delete.append(path)
        elif not same(PRIVATE / path, public / path):
            copy.append(path)
    for d in WORKTREE_DIRS:
        src, dst = PRIVATE / d, public / d
        names = {p.relative_to(src).as_posix() for p in src.rglob("*") if p.is_file()} if src.exists() else set()
        theirs = {p.relative_to(dst).as_posix() for p in dst.rglob("*") if p.is_file()} if dst.exists() else set()
        copy += [f"{d}/{n}" for n in sorted(names) if not same(src / n, dst / n)]
        delete += [f"{d}/{n}" for n in sorted(theirs - names)]
    dirty = [l[3:] for l in git(PRIVATE, "status", "--porcelain").splitlines() if l[3:] in set(copy)]
    return base, head, copy, delete, manual, skipped, dirty


def print_plan(plan) -> None:
    base, head, copy, delete, manual, skipped, dirty = plan
    print(f"private {base[:8]}..{head[:8]}")
    for title, items in (("copy", copy), ("delete", delete), ("manual (書き直して反映)", manual),
                         ("dirty in private (先に /commit-push)", dirty)):
        print(f"\n[{title}] {len(items)}")
        for p in items:
            print(f"  {p}")
    print(f"\n[excluded] {len(skipped)}")


def check_public(public: Path) -> None:
    if git(public, "rev-parse", "--abbrev-ref", "HEAD").strip() != "master":
        sys.exit("public が master ではありません")
    git(public, "fetch", "-q", "origin")
    behind = git(public, "rev-list", "--count", "HEAD..origin/master").strip()
    if behind != "0":
        sys.exit(f"public が origin/master より {behind} コミット遅れています。先に pull してください")
    staged = git(public, "diff", "--cached", "--name-only").strip()
    if staged:
        sys.exit(f"public に既にステージ済みの変更があります:\n{staged}")


def cmd_apply(public: Path) -> int:
    check_public(public)
    plan = build_plan(public)
    print_plan(plan)
    _, _, copy, delete, manual, _, dirty = plan
    if dirty:
        sys.exit("\nprivate に未コミットの変更があるので中止しました")
    for p in copy:
        (public / p).parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(PRIVATE / p, public / p)
    for p in delete:
        (public / p).unlink()
    if copy or delete:
        git(public, "add", "-A", "--", *copy, *delete)
    hits = []
    current = None
    for line in git(public, "diff", "--cached", "-U0").splitlines():
        if line.startswith("+++ "):
            current = line[6:]
        elif line.startswith("+") and TRACE.search(line):
            hits.append(f"  {current}: {line[1:].strip()[:160]}")
    names = [n for n in git(public, "diff", "--cached", "--name-only").splitlines() if TRACE.search(n)]
    print(f"\nstaged {len(copy)} copied / {len(delete)} deleted")
    if hits or names:
        print("\n[AI / tools の痕跡] 確認して消すこと")
        print("\n".join(hits + [f"  path: {n}" for n in names]))
        return 2
    if manual:
        print("\nmanual の項目を public 側で書き直してから commit してください")
    return 0


def cmd_commit(public: Path, message_file: Path, push: bool) -> int:
    msg = message_file.read_text(encoding="utf-8")
    if MESSAGE_TRACE.search(msg):
        sys.exit(f"コミットメッセージに AI の痕跡があります: {MESSAGE_TRACE.search(msg).group(0)}")
    if not git(public, "diff", "--cached", "--name-only").strip():
        sys.exit("public にステージされた変更がありません")
    name = git(public, "config", "user.name").strip()
    email = git(public, "config", "user.email").strip()
    if MESSAGE_TRACE.search(name + " " + email):
        sys.exit(f"public の git user が {name} <{email}> です")
    git(public, "commit", "-q", "-F", str(message_file))
    head = git(PRIVATE, "rev-parse", "HEAD").strip()
    state_file(public).write_text(head + "\n", encoding="utf-8")
    print(git(public, "log", "-1", "--format=%h %an <%ae>%n%B"))
    if push:
        r = subprocess.run(["git", "push", "origin", "master"], cwd=public, capture_output=True, text=True)
        print(r.stdout + r.stderr)
        return r.returncode
    return 0


def cmd_audit(public: Path) -> None:
    def tree(repo):
        out = git(repo, "ls-tree", "-r", "--format=%(objectname)\t%(path)", "HEAD")
        return {p: h for h, p in (l.split("\t", 1) for l in out.splitlines())}
    a, b = tree(PRIVATE), tree(public)
    public_only = [p for p in b if any(p.startswith(d + "/") for d in WORKTREE_DIRS)]
    print("[private only]", *[p for p in sorted(set(a) - set(b)) if not matches(p, EXCLUDE)], sep="\n  ")
    print("[public only]", *[p for p in sorted(set(b) - set(a)) if p not in public_only], sep="\n  ")
    print("[differ]", *[p for p in sorted(set(a) & set(b)) if a[p] != b[p] and not matches(p, EXCLUDE)], sep="\n  ")


def main() -> int:
    sys.stdout.reconfigure(encoding="utf-8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--public", type=Path, default=DEFAULT_PUBLIC)
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("plan")
    sub.add_parser("apply")
    sub.add_parser("audit")
    c = sub.add_parser("commit")
    c.add_argument("-F", dest="message_file", type=Path, required=True)
    c.add_argument("--no-push", action="store_true")
    m = sub.add_parser("mark")
    m.add_argument("commit")
    args = ap.parse_args()
    public = args.public.resolve()
    if args.cmd == "plan":
        print_plan(build_plan(public))
    elif args.cmd == "apply":
        return cmd_apply(public)
    elif args.cmd == "audit":
        cmd_audit(public)
    elif args.cmd == "commit":
        return cmd_commit(public, args.message_file, not args.no_push)
    elif args.cmd == "mark":
        state_file(public).write_text(git(PRIVATE, "rev-parse", args.commit).strip() + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
