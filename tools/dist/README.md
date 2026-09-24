# tools/dist — 配信マニフェストのビルドとアップロード

運営型アセット配信の**リリース側**ツール。`Assets/` を走査して `manifest.json`（配信される全
ファイルの一覧）を書き出し、配信先（Cloudflare R2）へ上げる。これを読む**クライアント側**は
`Packages/AssetUpdater/`。

```
python -m tools.dist build --version 1.1.0 [--base-url URL] [--out FILE]
python -m tools.dist upload [manifest.json] [--dry-run] [--no-release] [--remote REMOTE]
python -m tools.dist show <manifest.json>
python -m tools.dist diff <installed.json> <manifest.json>
python tools/dist/selftest.py
```

## 版番号

- `--version`（Asset Dist の Version）は**配信するアセットの版**。`[0-9A-Za-z._-]+` で、リリースのたびに上げる
  （`1.0.1`, `1.0.2`, …）。同じ版番号で別の内容は上げられない。
- `requiredClientVersion` は**これ未満のゲーム本体には更新を当てない**という下限。ゲーム本体の版は
  Build Settings の **Client Version**（`ProjectConfig/Build/Runtime/ClientVersion.json`、書き出したゲームに同梱）で、
  `--required-client-version` を省略するとこの値が入る。上げるのは exe ごと新しい zip を配るときだけ。
- 比較はドット区切りの数値（`1.9.0` < `1.10.0`、`1.0` = `1.0.0`、数字以外の文字は無視）。

## 配信先の設定

`tools/dist/dist_config.json` に置き、コードにはハードコードしない。

| キー | 値 | 意味 |
|---|---|---|
| `remote` | `r2:nanami-assets` | rclone の remote とバケット |
| `publicBaseUrl` | `https://pub-….r2.dev` | プレイヤーが読みに来る公開 URL |
| `rclone` | `rclone` | rclone の実行ファイル |

`build` の `baseUrl` 既定値は `publicBaseUrl + /files/`。

rclone の設定（鍵）は**リポジトリの外**の `%APPDATA%\rclone\rclone.conf` にある（remote 名 `r2`）。
トークンはバケット限定の Object Read & Write なので、`no_check_bucket = true` が必須
（無いとバケットの存在確認が権限エラーになる）。

> **r2.dev は開発用**（Cloudflare 公式に「レート制限あり・本番用途不可」）。プレイヤーに配る前に
> 独自ドメインを接続し、`publicBaseUrl` と、exe に埋め込んだクライアントの `MANIFEST_URL` の
> **両方**を変えること。後者は exe の更新になるので、最初の公開リリースより前に決めておく。

## サーバー側のレイアウト

```
nanami-assets/
├── manifest.json             唯一の可変ファイル。差し替え = リリース   Cache-Control: no-cache
├── manifest-<version>.json   版ごとの控え。不変                        Cache-Control: 1年 immutable
└── files/<sha256>            本体と .meta を中身のハッシュ名で。不変   Cache-Control: 1年 immutable
```

クライアントは `manifest.json` → 手元の `installed.json` と比較 → `baseUrl + <hash>` で取得 →
エントリの `path` に置く。

`installed.json` はエディタの Build Settings でゲームを書き出すとき（*Asset Updates > Write installed.json*、既定でオン）に
`GameBuilder` が出力先の直下に書く。中身は**書き出した `Assets/` を実際にハッシュした一覧**（`version` は `local`）で、
配信中のどの版とも一致しなくてよい。初回起動で `manifest.json` との差分だけが落ちてくる。オフにすると `installed.json`
を消すので、そのゲームは更新を一切確認しない（書き出したものを手元で試すとき向け。オンのままだと、配信より新しい
ローカルのアセットが配信版に戻される）。

- **URL は常に 16 進の ASCII** なので、日本語のアセットパスを percent-encode する必要が無い
- **同じ中身は1個のブロブになる**（`internal_ground_ao_texture.jpeg` が6フォルダにある等で約 50MB 減る）
- 一度置いたブロブは書き換わらないので、キャッシュ事故が起きず、ロールバックもマニフェストの差し替えだけで済む

## リリース手順

```bash
python -m tools.dist build --version 1.1.0
python -m tools.dist upload --dry-run      # 何が上がるかの表示と、中身の再検証だけ
python -m tools.dist upload                # 差分ブロブ → manifest-1.1.0.json → manifest.json（最後）
```

エディタのツールバーにある **Asset Dist** からも同じ 3 つを実行できる（`Packages/AssetUpdater/Editor/AssetDistributionToolbarWidget`）。
Release は同じ版の Build Manifest がそのセッションで成功した後だけ押せて、押すと確認ダイアログが出る。出力はウィジェット内のログに表示される。

