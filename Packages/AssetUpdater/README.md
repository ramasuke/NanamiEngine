# AssetUpdater — 運営型アセット配信

`Assets/` の差分をゲームの起動時に配る仕組み。2 つの側がある:

- **リリース側** (`Dist/` + `Editor/`, エディタ専用) — `Assets/` を走査して `manifest.json`（配信される全ファイルの一覧）を書き出し、
  配信先（Cloudflare R2）へ上げる。ツールバーの **Asset Dist** から動かす。
- **クライアント側** (`Http/`, `Install/`, `Task/`, ...) — 書き出したゲームがタイトル画面で `manifest.json` を取り、
  手元の `installed.json` と比べて差分だけを落とし、`Assets/` に適用する。

リリース側は以前 `tools/dist`（Python）だったものを C++ に移したもので、Python は要らない（rclone だけ使う）。

## Asset Dist ウィジェット

`ProjectConfig/Build/AssetDistribution/` があるプロジェクトのエディタだけに出る。

| ボタン | やること |
|---|---|
| **Build Manifest** | `Assets/` を走査して `<project>/manifest.json` を書く |
| **Upload (Dry Run)** | 何を上げるかの表示と、上げるファイルの中身の再検証だけ |
| **Upload (Release)** | 差分ブロブ → `manifest-<version>.json` → `manifest.json`（最後）。同じ版の Build がこのセッションで成功した後だけ押せて、確認ダイアログが出る |
| **Diff vs Live** | 公開中の `manifest.json` と手元の `manifest.json` の差分（プレイヤーが落とす量）。イベントのデータだけが変わっているかの確認に |
| **Self Test** | 正しさのゲート（下記） |

処理はワーカースレッドで動き（`Dist/DistJob`）、出力はウィジェット内のログに出る。Cancel はファイルの区切りか、実行中の rclone を止めた時点で終わる。

## 版番号

- **Version** は**配信するアセットの版**。`[0-9A-Za-z._-]+` で、リリースのたびに上げる（`1.0.1`, `1.0.2`, …）。同じ版番号で別の内容は上げられない。
- **Required Client Version** は**これ未満のゲーム本体には更新を当てない**という下限。ゲーム本体の版は Build Settings の
  **Client Version**（`ProjectConfig/Build/Runtime/ClientVersion.json`、書き出したゲームに同梱）で、空欄ならこの値が入る。
  上げるのは exe ごと新しい zip を配るときだけ。
- 比較はドット区切りの数値（`1.9.0` < `1.10.0`、`1.0` = `1.0.0`、数字以外の文字は無視。`Text/VersionString`）。

## 配信先の設定

`ProjectConfig/Build/AssetDistribution/`（ウィジェットの *Destination*）に置き、コードにはハードコードしない。

| キー | 値 | 意味 |
|---|---|---|
| `Remote` | `r2:nanami-assets` | rclone の remote とバケット |
| `PublicBaseUrl` | `https://pub-….r2.dev` | プレイヤーが読みに来る公開 URL。manifest の `baseUrl` は `<これ>/files/` |
| `Rclone` | `rclone` | rclone の実行ファイル（PATH 上の名前でもよい） |

rclone の設定（鍵）は**リポジトリの外**の `%APPDATA%\rclone\rclone.conf` にある（remote 名 `r2`）。
トークンはバケット限定の Object Read & Write なので、`no_check_bucket = true` が必須
（無いとバケットの存在確認が権限エラーになる）。

> **r2.dev は開発用**（Cloudflare 公式に「レート制限あり・本番用途不可」）。プレイヤーに配る前に
> 独自ドメインを接続し、`PublicBaseUrl` と、exe に埋め込んだクライアントの `MANIFEST_URL` の
> **両方**を変えること。後者は exe の更新になるので、最初の公開リリースより前に決めておく。

## サーバー側のレイアウト

