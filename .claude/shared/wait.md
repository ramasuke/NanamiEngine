# `-w` — 他セッションの作業終了を待ってから実行する

`/build` `/build-run` `/commit-push` に共通。`-w` を付けて呼ばれたときだけ、本来の手順の**前**に 1 を、**最後**に 2 を行う。
`-w` 付きで呼ばれたこと自体が、待機後にその skill を実行するユーザー指示にあたる。

## 引数

`$ARGUMENTS` から `-w` を取り除く。直後のトークンが整数(`3`, `-2` など)ならそれも取り除いて priority にする
(無ければ 0)。残りを本来の引数として扱う。

priority は待っているセッション同士の順番だけを決める: 作業中のセッションがいなくなったら、待機中のうち
**priority の大きい順**、同じなら待ち始めた順に 1 つずつ実行する。マイナスは既定(0)の後回し。
作業中のセッションは priority に関係なく必ず待つ。

## 1. 待つ

Bash ツールで **`run_in_background: true`** にして実行し(10 分のツールタイムアウトを超えて待てるように)、
終了通知を待つ。ポーリングや sleep はしない。`<skill>` は `build` / `build-run` / `commit-push`。

```
python .claude/shared/wait_for_sessions.py --self ${CLAUDE_SESSION_ID} --label <skill> --priority <priority>
```

- 各セッションの作業中/待機中は `.claude/hooks/session_state.py`(`UserPromptSubmit` / `Stop` / `SessionEnd` フック)が
  `~/.claude/projects/<slug>/session-state/` に記録している。待ち終わるとロックを取るので、待機中のセッション同士は
  順番に実行する(ロックは `-w` を付けたすべての skill で共通)。
- exit 0: 待ち終わってロック取得。出力の「何分待ったか」と、途中で表示された待ち相手を控えておく。
- exit 2: タイムアウト(既定 60 分)。待っていた相手を報告して終わる(本来の手順は実行しない)。
- `--self` が `$` のまま展開されていないなどで失敗したら、`--status` で状況だけ報告して止まる。
- 状況確認だけなら `python .claude/shared/wait_for_sessions.py --status`。

## 2. ロック解放

成功・失敗・中止のどれでも最後に実行する(忘れても `Stop` フックが解放する):

```
python .claude/shared/wait_for_sessions.py --self ${CLAUDE_SESSION_ID} --release
```

## 報告

本来の報告の先頭に 1 行、「待機: N 分(priority P、待った相手: セッション id 先頭 8 桁 + タイトル)/ 待ちなし」を足す。
