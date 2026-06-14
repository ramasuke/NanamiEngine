$json_input = $input | ConvertFrom-Json
$file_path = $json_input.tool_input.file_path

if ($null -eq $file_path) { exit 0 }
if ($file_path -notmatch '\.(h|cpp)$') { exit 0 }
if (-not (Test-Path $file_path)) { exit 0 }

try {
    $content = [System.IO.File]::ReadAllText($file_path, [System.Text.Encoding]::UTF8)
    $shiftjis = [System.Text.Encoding]::GetEncoding(932)
    [System.IO.File]::WriteAllText($file_path, $content, $shiftjis)
} catch {
    exit 0
}
exit 0