```
nanami-assets/
├── manifest.json             唯一の可変ファイル。差し替え = リリース   Cache-Control: no-cache
├── manifest-<version>.json   版ごとの控え。不変                        Cache-Control: 1年 immutable
└── files/<sha256>            本体と .meta を中身のハッシュ名で。不変   Cache-Control: 1年 immutable
```

クライアントは `manifest.json` → 手元の `installed.json` と比較 → `baseUrl + <hash>` で取得 → エントリの `path` に置く。

`installed.json` はエディタの Build Settings でゲームを書き出すとき（*Asset Updates > Write installed.json*、既定でオン）に
`GameBuilder` が出力先の直下に書く（`Manifest/InstalledState`）。中身は**書き出した `Assets/` を実際にハッシュした一覧**
（`version` は `local`）で、配信中のどの版とも一致しなくてよい。初回起動で `manifest.json` との差分だけが落ちてくる。
オフにすると `installed.json` を消すので、そのゲームは更新を一切確認しない。

- **URL は常に 16 進の ASCII** なので、日本語のアセットパスを percent-encode する必要が無い
- **同じ中身は1個のブロブになる**（`internal_ground_ao_texture.jpeg` が6フォルダにある等で約 50MB 減る）
- 一度置いたブロブは書き換わらないので、キャッシュ事故が起きず、ロールバックもマニフェストの差し替えだけで済む

## Release がやること

1. `manifest-<version>.json` が**別の内容で**すでにあれば、何も上げずに中止（同じ版番号の使い回しを防ぐ）
2. 公開中の `manifest.json` と比べてフォントの差し替えを拒否する（下記）
3. remote の `files/` を一覧して、**まだ無いハッシュだけ**を選ぶ
4. 一時フォルダへコピーしながら SHA-256 を取り直す。Build 後に編集されたファイルがあれば中止
5. `rclone copy` で `files/` へ上げる
6. 参照するブロブが全部そろったことを確かめてから `manifest-<version>.json` → `manifest.json`

前回のマニフェストに依存しないので、途中で止まっても**再実行すれば残りだけ上がる**。

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
ファイルの中から相対パスで参照されているので、guid で識別できない:

- `.efkefc` が参照する `.efkmodel` とテクスチャ
- `.mv1` が `<名前>.fbm/` 相対で参照するテクスチャ
- コンパイル済みシェーダ `Tree_VS.vso` / `Tree_PS.pso`
- `.mat` / `.mtl`

> **「`.meta` が無ければ配信対象外」は誤り。** それをやると随伴ファイルが丸ごと落ちて、
> エフェクトとモデルがテクスチャ無しで描画される。除外は開発専用のものだけを名指しする
> denylist（`Dist/DistExclusion.cpp`。`GameBuilder` の書き出しも同じ規則）にしてある。

除外されるのは `Assets/Scripts/`（exe にコンパイルされる）、**どの階層でも `_Source/` の下**
（変換前の原本置き場。実行時に参照されるのは変換後のファイルの隣にある同名コピーの方）、
`*.fbx` / `*.blend` / `*.blend1` / `*.efkproj`（原本）、`*.h` / `*.cpp` / `*.bak`、`desktop.ini` など。

