# Effekseer の /MD 再ビルド (HotReload 段階 0)

`Libs/プロジェクトに追加すべきファイル_VC用/` の Effekseer 系 8 lib (`Effekseer` / `EffekseerRendererDX9` /
`EffekseerRendererDX11` / `EffekseerForDXLib` × Release / Debug、`*_vs2019_x64(_d).lib`) は **静的 CRT (/MT, `LIBCMT`) 版しか無い**。
エンジンを DLL に分けるには全モジュールで CRT を共有する /MD が必須で、MT の lib を混ぜると
`LNK2038: RuntimeLibrary の不一致` になる (docs/HotReload.md §1)。DxLib 本体は `_DLL` 定義で MD 版を自動選択するので作業不要。

## 手順 (Windows)

1. 上流 (`effekseer/Effekseer`, `effekseer/EffekseerForDXLib`) で、同梱 lib と同じ版のタグを選ぶ。
   同梱の `Effekseer.h` と上流の `Dev/Cpp/Effekseer/Effekseer.h` を diff して一致する版にする (ランタイムは 1.7 系、
   `tools/effect/effect_config.json` の `project.runtime_version` 参照)。
2. 実行:
   ```
   powershell -ExecutionPolicy Bypass -File tools\effekseer_md\build_effekseer_md.ps1 -EffekseerTag <tag> -ForDxLibTag <tag> -Work D:\efk_md
   ```
   要: Visual Studio 2022 (MSBuild, vcvars64, dumpbin)、CMake 3.15+、Python 3、git。
   スクリプトは作業フォルダ直下に `Directory.Build.targets` を置いて、その下で走る全 MSBuild の `RuntimeLibrary` を
   MD / MDd に強制する (CMake が生成する .vcxproj にも効く)。上流の `release_dxlib.bat` → `copy_package.py` →
   `Dev/EffekseerForDXLib_vs2022.sln` の順に走らせ、できた lib を `dumpbin /DIRECTIVES` で
   `DEFAULTLIB:MSVCRT(D)` であることを確かめてから `*_vs2019_x64_MD.lib` / `*_MDd.lib` の名前で `Libs/...` にコピーする。
3. エンジン側は `EffekseerForDXLib.h` (同梱、パッチ済み) が `_DLL` 定義時にこの `_MD` 名を自動リンクする。
   `NanamiEngine.props` の `NanamiUseDynamicCrt` を `true` にする (または `-p:NanamiUseDynamicCrt=true`) と /MD になる。
4. 4 構成 (Editor / Game × Debug / Release) をビルドし、既存の全エフェクトが従来通り描けることを確認する。
   通ったら `NanamiUseDynamicCrt` の既定を `true` に変える。

## 注意

- スクリプトは Linux 側で書いたもので **実行は未確認** (2026-09-25)。上流のスクリプト名 (`release_dxlib.bat`, `copy_package.py`,
  `Dev/EffekseerForDXLib_vs20xx.sln`) や出力先が違えば `-SkipClone` / `-SkipBuild` で途中から手で続け、
  分かったことをこの README に追記する。
- 上流は EffekseerForDXLib 直下の `DxLib_VC/` に DxLib のパッケージがある前提。スクリプトは同梱の
  `プロジェクトに追加すべきファイル_VC用` をそこへコピーする (DxLib 3.25)。
- MD 版の名前は `_vs2019_x64_MD(d)` に固定している (実際のツールセットが vs2022 でも)。`EffekseerForDXLib.h` の
  `_MSC_VER >= 1920` 分岐がその名前を見るため。
- /MD のゲーム版 (Release) は VC++ ランタイム DLL (`msvcp140.dll` / `vcruntime140*.dll`) が要る。
  `NanamiEngine.Game.props` の `NanamiCopyCrtRedist` が `$(VCToolsRedistDir)` から出力先にコピーし、`GameBuilder` が exe と一緒に
  配布フォルダへ持っていく。Debug の CRT は再配布不可なので開発機の System32 に任せる。
