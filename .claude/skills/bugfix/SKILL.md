---
name: bugfix
description: Rider のデバッガで止まっている実行時エラーを、スタックトレース・変数の中身・最近のセッションの変更内容から特定して直す。ユーザーが /bugfix と打ったときだけ使う。
argument-hint: "[補足(症状 / 何をしたら止まったか)]"
disable-model-invocation: true
allowed-tools: mcp__rider__xdebug_get_debugger_status, mcp__rider__xdebug_get_threads, mcp__rider__xdebug_get_stack, mcp__rider__xdebug_get_frame_values, mcp__rider__xdebug_get_value_by_path, mcp__rider__xdebug_evaluate_expression, mcp__rider__xdebug_list_breakpoints
---

# /bugfix — Rider で止まっている実行時エラーを直す

補足: `$ARGUMENTS`(空でもよい。デバッガの状態から判断する)

## 1. 止まっている場所と変数を取る(Rider MCP)

ツールは deferred なので、最初に ToolSearch で一度にまとめて読み込む:
`select:mcp__rider__xdebug_get_debugger_status,mcp__rider__xdebug_get_threads,mcp__rider__xdebug_get_stack,mcp__rider__xdebug_get_frame_values,mcp__rider__xdebug_get_value_by_path,mcp__rider__xdebug_evaluate_expression`

`rootFolder` には常にリポジトリのルートを渡す。

1. `xdebug_get_debugger_status` — セッションがない / 実行中(suspended でない)なら、
   「Rider でデバッグ実行して、エラーで止まった状態で /bugfix してください」と伝えて止まる。
2. `xdebug_get_stack` — 止まったスレッドのコールスタック。例外・assert で止まった場合は CRT / DxLib / 標準ライブラリの
   フレームが上に積まれているので、**最初に出てくるリポジトリ内のフレーム**(`Assets/` `Engine/` `Packages/`)を探す。
   メインスレッドで止まっていないように見えたら `xdebug_get_threads` で止まったスレッドを探す。
3. `xdebug_get_frame_values`(`depth` 1 程度)をそのフレームと、必要なら 1〜2 個上のフレームで取る。
   null / 解放済み(`0xdddddddd` `0xfeeefeee` `0xcdcdcdcd`)/ 範囲外の添字 / 空の `std::vector` や `shared_ptr` などを探し、
   怪しい値は `xdebug_get_value_by_path` / `xdebug_evaluate_expression` で掘る。
4. **再開・ステップ実行・変数の書き換えはしない**(ユーザーの状態を壊さない)。止まっているうちに必要な値は全部取る。

Release / 最適化されたフレームで変数が取れないときは、そのことを報告に含める(Debug で再現してもらう提案)。

## 2. 最近のセッションの変更を見る

```
python .claude/skills/bugfix-recently-session/recent_sessions.py                  # 直近 3 セッション(このセッションは除く)
python .claude/skills/bugfix-recently-session/recent_sessions.py --grep <スタックに出た関数名 / 型名 / ファイル名>
python .claude/skills/bugfix-recently-session/recent_sessions.py --edits <セッションid先頭> [--file <パスの一部>]
```

あわせて `git status --short` / `git diff` / `git log --oneline -10`。コミット済みなら `git show` / `git log -p -- <file>`。

## 3. スタック・変数と変更を結び付ける

- スタック上のファイル / 関数が最近変更されていれば最有力。変更されていなければ、そのフレームに**値を渡している側**
  (呼び出し元、シリアライズされたデータ、`.scene`/`.prefab`/`.meta`)の変更を疑う。
- このリポジトリで多いパターン:
  - シリアライズ対象フィールドの追加/改名・`CEREAL_CLASS_VERSION` の不整合 → ロード後のメンバーが既定値 / null
  - `tools.scene` / `tools.bt` で書いたファイルの guid 参照切れ → `GetComponent` / アセット参照が null
    (`python -m tools.scene validate` / `python -m tools.bt validate` で確認)
  - `.meta` 追加・書き換え後にエディタで `assets_reload` していない
  - `OnDestroy` 後に残った R4 の購読(`.AddTo(this)` 漏れ)が破棄済みオブジェクトに触る
  - シーン context のフィールド追加に `.scene` 側が追従していない
- 変数の値から見て変更と無関係なら、推測で直さずその旨を伝える。

## 4. 直す

- 原因になった変更の**意図は保ったまま**直す。巻き戻すしかない場合は先に聞く。
- CLAUDE.md の規約に従う(ツールキット経由の編集、cereal 登録は `.cpp`、Engine から Assets を include しない等)。
- **ビルドはしない**。確かめたいときは `/build` を提案する。AutoMCP もユーザーが頼んだときだけ。

## 5. 報告

- 止まった場所: `file:line` と、決め手になった変数の値(例: `enemy_ = nullptr`)。
- 原因: どのセッション(タイトル + id 先頭 8 桁)/ コミットのどの変更か。
- 直したこと / 未確認のこと(ビルド未実施、デバッガはまだ止まったまま等)を短く。