> `.blend` には**作業した PC のユーザー名とフルパス**が入っている。除外ルールを緩めたら、配信対象を展開して
> ユーザー名や `Users\` などを UTF-8・UTF-16LE・CP932 で検索し直すこと（`.mv1` は `tools/model/mv1.decode` で展開してから）。
`ProjectConfig/` と `LocalPrefs/` は `Assets/` の外なので対象にならない（どちらも配らない）。

## 参照チェック（Build が止まる条件）

Build は、配信する `.efkefc`（INFO チャンク）と `.mv1`（展開した本文のテクスチャパス）が参照するファイルを、
そのファイルの場所から解決する（`Dist/DistReferences`）。**この PC に実在するのに配信されない**もの
（`Assets/` の外、または上の除外に当たるもの）を指していたら、一覧を出して **manifest を書かずに失敗**する。
開発 PC では表示されるのに、プレイヤーの PC では見つからないケースだからである。
2026-09-18 には、使用中のエフェクト 8 件がデスクトップの素材フォルダを参照していた。

どこにも実在しない参照は報告しない。開発 PC でも壊れているので配信の問題ではないうえ、`.mv1` は元 FBX の
絶対パス（`C:\Tarisland - Dragon\X.png`）を相対パスと並べて持っていることがあるため。
中身を読めなかったファイルは WARNING として出すだけで、止めない。

## フォントはパッチで差し替えない（Upload が止まる条件）

クライアントは、**タイトル画面でダウンロードしてすぐ `Assets/` に適用し、ゲームを終了して起動し直してもらう**。
ところがフォント（`.ttf` / `.otf` / `.ttc`）は、エンジンの `TtfFontFile` が起動時に `AddFontResourceEx` で
Windows に登録し、終了まで外さないので、実行中は置き換えられない。適用は「全部成功するか、何も変わらないか」
なので、フォントを変えたリリースは**全員の適用が失敗し続ける**。

そこで Upload は、公開中の `manifest.json` と比べて、**既存のフォントが変更・削除されているのに
`requiredClientVersion` が公開中のものより上がっていない**リリースを拒否する（Dry Run でも）。
フォントを変えるときは、Build Settings の Client Version を上げた新しい zip を配ったうえで、
Required Client Version をその版にして Build し直す。フォントの**追加**は問題にならない。

## ハッシュキャッシュ

1.76 GB / 1,800 エントリ（2026-09-18）を毎回読み直さないよう、mtime + size をキーにした SHA-256 と、
SHA-256 をキーにした参照リストを `<project>/.manifest_hash_cache.json`（`.gitignore` 済み）に置く。
形式は `tools/dist` 時代と同じなので、そのとき作ったキャッシュもそのまま効く。
Upload の中身の再検証はキャッシュを使わず実バイトで行う。

## 既知の注意点

- **非ASCII のパスがある**（`Assets/Art/Effect/Slash/Parts/ひし形比率0.0.png` など）。
  ダウンロード URL はハッシュなので影響しないが、**クライアントがローカルへ書くとき**、
  UTF-8 の `path` をそのまま `std::filesystem::path` に渡すと ACP（CP932）扱いで化ける。
  UTF-8 → UTF-16 変換してから使うこと（`Text/Utf8`）。Build が件数を NOTE として出す。
- **CP932 のまま残っている `.meta` がある**。`/execution-charset:utf-8` を入れる前のエンジンが書いたもの。
  guid の読み取りは UTF-8 → CP932 のフォールバックで読んでいる。Build が件数を NOTE として出す。
- 随伴エントリでは `.meta` を取りに行ってはいけない（`metaHash` が空のときは本体だけ落とす）。
- r2.dev は Python `urllib` の既定 User-Agent を 403 で弾く。確認用スクリプトでは User-Agent を付けること。
- **rclone の `--immutable` は `copyto`（1ファイル）では効かない**（中身が違っても上書きする）。
  版マニフェストの上書き防止は `Dist/Rclone` 側で「既にあるか・中身が同じか」を確かめている。
- バケット限定トークンだと `rclone purge` などで `GetBucketVersioning` の 403 ERROR が出るが、処理自体は成功する。
- `manifest.json` / `manifest-*.json` / `installed.json` はコミットしない（リリース成果物）。`.gitignore` 済み。

## Self Test

ウィジェットの **Self Test**（`Dist/DistSelfTest`）。rclone もネットワークも使わず、一時フォルダに小さな `Assets/` を作って
manifest の形 / 除外規則 / guid の読み取り / 走査 / 差分 / ハッシュキャッシュ / upload の計画と検証 / 配信されない参照 /
フォントの差し替え拒否 を確かめる。`Dist/` を触ったら必ず走らせること。とくに **stage 0** は出力 JSON のキー名が
`Manifest/AssetManifest.cpp` の `TryParse` と一致するかを見ている。ここがずれるとクライアントは例外も出さず「差分 0 件」になる。
