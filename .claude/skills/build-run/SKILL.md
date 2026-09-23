---
name: build-run
description: /build と同じ「ビルド→修正→再ビルド」ループを回し、成功したら exe を起動する。ユーザーが /build-run と打ったときだけ使う。
argument-hint: "[Debug|Release] [game]"
disable-model-invocation: true
---

# /build-run — ビルドが通るまで修正して、起動まで

`/build-run` の実行そのものが、ビルドと起動の両方のユーザー指示にあたる。

## 1. 起動中の exe を確認(ビルド前)

起動中の exe があるとリンクで `LNK1104` になるので、先に確認する。

```powershell
Get-Process NanamiEngine -ErrorAction SilentlyContinue | Select-Object Id, Path, MainWindowTitle
```

今回ビルドする exe(下の表)と同じパスのプロセスがあれば、**勝手に終了させず** AskUserQuestion で
「終了してからビルド」「中止」を聞く。終了を選ばれたら `Stop-Process -Id <Id>` し、未保存の変更は失われる旨を
質問文に書いておく。

## 2. ビルドループ

`.claude/skills/build/SKILL.md` を Read し、その「引数」「ビルドコマンド」「ループ」「止まる条件」「修正のルール」に
そのまま従う(引数 `$ARGUMENTS` の解釈も同じ)。止まる条件に当たったら起動はせず、/build と同じ形で報告して終わる。

## 3. 起動

ビルドが成功したら exe を起動する。作業ディレクトリはリポジトリルート、`-project` にもリポジトリルートを渡す。

| 引数 | exe |
|---|---|
| (なし) / `Debug` | `x64\Debug\NanamiEngine.exe` |
| `Release` | `x64\Release\NanamiEngine.exe` |
| `game` | `x64\Game\<Config>\NanamiEngine.exe` |

PowerShell ツールで、切り離して起動する(完了を待たない):

```powershell
$repo = (Get-Location).Path
$p = Start-Process -FilePath "$repo\<exe>" -ArgumentList '-project', "`"$repo`"" -WorkingDirectory $repo -PassThru
Start-Sleep -Seconds 5
if ($p.HasExited) { "exited: $($p.ExitCode)" } else { "running: $($p.Id)" }
```

- exe が見つからなければ、ビルド出力の `->` 行から実際の出力先を確認して起動する。
- 5 秒以内に終了していたら起動失敗として、終了コードと リポジトリルートの `EngineLog.txt` / `Log.txt`(と `Logs/` の最新ファイル)の末尾を見て報告する。
  起動時のクラッシュは修正ループの対象にしない(原因の見当を報告して判断を仰ぐ)。
- 起動は `nanami` MCP の `engine_launch` ではなく上のコマンドで行う。起動後にスクショやプレイなど AutoMCP の
  操作はしない(ユーザーに頼まれたときだけ)。

## 報告

日本語で短く:

- ビルド結果(構成・ビルド回数・直したエラーと変更ファイル `path:line`)。
- 起動結果(起動した exe とプロセス ID、または起動失敗の内容)。
