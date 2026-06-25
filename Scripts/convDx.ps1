<#
.SYNOPSIS
    UTF-8 から Shift-JIS へ .h / .cpp ファイルを変換する

.DESCRIPTION
    フォルダを指定すると配下の .h/.cpp を再帰的に変換。
    ファイルを指定すると単一ファイルを変換。

.EXAMPLE
    convDx .\Engine\Module\Physics
    convDx .\Engine\Module\Physics\Engine_Physics_Physics.h
    convDx .\Engine\Module\Physics -WhatIf
#>
param(
    [Parameter(Mandatory = $true, Position = 0, HelpMessage = "変換対象のファイルまたはフォルダパス")]
    [string]$Path,

    [Parameter()]
    [switch]$WhatIf
)

$sjis = [System.Text.Encoding]::GetEncoding(932)

function ConvertTo-ShiftJIS {
    param([string]$FilePath)

    try {
        $bytes  = [System.IO.File]::ReadAllBytes($FilePath)
        $hasBom = ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)
        $text   = if ($hasBom) {
            [System.Text.Encoding]::UTF8.GetString($bytes, 3, $bytes.Length - 3)
        } else {
            [System.Text.Encoding]::UTF8.GetString($bytes)
        }

        if ($WhatIf) {
            Write-Host "[WhatIf] $FilePath"
            return
        }

        [System.IO.File]::WriteAllText($FilePath, $text, $sjis)
        Write-Host "Converted : $FilePath"
    } catch {
        Write-Warning "Failed    : $FilePath`n  $_"
    }
}

$resolved = Resolve-Path -LiteralPath $Path -ErrorAction SilentlyContinue
if (-not $resolved) {
    Write-Error "パスが見つかりません: $Path"
    exit 1
}
$target = $resolved.Path

if (Test-Path $target -PathType Leaf) {
    ConvertTo-ShiftJIS $target
} elseif (Test-Path $target -PathType Container) {
    $files = Get-ChildItem -LiteralPath $target -Recurse -Include "*.h","*.cpp"
    if ($files.Count -eq 0) {
        Write-Host "対象ファイルが見つかりませんでした: $target"
        exit 0
    }
    foreach ($f in $files) {
        ConvertTo-ShiftJIS $f.FullName
    }
    $label = if ($WhatIf) { "[WhatIf] 対象" } else { "変換完了" }
    Write-Host "${label}: $($files.Count) ファイル"
} else {
    Write-Error "ファイルでもフォルダでもないパスです: $target"
    exit 1
}