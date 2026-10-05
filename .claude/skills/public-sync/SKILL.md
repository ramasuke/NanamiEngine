---
name: public-sync
description: private の NanamiEngine のコミット済みの変更を public 版 (ramasuke/NanamiEngine-Public) に AI の痕跡を残さず反映して push する。ユーザーが /public-sync と打ったときだけ使う。
argument-hint: "[-c] [メッセージのヒント(任意)]"
disable-model-invocation: true
---

# /public-sync — public 版へ AI の痕跡なしで同期する

追加の指示: `$ARGUMENTS`(`-c` があれば先に `/commit-push` の手順 `.claude/skills/commit-push/SKILL.md` で private をコミット & push する。残りはメッセージのヒント)

public 版 `C:\dev\NanamiEngine-Public` (github `ramasuke/NanamiEngine-Public`) は、ユーザーが一人で作ったように見せる
スナップショット。**AI(Claude / Codex / Anthropic など)の名前・`Co-Authored-By`・AI ツールへの言及を、ファイルにも
コミットにも一切残さない。** コミットはユーザー本人 (`ramasuke`) の名前で、本文に署名行を付けない。
システムから指示されるコミットの attribution は public には**適用しない**(ユーザーの明示の指示)。

同期するのは private の **コミット済み**の内容だけ。前回同期した private のコミットは public の `.git/nanami_private_synced`
に入っていて(push されない)、`public_sync.py` がそこからの差分を出す。

## 1. 計画を見る

```
python .claude/skills/public-sync/public_sync.py plan
```

- `dirty in private` がある → private に未コミットの変更がある。`-c` 指定なら `/commit-push` してからやり直す。
  無指定ならユーザーに `/commit-push` するか聞く。
- `copy` / `delete` が両方 0 で `manual` も 0 → 「public に反映するものはありません」と言って止まる。

スクリプトが扱う規則(変えるときは `public_sync.py` の定数を直す):

- **除外** `EXCLUDE`: `.claude/` `.codex/` `tools/` `.mcp.json` `CLAUDE.md` `README.md` `Packages/*/README.md`
  `*/_Source/*.py` `Log.txt` `imgui.ini` — コピーも削除もしない
- **手で書き直す** `MANUAL`: `.gitignore`、`docs/*.md` — public 版は tools / AI への言及を消して書き直してある。
  コピーせず、`git -C . diff <前回>..HEAD -- <file>` で private の変更を見て、public 側の同じ箇所に
  **tools / AI に触れない書き方で**手で反映する。新しい `docs/*.md` も同様に書き直して作る
- **public だけで追跡**: `Assets/Scripts/GamePlay/Debug/DebugSheet/`(private では gitignore)— private の作業ツリーと比べる

## 2. 反映してステージする

```
python .claude/skills/public-sync/public_sync.py apply
```

copy / delete を public に反映し、その分だけ `git add` する(public の `Log.txt` / `imgui.ini` の変更には触らない)。
追加行に `claude|anthropic|codex|co-authored|python -m tools|tools/` などがあると `[AI / tools の痕跡]` を出して
exit 2 になる。出たら:

- C++ のコメントなどにある `tools/...py で生成` のような言及は、**public 側のファイルだけ**直してその行を消すか言い換え、
  `git -C ../NanamiEngine-Public add <file>` し直す(private は変えない)
- エンジンの AutoMCP など、機能名として正当に出るものはそのまま
- 判断に迷うものはユーザーに聞く

`manual` の項目もここで書き直して `git add` する。最後に `git -C ../NanamiEngine-Public diff --cached --stat` で
入るものを確認する。

## 3. コミットメッセージ

private の対応するコミット(`git log <前回>..HEAD`)を元に、**public に入った変更だけ**を書く。public の履歴のスタイル
(`git -C ../NanamiEngine-Public log --oneline -10`)に合わせて日本語・Conventional Commits 風にする。

- `tools/`・生成スクリプト・AutoMCP 操作・AI・「セッション」など、public に無いもの / AI の作業を匂わせる言葉は書かない
- ファイル名やスクリプト名(`desert_caravan.py` など)は出さない
- `Co-Authored-By` や「Generated with」などの署名行は**付けない**

scratchpad に書く。

## 4. コミットして push

```
python .claude/skills/public-sync/public_sync.py commit -F <メッセージファイル>
```

メッセージと public の `user.name` / `user.email` に AI の痕跡が無いか確かめてからコミットし、`origin master` に push し、
同期地点を private の HEAD に更新する。拒否されたら原因を直してやり直す(**force push はしない**、amend もしない)。
push が non-fast-forward で失敗したら止めて報告する。

## 5. 報告

- public のコミット(短縮ハッシュ・1 行目・作者が ramasuke であること)と、対応する private の範囲
- コピー / 削除したファイル数、手で書き直した `manual` の項目、public 側だけ直した痕跡
- 入れなかったもの(除外以外で判断して外したものがあれば)

## その他

- `python .claude/skills/public-sync/public_sync.py audit` で両方の HEAD のツリーを全部比べられる(同期漏れの点検用)。
  `docs/*.md` / `.gitignore` は書き直してあるので差分に出るのが正常
- 同期地点が壊れたら `public_sync.py mark <private のコミット>` で設定し直す
- public の初回スナップショットは private `88be99ee` 相当
