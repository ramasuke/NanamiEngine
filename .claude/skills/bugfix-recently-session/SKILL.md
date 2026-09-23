---
name: bugfix-recently-session
description: 最近の Claude Code セッションで入った変更が原因のエラー/不具合を、そのセッションの変更内容を手がかりに特定して直す。ユーザーが /bugfix-recently-session と打ったときだけ使う。
argument-hint: "<エラー内容 / 症状>"
disable-model-invocation: true
---

# /bugfix-recently-session — 最近のセッションの変更が原因のバグを直す

症状: `$ARGUMENTS`

(空なら、何が起きているか(エラーメッセージ / ログ / 再現手順)を一言聞いて止まる。)

## 1. 最近のセッションが何を変えたかを見る

```
python .claude/skills/bugfix-recently-session/recent_sessions.py                  # 直近 3 セッション(実行中のこのセッションは除く)
python .claude/skills/bugfix-recently-session/recent_sessions.py --sessions 6
python .claude/skills/bugfix-recently-session/recent_sessions.py --grep <症状のキーワード/ファイル名/型名>
python .claude/skills/bugfix-recently-session/recent_sessions.py --edits <セッションid先頭> [--file <パスの一部>]   # そのセッションの Edit の old/new
```

一覧には各セッションのタイトル・最初の依頼・Edit/Write したリポジトリ内ファイル(`*` = git 未コミット)・
実行した `python -m tools.*` の書き込み系コマンド(scene / bt / animtree / effect などはツール経由でアセットを書き換えるので
Edit には出ない)が出る。

あわせて `git status --short` / `git diff` / `git log --oneline -10` も見る。セッションの変更がコミット済みなら
`git log -p --since=... -- <file>` や `git show` で差分を追う。

## 2. 症状と変更を結び付ける

- エラーメッセージ・ログに出るファイル名 / 型名 / GameObject 名 / アセット名を、上の一覧と `--grep` で突き合わせ、
  **原因になりうる変更を絞り込む**。関係するセッションの `--edits` で「何をなぜ変えたか」を読む。
- 典型パターン(このリポジトリで多いもの):
  - 新規 `.cpp`/`.h` が `.vcxproj` に未追加 → リンクエラー / 未登録
  - cereal 登録マクロの置き場所・`CEREAL_CLASS_VERSION` の不整合、シリアライズ対象フィールドの追加/改名 → 既存 `.scene`/`.prefab`/`.meta` のロード失敗
  - `tools.scene` / `tools.bt` で書いたファイルの guid 参照切れ、`worldMatrix_` 未ベイク、クラスバージョン不一致 → `python -m tools.scene validate` / `python -m tools.bt validate` で確認
  - `.meta` 追加・書き換え後にエディタ側で `assets_reload` されていない
  - ヘッダのシグネチャ変更に追従していない呼び出し元
- 変更と無関係に見えたら、推測で直さずその旨をユーザーに伝える。

## 3. 直す

- 原因になった変更の**意図は保ったまま**直す(そのセッションで依頼された機能を巻き戻さない)。巻き戻すしかない場合は先に聞く。
- CLAUDE.md の規約(ツールキット経由の編集、cereal 登録は `.cpp`、Engine から Assets を include しない等)に従う。
- **ビルドはしない**(CLAUDE.md)。ビルドで確かめたいときは `/build` を提案する。AutoMCP もユーザーが頼んだときだけ。
- `tools/*` を触ったらそのツールの selftest、アセットを書き換えたら `validate` は回してよい。

## 4. 報告

- 原因: どのセッション(タイトル + id 先頭 8 桁)のどの変更か、`file:line` で。
- 直したこと / 未確認のこと(ビルド未実施など)を短く。
