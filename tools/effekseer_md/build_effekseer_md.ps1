<#
.SYNOPSIS
  Effekseer / EffekseerForDXLib を /MD (MultiThreadedDLL) で再ビルドし、*_vs2019_x64_MD(d).lib として Libs に置く。
  docs/HotReload.md §1 (段階 0)。tools/effekseer_md/README.md を先に読むこと。

.DESCRIPTION
  1. Effekseer (タグ) と EffekseerForDXLib (コミット) を作業フォルダに clone する
  2. 作業フォルダ直下に Directory.Build.targets を置き、その下で走る全 MSBuild の RuntimeLibrary を MD/MDd に強制する (保険)
  3. Effekseer: CMake を USE_MSVC_RUNTIME_LIBRARY_DLL=ON で構成し、Effekseer / EffekseerRendererDX9 / EffekseerRendererDX11 だけビルドする
     (上流の release_dxlib.bat は VS2019 決め打ちで Win32 も含む 4 通りを MT で組むので使わない)
  4. EffekseerForDXLib: 同梱パッケージの *.h を Dev\include に置き、Dev\EffekseerForDXLib\EffekseerForDXLib.h を
     同梱のパッチ済みヘッダ (_DLL のとき *_MD(d).lib を自動リンク) で上書きしてから lib の vcxproj だけをビルドする
     (sln だと Sample exe のリンクで DxLib の lib が無くて失敗する。ヘッダを差し替えないと出来た lib の
      #pragma comment(lib) が MT 版 Effekseer_vs2019_x64.lib を指す)
  5. できた lib を dumpbin /DIRECTIVES で確認し (DEFAULTLIB:MSVCRT(D) であり LIBCMT を参照しないこと。
     DxLib ヘッダが出す /NODEFAULTLIB:libcmt(d).lib は除外して見る)、_MD / _MDd を付けて -DestDir にコピーする

  2026-09-25 に Windows (VS2022 17.x, MSVC 14.44, CMake 3.31 (VS 同梱), Python 3.13) で全工程を確認済み。

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File tools\effekseer_md\build_effekseer_md.ps1 -Work D:\efk_md
  powershell -ExecutionPolicy Bypass -File tools\effekseer_md\build_effekseer_md.ps1 -Work D:\efk_md -SkipClone -SkipBuild   # 集める工程だけ
#>
param(
    [string]$Work = (Join-Path $env:TEMP "nanami_effekseer_md"),
    # 同梱 Effekseer.h と完全一致するタグ (Dev/Cpp/Effekseer/Effekseer.h を diff して確認済み)
    [string]$EffekseerTag = "170e",
    # EffekseerForDXLib に 1.7 系のタグは無い。17x ブランチのうち同梱 EffekseerForDXLib.h と宣言が一致する最後のコミット
    # (2022-11-26 "Update to 3.24")。次の 656ad894 で UpdateEffekseer2D/3D の引数が変わり、78ffa976 以降は EffekseerRendererCommon lib も要る
    [string]$ForDxLibRev = "796064f1",
    [string]$DxLibPackageDir = (Join-Path $PSScriptRoot "..\..\Libs\プロジェクトに追加すべきファイル_VC用"),
    [string]$DestDir = (Join-Path $PSScriptRoot "..\..\Libs\プロジェクトに追加すべきファイル_VC用"),
    [switch]$SkipClone,
    [switch]$SkipBuild
)
$ErrorActionPreference = "Stop"
$DxLibPackageDir = (Resolve-Path $DxLibPackageDir).Path
$DestDir         = (Resolve-Path $DestDir).Path
New-Item -ItemType Directory -Force -Path $Work | Out-Null
$Work = (Resolve-Path $Work).Path

function Find-Tool([string]$name, [string[]]$candidates) {
    foreach ($c in $candidates) { if ($c -and (Test-Path $c)) { return $c } }
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    throw "$name が見つかりません"
}
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsRoot  = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
$msbuild = Find-Tool "MSBuild.exe" @("$vsRoot\MSBuild\Current\Bin\MSBuild.exe")
# CMake は PATH に無くても VS 同梱のものでよい
$cmake   = Find-Tool "cmake.exe" @("$vsRoot\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe")
$dumpbin = Get-ChildItem "$vsRoot\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe" | Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName

# 2. RuntimeLibrary を MD に強制する保険 (この下で走る全 MSBuild に効く)
Copy-Item (Join-Path $PSScriptRoot "Directory.Build.targets") (Join-Path $Work "Directory.Build.targets") -Force

# 1. clone
$efk    = Join-Path $Work "Effekseer"
$efkDx  = Join-Path $Work "EffekseerForDXLib"
if (-not $SkipClone) {
    if (-not (Test-Path $efk)) {
        # サブモジュールは 3rdParty 参照のために要る (shallow で十分)
        git clone --depth 1 --branch $EffekseerTag --recursive --shallow-submodules -j8 https://github.com/effekseer/Effekseer.git $efk
        if ($LASTEXITCODE -ne 0) { throw "git clone Effekseer failed" }
    }
    if (-not (Test-Path $efkDx)) {
        git clone https://github.com/effekseer/EffekseerForDXLib.git $efkDx
        if ($LASTEXITCODE -ne 0) { throw "git clone EffekseerForDXLib failed" }
        git -C $efkDx checkout -q $ForDxLibRev
        if ($LASTEXITCODE -ne 0) { throw "git checkout $ForDxLibRev failed" }
    }
}

# 3. Effekseer 本体 + DX9/DX11 レンダラ (CMake の USE_MSVC_RUNTIME_LIBRARY_DLL で MD にする)
$efkBuild = Join-Path $efk "build_x64_md"
if (-not $SkipBuild) {
    & $cmake -S $efk -B $efkBuild -G "Visual Studio 17 2022" -A x64 `
        -D USE_MSVC_RUNTIME_LIBRARY_DLL:BOOL=ON -D BUILD_EXAMPLES:BOOL=OFF -D USE_XAUDIO2:BOOL=ON
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }
    foreach ($cfg in @("Debug", "Release")) {
        # PowerShell は "-v:minimal" を引用しないと分割して MSB1016 になる
        & $cmake --build $efkBuild --config $cfg --target Effekseer EffekseerRendererDX9 EffekseerRendererDX11 -- "-m" "-v:minimal" "-nologo"
        if ($LASTEXITCODE -ne 0) { throw "cmake build $cfg failed" }
    }

    # 4. EffekseerForDXLib (DxLib と Effekseer のヘッダは同梱パッケージのもの = 上流と同一版)
    $inc = Join-Path $efkDx "Dev\include"
    New-Item -ItemType Directory -Force -Path $inc | Out-Null
    Copy-Item (Join-Path $DxLibPackageDir "*.h") $inc -Force
    Copy-Item (Join-Path $DxLibPackageDir "EffekseerForDXLib.h") (Join-Path $efkDx "Dev\EffekseerForDXLib\EffekseerForDXLib.h") -Force
    $proj = Join-Path $efkDx "Dev\EffekseerForDXLib\EffekseerForDXLib_vs2022.vcxproj"
    if (-not (Test-Path $proj)) { throw "$proj が見つかりません" }
    foreach ($cfg in @("Debug", "Release")) {
        & $msbuild $proj -p:Configuration=$cfg -p:Platform=x64 "-p:PreferredToolArchitecture=x64" "-m" "-v:minimal" "-nologo"
        if ($LASTEXITCODE -ne 0) { throw "MSBuild EffekseerForDXLib $cfg failed" }
    }
}

# 5. 集めて確認して _MD / _MDd を付けてコピーする
$cpp = Join-Path $efkBuild "Dev\Cpp"
$map = [ordered]@{
    "Effekseer_vs2019_x64_MD.lib"              = "$cpp\Effekseer\Release\Effekseer.lib"
    "Effekseer_vs2019_x64_MDd.lib"             = "$cpp\Effekseer\Debug\Effekseer.lib"
    "EffekseerRendererDX9_vs2019_x64_MD.lib"   = "$cpp\EffekseerRendererDX9\Release\EffekseerRendererDX9.lib"
    "EffekseerRendererDX9_vs2019_x64_MDd.lib"  = "$cpp\EffekseerRendererDX9\Debug\EffekseerRendererDX9.lib"
    "EffekseerRendererDX11_vs2019_x64_MD.lib"  = "$cpp\EffekseerRendererDX11\Release\EffekseerRendererDX11.lib"
    "EffekseerRendererDX11_vs2019_x64_MDd.lib" = "$cpp\EffekseerRendererDX11\Debug\EffekseerRendererDX11.lib"
    "EffekseerForDXLib_vs2019_x64_MD.lib"      = "$efkDx\Dev\lib\EffekseerForDXLib_vs2022_x64.lib"
    "EffekseerForDXLib_vs2019_x64_MDd.lib"     = "$efkDx\Dev\lib\EffekseerForDXLib_vs2022_x64_d.lib"
}
$copied = 0
foreach ($dest in $map.Keys) {
    $src = $map[$dest]
    if (-not (Test-Path $src)) { Write-Warning "$src がビルド出力に見つかりません"; continue }
    $directives = & $dumpbin /DIRECTIVES $src | Out-String
    $expected = if ($dest -like "*_MDd.lib") { "MSVCRTD" } else { "MSVCRT" }
    # (?<!NO): DxLib のヘッダは /NODEFAULTLIB:libcmt(d).lib も出すので、それを LIBCMT 参照と誤認しない
    if ($directives -notmatch "(?<!NO)DEFAULTLIB:`"?$expected") { throw "$src は $expected を参照していません (MT のまま)" }
    if ($directives -match "(?<!NO)DEFAULTLIB:`"?LIBCMT") { throw "$src に LIBCMT (静的 CRT) の参照が残っています" }
    Copy-Item $src (Join-Path $DestDir $dest) -Force
    Write-Host ("  {0,-42} <- {1}  ({2})" -f $dest, (Split-Path $src -Leaf), $expected)
    $copied++
}
Write-Host "copied $copied / 8 libs to $DestDir"
if ($copied -ne 8) { exit 1 }