`upload` がやること:

1. `manifest-<version>.json` が**別の内容で**すでにあれば、何も上げずに中止（同じ版番号の使い回しを防ぐ）
2. remote の `files/` を一覧して、**まだ無いハッシュだけ**を選ぶ
3. 一時フォルダへコピーしながら SHA-256 を取り直す。`build` 後に編集されたファイルがあれば中止
4. `rclone copy` で `files/` へ上げる
5. 参照するブロブが全部そろったことを確かめてから `manifest-<version>.json` → `manifest.json`

前回のマニフェストに依存しないので、途中で止まっても**再実行すれば残りだけ上がる**。
`--no-release` は 5 の `manifest.json` の差し替えだけをしない（先に上げておいて後で切り替える用）。

ロールバック:

```bash
rclone copyto r2:nanami-assets/manifest-1.0.0.json r2:nanami-assets/manifest.json --header-upload "Cache-Control: no-cache"
```

## エントリは2種類ある

```json
{
  "guid": "3F2A9C10-...", "path": "Assets/Art/Models/Hyena.mv1",
  "hash": "c5d0...", "size": 2560000,
  "metaHash": "91ff...", "metaSize": 1284
}
```

**アセット** — `.meta` を持つファイル。本体と `.meta` を1エントリに畳んである。`.meta` は guid と
`contentPath_` の出どころ（`AssetFactory::RegisterLoader` が `filePath + ".meta"` から実体を作る）
なので、片方だけ更新すると解決できなくなる。必ずペアで動かす。

**随伴ファイル** — `.meta` を持たないが実行時に要るファイル。`guid` / `metaHash` が空になる。

```json
{ "guid": "", "path": "Assets/Art/Models/T-Rex.fbm/trexDiff.jpg",
  "hash": "921e...", "size": 190, "metaHash": "", "metaSize": 0 }
```

これらは**ファイルの中から相対パスで参照されている**ので、guid で識別できない：

- `.efkefc` が参照する `.efkmodel` とテクスチャ
- `.mv1` が `<名前>.fbm/` 相対で参照するテクスチャ（`Knight D Pelegrini.fbm/Knight_diffuse.png` 等）
- コンパイル済みシェーダ `Tree_VS.vso` / `Tree_PS.pso`
- `.mat` / `.mtl`

> **「`.meta` が無ければ配信対象外」は誤り。** それをやると随伴ファイルが丸ごと落ちて、
> エフェクトとモデルがテクスチャ無しで描画される。除外は開発専用のものだけを名指しする
> denylist（`manifest.EXCLUDE_DIRS` / `EXCLUDE_SUFFIXES` / `EXCLUDE_NAMES`）にしてある。

除外されるのは `Assets/Scripts/`（exe にコンパイルされる）、**どの階層でも `_Source/` の下**
（変換前の原本置き場。実行時に参照されるのは変換後のファイルの隣にある同名コピーの方）、
`*.fbx` / `*.blend` / `*.blend1` / `*.efkproj`（原本）、`*.h` / `*.cpp` / `*.bak`、`desktop.ini` など。

