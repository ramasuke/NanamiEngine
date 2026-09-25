---
name: build
description: EnviroHunter.sln をビルドし、エラーが出たら修正して再ビルドを成功するまで繰り返す。ユーザーが /build と打ったときだけ使う。
argument-hint: "[-w [priority]] [Debug|Release] [game]"
disable-model-invocation: true
---

# /build — ビルドが通るまで修正ループ

`/build` の実行そのものがユーザーからのビルド指示(CLAUDE.md の「ビルドして」)にあたる。
このループ中は確認なしでビルド・再ビルドしてよい。

## 待機オプション `-w [priority]`

引数に `-w` があれば `.claude/shared/wait.md` を Read し、その手順で他セッションの作業終了を待ってから
以下を実行する(`-w` と priority は取り除いた残りを引数とする)。無ければ待たない。

## 引数

`$ARGUMENTS` を見て決める(大文字小文字は無視)。

- `Debug` / `Release` — 構成。省略時は `Debug`。
- `game` — ゲーム(非エディタ)版。`-p:NanamiApplicationMode=Game` を付ける。省略時はエディタ版。

## ビルドコマンド

Bash ツールから、ログをスクラッチパッドに書き出して実行する(`timeout` は 600000)。

```
"/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" EnviroHunter.sln \
  -p:Configuration=<Config> -p:Platform=x64 -p:PreferredToolArchitecture=x64 -m:12 \
  [-p:NanamiApplicationMode=Game] \
  -nologo -v:minimal -clp:ErrorsOnly \
  "-flp:LogFile=<scratchpad>/build.log;Verbosity=normal"
```

- `-p:PreferredToolArchitecture=x64` は必須(外すと C1060 でヒープ不足)。
- 10 分で終わらなければ `run_in_background` で回し、完了通知を待つ。
- 出力が長いときは `grep -E "error [A-Z]+[0-9]+" <scratchpad>/build.log | sort -u` でエラーだけ拾う。

## ループ

1. ビルドする。
2. 成功(`0 エラー` / `0 Error(s)`、終了コード 0)なら終了して報告する。
3. 失敗したら、重複を除いたエラー一覧を作り、**最初のエラーから**原因を読んで直す。
   連鎖エラー(1 つのヘッダの誤りから大量に出るもの)は根本の 1 件を直してから再ビルドする。
4. 1 に戻る。

### 止まる条件

次のときはループを止め、状況(残りのエラー・試したこと)を報告してユーザーに判断を仰ぐ。

- 修正→再ビルドを **8 回**繰り返しても通らない。
- 同じエラーが 2 回続けて変わらない(直し方が合っていない)。
- 直すには仕様判断が要る(API の形を変える、機能を削る、どちらの実装を正とするか決める等)。
- `LNK1104`(exe/lib を開けない)— エディタやゲームが起動中のことが多い。プロセスを勝手に kill しない。
- ビルド環境そのものの問題(MSBuild / SDK が見つからない、ディスク不足など)。

## 修正のルール

- エラーを黙らせるための修正はしない: 警告レベルを下げる、`#pragma warning(disable)`、コードのコメントアウト・
  削除、`static_assert` の削除、関数を空実装にする、などは不可。原因を直す。
- 自分で書いたわけではない既存コードも直してよいが、変更は最小限に。無関係なリファクタはしない。
- CLAUDE.md の規約を守る。特にビルドエラーと絡みやすいもの:
  - `.cpp`/`.h` を新しく作ったら `NanamiEngine.vcxproj`(`Engine/` `Packages/` `Libs/`)か
    `EnviroHunter.vcxproj`(`Assets/`)に手で追加する。`LNK2019`/`LNK2001` はまずこれを疑う。
  - Editor 構成のエンジンは DLL。ゲーム側のリンクで `__imp_` 付きのシンボルが未解決なら、エンジンのクラス / 関数に
    `NANAMI_API` が無い (`python tools/engine_api/add_nanami_api.py` を実行。`--check` で一覧)。エンジン DLL 側で
    `C2280` / `C2027` (暗黙のコピー・デストラクタの実体化) が出たらコピーを `= delete` するか、入れ子の集成体なら
    `NANAMI_NO_API` を付ける(CLAUDE.md の冒頭参照)。
  - per-file `<ClCompile>` に構成依存の設定を書かない。`<AdditionalOptions>` には `/execution-charset:utf-8` を残す。
  - `NANAMI_REGISTER_TYPE` 等の登録マクロは `.cpp` 末尾、`CEREAL_CLASS_VERSION` はヘッダ。`CEREAL_REGISTER_TYPE` /
    `CEREAL_REGISTER_POLYMORPHIC_RELATION` を直接書かない (`tools/serialization/migrate_register_macros.py` で置き換える)。
  - `Engine/` `Packages/` から `Assets/` を include しない。エンジンヘッダに DxLib を出さない。
  - rxcpp は `Packages/R4` 経由。
- ソースは UTF-8 (BOM 付き) のまま編集する。
- 変更をコミットはしない。

## 報告

終わったら日本語で短く報告する。

- 結果(成功/中断)、構成、ビルド回数。
- 直したエラーと変更したファイル(`path:line`)を 1 行ずつ。
- 中断した場合は残っているエラーと、止めた理由。
