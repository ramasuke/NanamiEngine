# Effekseer の /MD 再ビルド (HotReload 段階 0)

`Libs/プロジェクトに追加すべきファイル_VC用/` の Effekseer 系 8 lib (`Effekseer` / `EffekseerRendererDX9` /
`EffekseerRendererDX11` / `EffekseerForDXLib` × Release / Debug、`*_vs2019_x64(_d).lib`) は **静的 CRT (/MT, `LIBCMT`) 版しか無い**。
エンジンを DLL に分けるには全モジュールで CRT を共有する /MD が必須で、MT の lib を混ぜると
`LNK2038: RuntimeLibrary の不一致` になる (docs/HotReload.md §1)。DxLib 本体は `_DLL` 定義で MD 版を自動選択するので作業不要。

`*_vs2019_x64_MD.lib` / `*_MDd.lib` の 8 本は **2026-09-25 に再ビルドしてコミット済み**。作り直すときだけ以下を実行する。

## 上流の版 (確認済み)

| | 版 | 根拠 |
|---|---|---|
| Effekseer | タグ **`170e`** (2023-05-24) | 同梱 `Effekseer.h` と `Dev/Cpp/Effekseer/Effekseer.h` が完全一致 (170/170a/170b は 28 行、1.7.2.0/1.7.3.0 は 47 行以上の差) |
| EffekseerForDXLib | ブランチ `17x` のコミット **`796064f1`** (2022-11-26 "Update to 3.24") | 1.7 系のタグは無い (タグは `120_316d` まで)。同梱 `EffekseerForDXLib.h` と関数宣言が一致する最後のコミット。次の `656ad894` で `UpdateEffekseer2D/3D(float deltaFrame)` に変わり、`78ffa976` 以降は `EffekseerRendererCommon` lib も要る (1.7.2+ 向け) |
| DxLib | 3.25 (同梱パッケージ) | `DxLib.h` の `DXLIB_VERSION` |

EffekseerForDXLib の GitHub Releases は `120_316d` で止まっていて、1.7 系の配布 zip は GitHub には無い。MD 版 lib の公式配布も確認できなかったので再ビルドする。

## 手順 (Windows)

要: Visual Studio 2022 (MSBuild、MSVC、`dumpbin`、同梱 CMake)、git。Python / vcvars は不要。

```
powershell -ExecutionPolicy Bypass -File tools\effekseer_md\build_effekseer_md.ps1 -Work D:\efk_md
```

スクリプトがやること (2026-09-25 に全工程を実行して確認済み):

1. `Effekseer` (`170e`, shallow + submodule) と `EffekseerForDXLib` (`796064f1`) を `-Work` に clone。
2. `-Work` 直下に `Directory.Build.targets` を置き、その下の全 MSBuild の `RuntimeLibrary` を MD / MDd に強制する (保険)。
3. Effekseer は **CMake を直接** `USE_MSVC_RUNTIME_LIBRARY_DLL=ON` で構成し、`Effekseer` / `EffekseerRendererDX9` /
   `EffekseerRendererDX11` の 3 ターゲットだけを Debug / Release でビルドする。
   上流の `release_dxlib.bat` は VS2019 のパスを決め打ちし、Win32 も含む 4 通りを `USE_MSVC_RUNTIME_LIBRARY_DLL=OFF` で組むので使わない。
   `EffekseerRendererCommon` は 170e では Unity プラグインのときだけ lib になる (それ以外は各レンダラに含まれる) ので 4 種類の lib で足りる。
4. EffekseerForDXLib は同梱パッケージの `*.h` を `Dev\include` に置き、`Dev\EffekseerForDXLib\EffekseerForDXLib.h` を
   **同梱のパッチ済みヘッダで上書き**してから `Dev\EffekseerForDXLib\EffekseerForDXLib_vs2022.vcxproj` を x64 Debug / Release でビルドする。
   - ヘッダを差し替えないと、出来た lib の `#pragma comment(lib)` が MT 版 `Effekseer_vs2019_x64.lib` / `DxLib_*_MT.lib` を指す
     (`_DLL` 定義時に `_MD` 名を選ぶのはパッチ側の分岐)。
   - `.sln` ではなく `.vcxproj` を組む。sln に入っている Sample exe は DxLib の lib が無くてリンクに失敗する。
   - 上流の `copy_package.py` (`EffekseerRuntime_DXLib/Compiled/lib/VS2019(WIN64)/...` 前提) は使わない。
5. `dumpbin /DIRECTIVES` で `DEFAULTLIB:MSVCRT(D)` を参照し `LIBCMT` を参照しないことを確かめ、`*_vs2019_x64_MD.lib` / `*_MDd.lib` の名前で
   `Libs/...` にコピーする。DxLib のヘッダは `/NODEFAULTLIB:libcmt(d).lib` も出すので、検査は `(?<!NO)DEFAULTLIB:` で見る。

その後:

- エンジン側は `EffekseerForDXLib.h` (同梱、パッチ済み) が `_DLL` 定義時にこの `_MD` 名を自動リンクする。
  `NanamiEngine.props` の `NanamiUseDynamicCrt` を `true` にする (または `-p:NanamiUseDynamicCrt=true`) と /MD になる。
- 4 構成 (Editor / Game × Debug / Release) をビルドし、既存の全エフェクトが従来通り描けることを確認する。

## 注意

- スクリプトは **UTF-8 BOM 付き** で保存する。Windows PowerShell 5.1 は BOM 無しだと日本語コメントを ANSI として読み、
  `The string is missing the terminator` で止まる。
- PowerShell から MSBuild に `-v:minimal` を渡すときは `"-v:minimal"` と引用する (引用しないと分割されて `MSB1016`)。
- 上流の EffekseerForDXLib は `DXLib_VC/` に DxLib パッケージを置く前提だが、本スクリプトは `Dev\include` に直接ヘッダを置くので不要。
- MD 版の名前は `_vs2019_x64_MD(d)` に固定している (実際のツールセットは v143)。`EffekseerForDXLib.h` の
  `_MSC_VER >= 1920` 分岐がその名前を見るため。v142 と v143 の静的 lib は互換。
- /MD のゲーム版 (Release) は VC++ ランタイム DLL (`msvcp140.dll` / `vcruntime140*.dll`) が要る。
  `NanamiEngine.Game.props` の `NanamiCopyCrtRedist` が `$(VCToolsRedistDir)` から出力先にコピーし、`GameBuilder` が exe と一緒に
  配布フォルダへ持っていく。Debug の CRT は再配布不可なので開発機の System32 に任せる。
