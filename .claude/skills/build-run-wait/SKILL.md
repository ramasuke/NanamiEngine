---
name: build-run-wait
description: このリポジトリの他の Claude Code セッションが作業(ターン)を終えるのを待ってから /build-run を実行する。ユーザーが /build-run-wait と打ったときだけ使う。
argument-hint: "[Debug|Release] [game]"
disable-model-invocation: true
---

# /build-run-wait — 他セッションの作業終了を待ってから /build-run

`/build-run-wait` の実行そのものが、待機・ビルド・起動のユーザー指示にあたる。

## 1. 他セッションを待つ

Bash ツールで **`run_in_background: true`** にして実行し(10 分のツールタイムアウトを超えて待てるように)、
終了通知を待つ。ポーリングや sleep はしない。

```
python .claude/skills/build-run-wait/wait_for_sessions.py --self ${CLAUDE_SESSION_ID}
```

- 各セッションの作業中/待機中は `.claude/hooks/session_state.py`(`UserPromptSubmit` / `Stop` / `SessionEnd` フック)が
  `~/.claude/projects/<slug>/session-state/` に記録している。他の `/build-run-wait` で待機中のセッションは待たない。
  待ち終わったらビルドロックを取るので、同時に待っていたセッション同士は順番にビルドする。
- exit 0: 待ち終わってロック取得。出力の「何分待ったか」と、途中で表示された待ち相手を控えておく。
- exit 2: タイムアウト(既定 60 分)。待っていた相手を報告して終わる(ビルドしない)。
- `--self` が `$` のまま展開されていないなどで失敗したら、`--status` で状況だけ報告して止まる。
- 状況確認だけなら `python .claude/skills/build-run-wait/wait_for_sessions.py --status`。

## 2. /build-run

`.claude/skills/build-run/SKILL.md` を Read し、その 1〜3 と「報告」にそのまま従う(引数 `$ARGUMENTS` の解釈も同じ)。

## 3. ロック解放

成功・失敗・中止のどれでも最後に実行する(忘れても `Stop` フックが解放する):

```
python .claude/skills/build-run-wait/wait_for_sessions.py --self ${CLAUDE_SESSION_ID} --release
```

## 報告

/build-run の報告の先頭に 1 行、「待機: N 分(待った相手: セッション id 先頭 8 桁 + タイトル)/ 待ちなし」を足す。