> `.blend` には**作業した PC のユーザー名とフルパス**が入っている。以前は
> `Assets/Art/Models/Nature/Trees/_Source/Trees.blend` が配信対象に紛れていた（2026-09-18 に除外）。
> 除外ルールを緩めたら、配信対象を展開して `e29sw` / `Users\` などを UTF-8・UTF-16LE・CP932 で
> 検索し直すこと（`.mv1` は `tools/model/mv1.decode` で展開してから）。
`ProjectConfig/` と `LocalPrefs/` は `Assets/` の外なので対象にならない（どちらも配らない）。

## 参照チェック（`build` が止まる条件）

`build` は、配信する `.efkefc`（INFO チャンク）と `.mv1`（展開した本文のテクスチャパス）が参照するファイルを、
そのファイルの場所から解決する（`tools/dist/refs.py`）。**この PC に実在するのに配信されない**もの
（`Assets/` の外、または上の除外に当たるもの）を指していたら、一覧を出して **manifest を書かずに終了コード 1** で止まる。
開発 PC では表示されるのに、プレイヤーの PC では見つからないケースだからである。
2026-09-18 には、使用中のエフェクト 8 件が `../../../../../Effekseer素材/...`（デスクトップの素材フォルダ）を
参照していた。これらは隣にある同一のコピーを指すように、1.7.3 CUI で再コンパイルした。

どこにも実在しない参照は報告しない。開発 PC でも壊れているので配信の問題ではないうえ、`.mv1` は元 FBX の
絶対パス（`C:\Tarisland - Dragon\X.png`）を相対パスと並べて持っていることがあるため。
中身を読めなかったファイルは WARNING として出すだけで、止めない。

## フォントはパッチで差し替えない（`upload` が止まる条件）

クライアント（`Packages/AssetUpdater`）は、**タイトル画面でダウンロードしてすぐ `Assets/` に適用し、再起動する**。
ところがフォント（`.ttf` / `.otf` / `.ttc`）は、エンジンの `TtfFontFile` が起動時に `AddFontResourceEx` で
Windows に登録し、終了まで外さないので、実行中は置き換えられない。適用は「全部成功するか、何も変わらないか」
なので、フォントを変えたリリースは**全員の適用が失敗し続ける**。

そこで `upload` は、公開中の `manifest.json` と比べて、**既存のフォントが変更・削除されているのに
`requiredClientVersion` が公開中のものより上がっていない**リリースを拒否する（dry-run でも）。
フォントを変えるときは、Build Settings の Client Version を上げた新しい zip を配ったうえで、
`build --required-client-version <新しい版>` で作り直す。
フォントの**追加**は問題にならない（プレイヤーの PC にまだ無いファイルは開かれていないので書ける）。

## 現状の規模（2026-09-18）

```
assets       1730   (.meta あり)
companions     82   (.meta 無し / 相対参照される随伴ファイル)
excluded      811   (開発専用)
entries      1812 / total 1.76 GB / ブロブ 3146 個 1.71 GB（重複除去後）
manifest.json 約 600KB（gzip で約 210KB）
```

初回スキャンは約 6 秒、参照チェックの `.mv1` 展開を合わせると約 22 秒。2 回目以降は mtime + size をキーにした
SHA-256 キャッシュと、SHA-256 をキーにした参照リストのキャッシュ（どちらも既定 `<repo>/.manifest_hash_cache.json`、
`.gitignore` 済み）が効いて約 4 秒。
`--no-cache` で毎回読み直せる。`upload` の中身の再検証はキャッシュを使わず実バイトで行う。

## 既知の注意点

- **非ASCII のパスが 74 件ある**（`Assets/Art/Effect/Slash/Parts/ひし形比率0.0.png` など）。
  ダウンロード URL はハッシュなので影響しないが、**クライアントがローカルへ書くとき**、
  UTF-8 の `path` をそのまま `std::filesystem::path` に渡すと ACP（CP932）扱いで化ける。
  UTF-8 → UTF-16 変換してから使うこと。`build` が件数を NOTE として出す。
- **CP932 のまま残っている `.meta` が 71 件ある**。`/execution-charset:utf-8` を入れる前の
  エンジンが書いたもの。`path` はファイルシステムから取るので配信には影響しないが、
  `read_guid` は UTF-8 → CP932 のフォールバックで読んでいる。`build` が件数を NOTE として出す。
- ダウンロード実装時、**随伴エントリでは `.meta` を取りに行ってはいけない**
  （`metaHash` が空のときは本体だけ落とす）。
- **Python の `urllib` の既定 User-Agent（`Python-urllib/3.x`）は r2.dev で 403** になる。
  確認用スクリプトでは User-Agent を付けること。curl・User-Agent なし・クライアントの
  `NanamiEngine AssetUpdater` は 200。
- **rclone の `--immutable` は `copyto`（1ファイル）では効かない**（中身が違っても上書きする）。
  版マニフェストの上書き防止は `upload.py` 側で「既にあるか・中身が同じか」を確かめている。
- バケット限定トークンだと `rclone purge` などで `GetBucketVersioning` の 403 ERROR が出るが、
  処理自体は成功する。
- `manifest.json` / `manifest-*.json` / `installed.json` はコミットしない（リリース成果物）。`.gitignore` 済み。

## selftest

`python tools/dist/selftest.py`（36 チェック）。第三者ライブラリ不要で、rclone もネットワークも使わない。

`manifest.py` / `upload.py` / `refs.py` を触ったら必ず走らせること。とくに **stage 0** は出力 JSON のキー名が
`Packages/AssetUpdater/Manifest/AssetManifest.cpp` の `TryParse` と一致するかを見ている。
ここがずれるとクライアントは例外も出さず「差分 0 件」になる。

R2 との実際のやりとりを確かめたいときは、`--remote r2:nanami-assets/_test` と `--repo-root` に
小さな木を渡して上げ、終わったら `rclone purge r2:nanami-assets/_test` で消す。
