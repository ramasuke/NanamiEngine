<#
.SYNOPSIS
  Effekseer / EffekseerForDXLib を /MD (MultiThreadedDLL) で再ビルドし、*_vs2019_x64_MD(d).lib として Libs に置く。
  docs/HotReload.md §1 (段階 0)。tools/effekseer_md/README.md を先に読むこと。

.DESCRIPTION
  1. Effekseer と EffekseerForDXLib を指定タグで clone する (同じ親フォルダに並べる。上流の README の前提)
  2. 作業フォルダ直下に Directory.Build.targets を置き、その下で走る全 MSBuild の RuntimeLibrary を MD/MDd に強制する
  3. Effekseer\release_dxlib.bat → EffekseerForDXLib\copy_package.py → Dev\EffekseerForDXLib_vs2022.sln を順に実行する
  4. できた lib を dumpbin /DIRECTIVES で確認し (DEFAULTLIB:MSVCRT(D) であること)、_MD / _MDd を付けて -DestDir にコピーする

  上流のスクリプト名や出力先が変わっていたら、-SkipClone / -SkipBuild で途中から手で続けられる。
  このスクリプトは Linux 側で書いたもので、実行は未確認 (2026-09-25)。止まった箇所を README に追記すること。

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File tools\effekseer_md\build_effekseer_md.ps1 -EffekseerTag 170h -ForDxLibTag <tag> -Work D:\efk_md
#>
param(
    [string]$Work = (Join-Path $env:TEMP "nanami_effekseer_md"),
    [Parameter(Mandatory = $true)][string]$EffekseerTag,
    [Parameter(Mandatory = $true)][string]$ForDxLibTag,
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
$vcvars  = "$vsRoot\VC\Auxiliary\Build\vcvars64.bat"
$dumpbin = Get-ChildItem "$vsRoot\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe" | Select-Object -First 1 -ExpandProperty FullName

# 2. RuntimeLibrary を MD に強制する (この下で走る全 MSBuild に効く)
Copy-Item (Join-Path $PSScriptRoot "Directory.Build.targets") (Join-Path $Work "Directory.Build.targets") -Force

# 1. clone
$efk    = Join-Path $Work "Effekseer"
$efkDx  = Join-Path $Work "EffekseerForDXLib"
if (-not $SkipClone) {
    if (-not (Test-Path $efk))   { git clone --depth 1 --branch $EffekseerTag --recursive https://github.com/effekseer/Effekseer.git $efk }
    if (-not (Test-Path $efkDx)) { git clone --depth 1 --branch $ForDxLibTag  https://github.com/effekseer/EffekseerForDXLib.git $efkDx }
    # 上流は EffekseerForDXLib/DxLib_VC/ に DxLib のパッケージを置く前提
    $dxTarget = Join-Path $efkDx "DxLib_VC\プロジェクトに追加すべきファイル_VC用"
    New-Item -ItemType Directory -Force -Path $dxTarget | Out-Null
    Copy-Item (Join-Path $DxLibPackageDir "*") $dxTarget -Recurse -Force
}

# 3. build (上流の手順どおり。CMake が作る .vcxproj も Directory.Build.targets の下にあるので MD になる)
if (-not $SkipBuild) {
    Push-Location $efk
    try   { cmd /c "call `"$vcvars`" && release_dxlib.bat" ; if ($LASTEXITCODE -ne 0) { throw "release_dxlib.bat failed ($LASTEXITCODE)" } }
    finally { Pop-Location }
    Push-Location $efkDx
    try {
        python copy_package.py ; if ($LASTEXITCODE -ne 0) { throw "copy_package.py failed ($LASTEXITCODE)" }
        $sln = Get-ChildItem -Recurse -Filter "EffekseerForDXLib_vs20*.sln" | Sort-Object Name -Descending | Select-Object -First 1
        if (-not $sln) { throw "Dev\EffekseerForDXLib_vs20xx.sln が見つかりません" }
        foreach ($cfg in @("Debug", "Release")) {
            & $msbuild $sln.FullName -p:Configuration=$cfg -p:Platform=x64 -p:PreferredToolArchitecture=x64 -m
            if ($LASTEXITCODE -ne 0) { throw "MSBuild $cfg failed" }
        }
    } finally { Pop-Location }
}

# 4. 集めて確認して _MD / _MDd を付けてコピーする
$names = @("Effekseer", "EffekseerRendererDX9", "EffekseerRendererDX11", "EffekseerForDXLib")
$copied = 0
foreach ($n in $names) {
    foreach ($dbg in @($false, $true)) {
        $suffix = if ($dbg) { "_d" } else { "" }
        $lib = Get-ChildItem $Work -Recurse -Filter "${n}_vs20??_x64${suffix}.lib" -ErrorAction SilentlyContinue |
               Sort-Object LastWriteTime -Descending | Select-Object -First 1
        if (-not $lib) { Write-Warning "${n}_vs20xx_x64${suffix}.lib がビルド出力に見つかりません"; continue }
        $directives = & $dumpbin /DIRECTIVES $lib.FullName | Out-String
        $expected = if ($dbg) { "MSVCRTD" } else { "MSVCRT" }
        if ($directives -notmatch "DEFAULTLIB:`"?$expected") { throw "$($lib.FullName) は $expected を参照していません (MT のまま)。Directory.Build.targets が効いていない" }
        if ($directives -match "DEFAULTLIB:`"?LIBCMT") { throw "$($lib.FullName) に LIBCMT (静的 CRT) の参照が残っています" }
        $destName = "${n}_vs2019_x64" + $(if ($dbg) { "_MDd.lib" } else { "_MD.lib" })
        Copy-Item $lib.FullName (Join-Path $DestDir $destName) -Force
        Write-Host "  $($lib.Name) -> $destName  ($expected)"
        $copied++
    }
}
Write-Host "copied $copied / 8 libs to $DestDir"
if ($copied -ne 8) { exit 1 }
