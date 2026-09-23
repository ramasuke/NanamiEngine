---
name: commit-push
description: 最近の Claude Code セッション履歴と変更ファイルからコミットメッセージを作り、全変更をコミットして push する。ユーザーが /commit-push と打ったときだけ使う。
argument-hint: "[メッセージのヒント / 対象を絞る指示(任意)]"
disable-model-invocation: true
---

# /commit-push — セッション履歴と変更を見てコミット & プッシュ

追加の指示: `$ARGUMENTS`(空なら全変更を 1 コミットにまとめる。ヒントがあればメッセージに反映し、対象の指定があればそれだけをステージする)

## 1. 状況を集める(並列で)

```
git status --short
git diff --stat
git diff --cached --stat
git log --oneline -10
git branch --show-current
python .claude/skills/bugfix-recently-session/recent_sessions.py --sessions 5 --include-current
```

- `recent_sessions.py` は各セッションのタイトル・最初の依頼・Edit/Write したファイル(`*` = 未コミット)・
  `python -m tools.*` の書き込み系コマンドを出す。**変更ファイルがどのセッションの何の作業か**を対応付けるのに使う。
- コード差分は `git diff -- <file>` で必要な分だけ見る(アセットや `.meta`・バイナリは stat で十分)。
- 変更が何も無ければ「コミットするものがありません」と言って止まる。

## 2. 入れてはいけないものを確認する

次に当たるファイルがあればステージから外し、報告で触れる(勝手に削除・`.gitignore` 追記はしない):

- 秘密情報: `rclone.conf`、`.env`、鍵・トークン・パスワードを含むもの
- ビルド成果物・リリース成果物(`.gitignore` 済みのはずだが、`manifest*.json` / `installed.json` / `*.exe` / `lib/` / `x64/` など)
- `LocalPrefs/` の個人設定、`*.bak`、明らかな一時ファイル
- 100 MB を超えるファイル(GitHub が拒否する): `git diff --cached --numstat` ではなく実ファイルサイズで確認

迷うものは入れずに報告する。

## 3. コミットメッセージを書く

既存の履歴のスタイルに合わせる(日本語、Conventional Commits 風):

```
<type>(<scope 任意>): <全体の要約を 1 行で>

- <作業単位ごとの箇条書き(セッション/機能ごとに 1 行)>
- ...

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
```

- `type`: `feat` / `fix` / `refactor` / `chore` / `docs` など。複数混在なら主な方。scope は `game` / `tools` / `engine` など絞れるときだけ。
- 要約行は「何ができるようになったか / 何を直したか」。ファイル名の羅列にしない。
- 箇条書きはセッション履歴の依頼内容とファイルから作業単位にまとめる。ログ(`EngineLog.txt` 等)や ImGui レイアウト(`imgui.ini`)、
  `.meta` の再生成だけの変更は「ログと ImGui レイアウトを更新」のように 1 行に丸める。
- それしか変更が無い小さなコミットは本文なしの 1 行でよい(例: `chore: ログと ImGui レイアウトを更新`)。
- 末尾の `Co-Authored-By` 行はシステムから指示されたものを使う(上は現時点の例)。

## 4. コミットしてプッシュする

- ステージ: 除外が無ければ `git add -A`、あれば `git add -A` の後 `git reset -- <除外>`。
- メッセージは一時ファイル経由で渡す(日本語・複数行のため):
  scratchpad に書いて `git commit -F <file>`。
- pre-commit フックが失敗したら `--no-verify` で飛ばさず、原因を直して**新しい**コミットを作る(amend しない)。
- `git push`(上流が無ければ `git push -u origin <branch>`)。
- push が non-fast-forward で拒否されたら `git pull --rebase` → 競合が無ければ再 push。競合したら止めて報告する。
  **force push は絶対にしない。**
- ビルドはしない(CLAUDE.md)。

## 5. 報告

- コミットハッシュ(短縮)とメッセージの 1 行目、push 先ブランチ
- 除外したファイルがあればその一覧と理由
