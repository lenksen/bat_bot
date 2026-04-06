@echo off
setlocal EnableExtensions DisableDelayedExpansion
chcp 65001 >nul

set "__WG_SCRIPT_DIR=%~dp0."
set "WORK_ROOT=%TEMP%\WGZipInstaller"
set "RUN_STAMP=%RANDOM%_%RANDOM%_%RANDOM%"
set "WORK_DIR=%WORK_ROOT%\run_%RUN_STAMP%"
set "CORE_PS1=%WORK_DIR%\core.ps1"
set "SELF_PATH=%~f0"
set "PS_MARKER=#__POWERSHELL_BELOW__"
set "__WG_WORK_DIR=%WORK_DIR%"
set "__WG_ZIP_PATH=%~1"

if not exist "%WORK_ROOT%" md "%WORK_ROOT%" >nul 2>nul
if not exist "%WORK_DIR%" md "%WORK_DIR%" >nul 2>nul

if not exist "%WORK_DIR%" (
    echo [ERROR] Failed to create temp work dir: "%WORK_DIR%"
    pause
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$path = $env:SELF_PATH; " ^
  "$bytes = [System.IO.File]::ReadAllBytes($path); " ^
  "if($bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191){ $enc = New-Object System.Text.UTF8Encoding($true) } else { $enc = [System.Text.Encoding]::Default } " ^
  "$lines = [System.IO.File]::ReadAllLines($path, $enc); " ^
  "$marker = $env:PS_MARKER; " ^
  "$idx = [Array]::IndexOf($lines, $marker); " ^
  "if($idx -lt 0){ throw 'payload marker line not found' } " ^
  "if($idx -ge ($lines.Length - 1)){ throw 'payload is empty' } " ^
  "$payload = [string]::Join([Environment]::NewLine, $lines[($idx + 1)..($lines.Length - 1)]); " ^
  "$utf8bom = New-Object System.Text.UTF8Encoding($true); " ^
  "[System.IO.File]::WriteAllText($env:CORE_PS1, $payload, $utf8bom)" >nul

if errorlevel 1 (
    echo [ERROR] Failed to generate PowerShell core script.
    if exist "%WORK_DIR%" rd /s /q "%WORK_DIR%" >nul 2>nul
    pause
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -STA -File "%CORE_PS1%"
set "EXITCODE=%ERRORLEVEL%"

if exist "%WORK_DIR%" rd /s /q "%WORK_DIR%" >nul 2>nul

if not "%EXITCODE%"=="0" pause
endlocal & exit /b %EXITCODE%
goto :eof
#__POWERSHELL_BELOW__
param(
    [Parameter()][string]$ScriptRoot,
    [Parameter()][string]$WorkDir,
    [Parameter()][string]$ZipPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Console]::InputEncoding  = New-Object System.Text.UTF8Encoding($false)
[Console]::OutputEncoding = New-Object System.Text.UTF8Encoding($false)
$OutputEncoding = [Console]::OutputEncoding

$script:ConfigFile = $null
$script:LogFile = $null
$script:GlobalOverwriteMode = 'ASK'
$script:AskEscalation = $null

function Write-Log {
    param(
        [Parameter(Mandatory)][string]$Level,
        [Parameter(Mandatory)][string]$Message
    )

    if ([string]::IsNullOrWhiteSpace($script:LogFile)) {
        return
    }

    $ts = Get-Date -Format 'yyyy-MM-dd HH:mm:ss.fff'
    $line = "[{0}] [{1}] {2}" -f $ts, $Level.ToUpperInvariant(), $Message
    Add-Content -LiteralPath $script:LogFile -Value $line -Encoding UTF8
}

function Write-Step {
    param([string]$Message)
    Write-Host ""
    Write-Host ("==> {0}" -f $Message) -ForegroundColor Cyan
    Write-Log -Level 'INFO' -Message $Message
}

function Write-InfoLine {
    param([string]$Message)
    Write-Host ("    {0}" -f $Message) -ForegroundColor Gray
}

function Write-WarnLine {
    param([string]$Message)
    Write-Host ("    {0}" -f $Message) -ForegroundColor Yellow
    Write-Log -Level 'WARN' -Message $Message
}

function Resolve-FullPathStrict {
    param(
        [string]$Path,
        [string]$Name
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        throw "$Name 为空。"
    }

    $trimmed = $Path.Trim().Trim('"')
    if ([string]::IsNullOrWhiteSpace($trimmed)) {
        throw "$Name 为空。"
    }

    try {
        return [System.IO.Path]::GetFullPath($trimmed)
    } catch {
        throw ("{0} 非法: [{1}]。{2}" -f $Name, $Path, $_.Exception.Message)
    }
}

function Show-Banner {
    [Console]::Title = 'WeGame ZIP Installer'
    Write-Host '============================================================' -ForegroundColor DarkGray
    Write-Host ' WeGame ZIP 安装器 - 重构版' -ForegroundColor Green
    Write-Host '============================================================' -ForegroundColor DarkGray
}

function Cleanup-OldWorkDirs {
    param(
        [string]$Root,
        [string]$Current
    )

    if (-not (Test-Path -LiteralPath $Root -PathType Container)) { return }
    $cutoff = (Get-Date).AddDays(-1)

    try {
        foreach ($dir in Get-ChildItem -LiteralPath $Root -Directory -ErrorAction SilentlyContinue) {
            if ($dir.FullName -ieq $Current) { continue }
            if ($dir.LastWriteTime -lt $cutoff) {
                try {
                    Remove-Item -LiteralPath $dir.FullName -Recurse -Force -ErrorAction Stop
                } catch {
                }
            }
        }
    } catch {
    }
}

function Normalize-PathString {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return $null }

    $trimmed = $Path.Trim().Trim('"')
    if ([string]::IsNullOrWhiteSpace($trimmed)) { return $null }

    try {
        return [System.IO.Path]::GetFullPath($trimmed)
    } catch {
        return $null
    }
}

function Read-AllLinesAuto {
    param([string]$Path)

    $bytes = [System.IO.File]::ReadAllBytes($Path)

    if ($bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191) {
        $enc = New-Object System.Text.UTF8Encoding($true)
    }
    elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 255 -and $bytes[1] -eq 254) {
        $enc = [System.Text.Encoding]::Unicode
    }
    elseif ($bytes.Length -ge 2 -and $bytes[0] -eq 254 -and $bytes[1] -eq 255) {
        $enc = [System.Text.Encoding]::BigEndianUnicode
    }
    else {
        $enc = [System.Text.Encoding]::Default
    }

    return [System.IO.File]::ReadAllLines($Path, $enc)
}

function Get-UniqueOrdered {
    param([object[]]$Items)

    $seen = @{}
    $out = New-Object System.Collections.Generic.List[string]

    function Add-UniqueValue {
        param([object]$InputObject)

        if ($null -eq $InputObject) { return }

        $value = [string]$InputObject
        if ([string]::IsNullOrWhiteSpace($value)) { return }

        $value = $value.Trim().Trim('"')
        if ([string]::IsNullOrWhiteSpace($value)) { return }

        if ($value -notmatch '^[A-Za-z]:\\$') {
            $value = $value.TrimEnd('\')
        }

        if (-not $seen.ContainsKey($value)) {
            $seen[$value] = $true
            [void]$out.Add($value)
        }
    }

    foreach ($item in @($Items)) {
        if ($null -eq $item) { continue }

        if (($item -is [System.Collections.IEnumerable]) -and -not ($item -is [string])) {
            foreach ($sub in $item) {
                Add-UniqueValue $sub
            }
        } else {
            Add-UniqueValue $item
        }
    }

    return $out.ToArray()
}

function Load-Config {
    param([string]$Path)

    $map = @{}
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $map
    }

    foreach ($line in Read-AllLinesAuto $Path) {
        if ($line -match '^\s*[#;]') { continue }
        if ($line -match '^\s*([^=]+?)\s*=\s*(.*)\s*$') {
            $key = $matches[1].Trim()
            $val = $matches[2]
            $map[$key] = $val
        }
    }

    return $map
}

function Save-ConfigValues {
    param(
        [string]$Path,
        [hashtable]$Values
    )

    $existing = @()
    if (Test-Path -LiteralPath $Path -PathType Leaf) {
        $existing = Read-AllLinesAuto $Path
    }

    $written = @{}
    $out = New-Object System.Collections.Generic.List[string]

    foreach ($line in $existing) {
        if ($line -match '^\s*([^=;#][^=]*)=(.*)$') {
            $key = $matches[1].Trim()
            if ($Values.ContainsKey($key)) {
                [void]$out.Add(('{0}={1}' -f $key, $Values[$key]))
                $written[$key] = $true
                continue
            }
        }
        [void]$out.Add($line)
    }

    foreach ($key in $Values.Keys) {
        if (-not $written.ContainsKey($key)) {
            [void]$out.Add(('{0}={1}' -f $key, $Values[$key]))
        }
    }

    $utf8Bom = New-Object System.Text.UTF8Encoding($true)
    [System.IO.File]::WriteAllLines($Path, $out, $utf8Bom)
}

function Get-TargetRoot {
    param([string]$Path)

    $full = Normalize-PathString $Path
    if (-not $full) { return $null }

    $normalized = $full.TrimEnd('\')
    if ($normalized.EndsWith('\Game', [System.StringComparison]::OrdinalIgnoreCase)) {
        $normalized = Split-Path -LiteralPath $normalized
    }

    $gameDir = Join-Path $normalized 'Game'
    if ((Test-Path -LiteralPath $normalized -PathType Container) -and (Test-Path -LiteralPath $gameDir -PathType Container)) {
        return $normalized
    }

    return $null
}

function Ensure-WinForms {
    if (-not ('System.Windows.Forms.OpenFileDialog' -as [type])) {
        Add-Type -AssemblyName System.Windows.Forms | Out-Null
        [System.Windows.Forms.Application]::EnableVisualStyles()
    }
}

function Select-ZipFileInteractive {
    try {
        Ensure-WinForms
        $dialog = New-Object System.Windows.Forms.OpenFileDialog
        $dialog.Filter = 'ZIP 文件 (*.zip)|*.zip'
        $dialog.Title = '请选择 ZIP 文件'
        $dialog.Multiselect = $false
        $dialog.CheckFileExists = $true
        $dialog.InitialDirectory = $ScriptRoot

        if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
            return $dialog.FileName
        }
        return $null
    } catch {
        Write-WarnLine '文件选择框不可用，请手动输入 ZIP 路径。'
        return (Read-Host '请输入 ZIP 路径')
    }
}

function Select-TargetFolderInteractive {
    param([string]$InitialPath)

    try {
        Ensure-WinForms
        $dialog = New-Object System.Windows.Forms.FolderBrowserDialog
        $dialog.Description = '请选择 英雄联盟体验服 或其 Game 目录'
        if ($InitialPath -and (Test-Path -LiteralPath $InitialPath -PathType Container)) {
            $dialog.SelectedPath = $InitialPath
        }

        if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
            return $dialog.SelectedPath
        }
        return $null
    } catch {
        Write-WarnLine '文件夹选择框不可用，请手动输入目录路径。'
        return (Read-Host '请输入 目标目录路径')
    }
}

function Resolve-ZipInput {
    param([string]$InitialZip)

    $candidate = Normalize-PathString $InitialZip
    if (-not $candidate) {
        $candidate = Select-ZipFileInteractive
    }

    if (-not $candidate) {
        return $null
    }

    $candidate = Normalize-PathString $candidate
    if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
        throw "ZIP 文件不存在: $candidate"
    }

    if ([System.IO.Path]::GetExtension($candidate) -ine '.zip') {
        throw "请选择 .zip 文件: $candidate"
    }

    return $candidate
}

function Get-FixedDriveObjects {
    param([string]$SystemDrive)

    if ([string]::IsNullOrWhiteSpace($SystemDrive)) {
        $SystemDrive = 'C:'
    }

    $sys = (Normalize-PathString ($SystemDrive.TrimEnd('\') + '\')).TrimEnd('\')
    $list = New-Object System.Collections.Generic.List[object]

    foreach ($drv in [System.IO.DriveInfo]::GetDrives()) {
        if ($drv.DriveType -ne [System.IO.DriveType]::Fixed) { continue }
        if (-not $drv.IsReady) { continue }

        $rootPath = $drv.RootDirectory.FullName
        $root = $rootPath.TrimEnd('\')

        [void]$list.Add([pscustomobject]@{
            Root     = $root
            RootPath = $rootPath
            IsSystem = ($root -ieq $sys)
        })
    }

    return $list.ToArray() | Sort-Object -Property @{Expression='IsSystem';Descending=$false}, @{Expression='Root';Descending=$false}
}

function Get-CommonCandidateRoots {
    param([object[]]$DriveObjects)

    $candidates = New-Object System.Collections.Generic.List[string]

    foreach ($drive in $DriveObjects) {
        if ($drive.IsSystem) {
            foreach ($rel in @(
                '\WeGameApps\英雄联盟体验服',
                '\Program Files\WeGameApps\英雄联盟体验服',
                '\Program Files (x86)\WeGameApps\英雄联盟体验服'
            )) {
                [void]$candidates.Add($drive.Root + $rel)
            }
        } else {
            foreach ($rel in @(
                '\WeGameApps\英雄联盟体验服',
                '\Tencent\WeGameApps\英雄联盟体验服',
                '\Games\WeGameApps\英雄联盟体验服'
            )) {
                [void]$candidates.Add($drive.Root + $rel)
            }
        }
    }

    return Get-UniqueOrdered $candidates
}

function Get-ChildDirectoriesSafe {
    param([string]$Path)

    $list = New-Object System.Collections.Generic.List[string]
    try {
        foreach ($dir in [System.IO.Directory]::EnumerateDirectories($Path)) {
            [void]$list.Add($dir.TrimEnd('\'))
        }
    } catch {
    }
    return $list.ToArray()
}

function Find-WeGameAppsShallow {
    param(
        [string]$DriveRoot,
        [int]$MaxDepth = 3
    )

    $results = New-Object System.Collections.Generic.List[string]
    $queue = New-Object System.Collections.Queue
    $seen = @{}

    $queue.Enqueue([pscustomobject]@{
        Path  = $DriveRoot
        Depth = 0
    })

    while ($queue.Count -gt 0) {
        $node = $queue.Dequeue()

        if ($node.Depth -ge $MaxDepth) {
            continue
        }

        foreach ($child in Get-ChildDirectoriesSafe $node.Path) {
            if ($seen.ContainsKey($child)) { continue }
            $seen[$child] = $true

            $name = [System.IO.Path]::GetFileName($child)
            if ($name -ieq 'WeGameApps') {
                [void]$results.Add($child)
            }

            $queue.Enqueue([pscustomobject]@{
                Path  = $child
                Depth = $node.Depth + 1
            })
        }
    }

    return Get-UniqueOrdered $results
}

function Find-LolDirectUnderWeGame {
    param([string]$WeGamePath)
    return (Get-TargetRoot (Join-Path $WeGamePath '英雄联盟体验服'))
}

function Find-LolLimitedDepth {
    param(
        [string]$WeGamePath,
        [int]$MaxDepth = 2
    )

    $results = New-Object System.Collections.Generic.List[string]
    $queue = New-Object System.Collections.Queue
    $seen = @{}

    $queue.Enqueue([pscustomobject]@{
        Path  = $WeGamePath
        Depth = 0
    })

    while ($queue.Count -gt 0) {
        $node = $queue.Dequeue()

        if ($node.Depth -ge $MaxDepth) {
            continue
        }

        foreach ($child in Get-ChildDirectoriesSafe $node.Path) {
            if ($seen.ContainsKey($child)) { continue }
            $seen[$child] = $true

            $name = [System.IO.Path]::GetFileName($child)
            if ($name -ieq '英雄联盟体验服') {
                $valid = Get-TargetRoot $child
                if ($valid) {
                    [void]$results.Add($valid)
                }
            }

            $queue.Enqueue([pscustomobject]@{
                Path  = $child
                Depth = $node.Depth + 1
            })
        }
    }

    return Get-UniqueOrdered $results
}

function Find-LolDeepFallback {
    param([string]$WeGamePath)

    $results = New-Object System.Collections.Generic.List[string]
    $stack = New-Object System.Collections.Stack
    $seen = @{}

    foreach ($child in Get-ChildDirectoriesSafe $WeGamePath) {
        $stack.Push($child)
    }

    while ($stack.Count -gt 0) {
        $current = [string]$stack.Pop()
        if ($seen.ContainsKey($current)) { continue }
        $seen[$current] = $true

        $name = [System.IO.Path]::GetFileName($current)
        if ($name -ieq '英雄联盟体验服') {
            $valid = Get-TargetRoot $current
            if ($valid) {
                [void]$results.Add($valid)
            }
        }

        foreach ($child in Get-ChildDirectoriesSafe $current) {
            $stack.Push($child)
        }
    }

    return Get-UniqueOrdered $results
}

function Prompt-CandidateChoice {
    param(
        [string[]]$Candidates,
        [string]$Title
    )

    $items = @(Get-UniqueOrdered $Candidates)
    if ($items.Count -eq 0) { return $null }
    if ($items.Count -eq 1) { return $items[0] }

    Write-Host ""
    Write-Host $Title -ForegroundColor Yellow
    for ($i = 0; $i -lt $items.Count; $i++) {
        Write-Host ([string]::Format('[{0}] {1}', ($i + 1), $items[$i])) -ForegroundColor Gray
    }
    Write-Host '[0] 手动选择目录' -ForegroundColor Gray

    while ($true) {
        $raw = Read-Host '请输入编号'
        $n = 0
        if ([int]::TryParse($raw, [ref]$n)) {
            if ($n -eq 0) { return '__MANUAL__' }
            if ($n -ge 1 -and $n -le $items.Count) {
                return $items[$n - 1]
            }
        }
        Write-Host '输入无效，请重新输入。' -ForegroundColor Yellow
    }
}

function Try-Select-ManualTarget {
    param([string]$InitialPath)

    $picked = Select-TargetFolderInteractive $InitialPath
    if (-not $picked) { return $null }

    $valid = Get-TargetRoot $picked
    if ($valid) {
        return $valid
    }

    Write-WarnLine '手动选择的目录无效，必须是 英雄联盟体验服 或其 Game 目录。'
    return $null
}

function Finish-Resolve {
    param(
        [System.Diagnostics.Stopwatch]$Stopwatch,
        [string]$ResolvedPath,
        [string]$Source
    )

    if ($Stopwatch.IsRunning) {
        $Stopwatch.Stop()
    }

    if ($ResolvedPath) {
        Write-Log -Level 'INFO' -Message ("目标搜索完成，来源={0}，耗时={1:N2}s，路径={2}" -f $Source, $Stopwatch.Elapsed.TotalSeconds, $ResolvedPath)
    } else {
        Write-Log -Level 'WARN' -Message ("目标搜索未命中，耗时={0:N2}s" -f $Stopwatch.Elapsed.TotalSeconds)
    }

    return $ResolvedPath
}

function Resolve-TargetRoot {
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $config = Load-Config $script:ConfigFile
    $lastPath = $config['last_path']

    Write-Step '检查 config.ini 的 last_path'
    if (-not [string]::IsNullOrWhiteSpace($lastPath)) {
        $valid = Get-TargetRoot $lastPath
        if ($valid) {
            Write-InfoLine $valid
            return (Finish-Resolve -Stopwatch $sw -ResolvedPath $valid -Source 'config.ini')
        } else {
            Write-WarnLine ("config.ini 中的 last_path 已失效: {0}" -f $lastPath)
        }
    } else {
        Write-InfoLine 'last_path 为空，继续自动搜索。'
        Write-Log -Level 'INFO' -Message 'config.ini 未设置 last_path 或内容为空。'
    }

    $drives = @(Get-FixedDriveObjects $env:SystemDrive)
    if ($drives.Count -eq 0) {
        throw '未检测到可用的固定磁盘。'
    }

    $driveText = ($drives | ForEach-Object {
        if ($_.IsSystem) { '{0}(系统盘)' -f $_.Root } else { $_.Root }
    }) -join ', '
    Write-Log -Level 'INFO' -Message ("检测到固定磁盘: {0}" -f $driveText)

    Write-Step '检查常见固定路径'
    $commonHits = New-Object System.Collections.Generic.List[string]
    foreach ($candidate in @(Get-CommonCandidateRoots $drives)) {
        $valid = Get-TargetRoot $candidate
        if ($valid) {
            [void]$commonHits.Add($valid)
        }
    }
    $commonHits = @(Get-UniqueOrdered $commonHits)
    Write-Log -Level 'INFO' -Message ("常见路径命中数量: {0}" -f $commonHits.Count)

    if ($commonHits.Count -gt 0) {
        $choice = Prompt-CandidateChoice -Candidates $commonHits -Title '发现以下常见路径候选：'
        if ($choice -eq '__MANUAL__') {
            $manual = Try-Select-ManualTarget $lastPath
            if ($manual) {
                return (Finish-Resolve -Stopwatch $sw -ResolvedPath $manual -Source 'manual-after-common')
            }
        } elseif ($choice) {
            return (Finish-Resolve -Stopwatch $sw -ResolvedPath $choice -Source 'common-path')
        }
    }

    $nonSystemDrives = @($drives | Where-Object { -not $_.IsSystem })
    if ($nonSystemDrives.Count -eq 0) {
        Write-WarnLine '未发现非系统盘，将跳过自动浅搜索。'
        Write-Log -Level 'INFO' -Message '未发现非系统盘，跳过浅搜索。'
    } else {
        Write-Step '自动搜索非系统盘中的 WeGameApps'

        $weGameRoots = New-Object System.Collections.Generic.List[string]
        foreach ($drive in $nonSystemDrives) {
            Write-Host ('    正在浅搜索 {0}' -f $drive.RootPath) -ForegroundColor DarkCyan
            $found = @(Find-WeGameAppsShallow -DriveRoot $drive.RootPath -MaxDepth 3)
            if ($found.Count -gt 0) {
                Write-Log -Level 'INFO' -Message ("{0} 浅搜索找到 WeGameApps: {1}" -f $drive.Root, $found.Count)
                foreach ($item in $found) {
                    [void]$weGameRoots.Add($item)
                }
            }
        }

        $weGameRoots = @(Get-UniqueOrdered $weGameRoots)
        Write-Log -Level 'INFO' -Message ("浅搜索共找到 WeGameApps: {0}" -f $weGameRoots.Count)

        if ($weGameRoots.Count -gt 0) {
            Write-Step '找到 WeGameApps 后先直接检查 英雄联盟体验服\Game'
            $directHits = New-Object System.Collections.Generic.List[string]
            foreach ($wg in $weGameRoots) {
                $hit = Find-LolDirectUnderWeGame $wg
                if ($hit) {
                    [void]$directHits.Add($hit)
                }
            }
            $directHits = @(Get-UniqueOrdered $directHits)
            Write-Log -Level 'INFO' -Message ("直查命中数量: {0}" -f $directHits.Count)

            if ($directHits.Count -gt 0) {
                $choice = Prompt-CandidateChoice -Candidates $directHits -Title '发现以下直查候选：'
                if ($choice -eq '__MANUAL__') {
                    $manual = Try-Select-ManualTarget $lastPath
                    if ($manual) {
                        return (Finish-Resolve -Stopwatch $sw -ResolvedPath $manual -Source 'manual-after-direct')
                    }
                } elseif ($choice) {
                    return (Finish-Resolve -Stopwatch $sw -ResolvedPath $choice -Source 'direct-under-wegameapps')
                }
            }

            Write-Step '执行有限深度搜索（WeGameApps 下 2 层）'
            $limitedHits = New-Object System.Collections.Generic.List[string]
            foreach ($wg in $weGameRoots) {
                foreach ($hit in @(Find-LolLimitedDepth -WeGamePath $wg -MaxDepth 2)) {
                    [void]$limitedHits.Add($hit)
                }
            }
            $limitedHits = @(Get-UniqueOrdered $limitedHits)
            Write-Log -Level 'INFO' -Message ("有限深度命中数量: {0}" -f $limitedHits.Count)

            if ($limitedHits.Count -gt 0) {
                $choice = Prompt-CandidateChoice -Candidates $limitedHits -Title '发现以下有限深度候选：'
                if ($choice -eq '__MANUAL__') {
                    $manual = Try-Select-ManualTarget $lastPath
                    if ($manual) {
                        return (Finish-Resolve -Stopwatch $sw -ResolvedPath $manual -Source 'manual-after-limited')
                    }
                } elseif ($choice) {
                    return (Finish-Resolve -Stopwatch $sw -ResolvedPath $choice -Source 'limited-depth')
                }
            }

            Write-Step '执行深度兜底搜索（仅在已找到的 WeGameApps 下）'
            $deepHits = New-Object System.Collections.Generic.List[string]
            foreach ($wg in $weGameRoots) {
                Write-Host ('    深度搜索: {0}' -f $wg) -ForegroundColor DarkYellow
                foreach ($hit in @(Find-LolDeepFallback -WeGamePath $wg)) {
                    [void]$deepHits.Add($hit)
                }
            }
            $deepHits = @(Get-UniqueOrdered $deepHits)
            Write-Log -Level 'INFO' -Message ("深度兜底命中数量: {0}" -f $deepHits.Count)

            if ($deepHits.Count -gt 0) {
                $choice = Prompt-CandidateChoice -Candidates $deepHits -Title '发现以下深度搜索候选：'
                if ($choice -eq '__MANUAL__') {
                    $manual = Try-Select-ManualTarget $lastPath
                    if ($manual) {
                        return (Finish-Resolve -Stopwatch $sw -ResolvedPath $manual -Source 'manual-after-deep')
                    }
                } elseif ($choice) {
                    return (Finish-Resolve -Stopwatch $sw -ResolvedPath $choice -Source 'deep-fallback')
                }
            }
        } else {
            Write-Log -Level 'INFO' -Message '浅搜索阶段未找到任何 WeGameApps。'
        }
    }

    Write-Step '未自动找到目标目录，请手动选择'
    $manualFinal = Try-Select-ManualTarget $lastPath
    return (Finish-Resolve -Stopwatch $sw -ResolvedPath $manualFinal -Source 'manual-final')
}

function Prompt-OverwriteMode {
    Write-Host ""
    Write-Host '请选择覆盖模式：' -ForegroundColor Cyan
    Write-Host '[1] ASK       冲突时逐个询问（可切换为全部覆盖/全部跳过）' -ForegroundColor Gray
    Write-Host '[2] OVERWRITE 全部覆盖' -ForegroundColor Gray
    Write-Host '[3] SKIP      全部跳过已存在文件' -ForegroundColor Gray

    while ($true) {
        $choice = Read-Host '输入 1 / 2 / 3'
        switch ($choice) {
            '1' { return 'ASK' }
            '2' { return 'OVERWRITE' }
            '3' { return 'SKIP' }
            default { Write-Host '输入无效，请重新输入。' -ForegroundColor Yellow }
        }
    }
}

function Should-WriteFile {
    param([string]$DestinationPath)

    if (-not (Test-Path -LiteralPath $DestinationPath -PathType Leaf)) {
        return $true
    }

    switch ($script:GlobalOverwriteMode) {
        'OVERWRITE' { return $true }
        'SKIP'      { return $false }
        default {
            if ($script:AskEscalation -eq 'OVERWRITE') { return $true }
            if ($script:AskEscalation -eq 'SKIP') { return $false }

            while ($true) {
                Write-Host ('文件已存在: {0}' -f $DestinationPath) -ForegroundColor DarkYellow
                $answer = (Read-Host 'Y=覆盖 N=跳过 A=后续全部覆盖 K=后续全部跳过').Trim().ToUpperInvariant()
                switch ($answer) {
                    'Y' { return $true }
                    'N' { return $false }
                    'A' { $script:AskEscalation = 'OVERWRITE'; return $true }
                    'K' { $script:AskEscalation = 'SKIP'; return $false }
                    default { Write-Host '输入无效，请重新输入。' -ForegroundColor Yellow }
                }
            }
        }
    }
}

function Get-SafeDestinationPath {
    param(
        [string]$BaseDir,
        [string]$RelativePath
    )

    if ([string]::IsNullOrWhiteSpace($RelativePath)) { return $null }

    $clean = ($RelativePath -replace '/', '\').TrimStart('\')
    if ([string]::IsNullOrWhiteSpace($clean)) { return $null }

    if ($clean -match '(^|\\)\.\.(\\|$)') {
        throw "ZIP 条目包含非法相对路径: $RelativePath"
    }

    if ($clean -like '处于关闭状态。*' -or $clean -like 'ECHO 处于关闭状态。*') {
        throw "ZIP 条目包含异常前缀: $RelativePath"
    }

    $baseFull = [System.IO.Path]::GetFullPath($BaseDir).TrimEnd('\')
    $destFull = [System.IO.Path]::GetFullPath((Join-Path $BaseDir $clean))

    if (-not $destFull.StartsWith($baseFull + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "ZIP 条目越界: $RelativePath"
    }

    return $destFull
}

function Copy-ZipEntryToFile {
    param(
        [System.IO.Compression.ZipArchiveEntry]$Entry,
        [string]$DestinationPath
    )

    $parent = Split-Path -LiteralPath $DestinationPath
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        [System.IO.Directory]::CreateDirectory($parent) | Out-Null
    }

    if (Test-Path -LiteralPath $DestinationPath -PathType Leaf) {
        try {
            [System.IO.File]::SetAttributes($DestinationPath, [System.IO.FileAttributes]::Normal)
        } catch {
        }
    }

    $input = $Entry.Open()
    try {
        $output = [System.IO.File]::Open($DestinationPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
        try {
            $input.CopyTo($output)
        } finally {
            $output.Dispose()
        }
    } finally {
        $input.Dispose()
    }

    if ($Entry.LastWriteTime.DateTime -gt [datetime]::MinValue) {
        try {
            [System.IO.File]::SetLastWriteTime($DestinationPath, $Entry.LastWriteTime.DateTime)
        } catch {
        }
    }
}

function Extract-GameZipSubset {
    param(
        [string]$ZipPath,
        [string]$TargetRoot,
        [string]$OverwriteMode,
        [string]$ManifestFile
    )

    Write-Step '开始解压 ZIP'
    Add-Type -AssemblyName System.IO.Compression | Out-Null
    Add-Type -AssemblyName System.IO.Compression.FileSystem | Out-Null

    $gameDir = Join-Path $TargetRoot 'Game'
    if (-not (Test-Path -LiteralPath $gameDir -PathType Container)) {
        throw "目标 Game 目录不存在: $gameDir"
    }

    $script:GlobalOverwriteMode = $OverwriteMode
    $script:AskEscalation = $null

    $manifest = New-Object System.Collections.Generic.List[string]
    $stats = [ordered]@{
        Total       = 0
        New         = 0
        Overwritten = 0
        Skipped     = 0
    }

    $zip = [System.IO.Compression.ZipFile]::OpenRead($ZipPath)
    try {
        $prefix = 'league of legends/'
        $prefixLen = $prefix.Length

        $entries = @(
            $zip.Entries | Where-Object {
                $name = $_.FullName
                if ([string]::IsNullOrWhiteSpace($name)) { return $false }
                $normalized = $name -replace '\\', '/'
                return $normalized.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)
            }
        )

        if ($entries.Count -eq 0) {
            throw 'ZIP 中未找到 league of legends 根目录内容。'
        }

        Write-Log -Level 'INFO' -Message ("ZIP 子集条目数量: {0}" -f $entries.Count)

        foreach ($entry in $entries) {
            $normalized = $entry.FullName -replace '\\', '/'
            $relative = $normalized.Substring($prefixLen)

            if ([string]::IsNullOrWhiteSpace($relative)) {
                continue
            }

            if ($normalized.EndsWith('/')) {
                $dirPath = Get-SafeDestinationPath -BaseDir $gameDir -RelativePath $relative
                if ($dirPath) {
                    [System.IO.Directory]::CreateDirectory($dirPath) | Out-Null
                }
                continue
            }

            $destination = Get-SafeDestinationPath -BaseDir $gameDir -RelativePath $relative
            if (-not $destination) { continue }

            $stats.Total++
            $exists = Test-Path -LiteralPath $destination -PathType Leaf

            if (-not (Should-WriteFile -DestinationPath $destination)) {
                $stats.Skipped++
                continue
            }

            Copy-ZipEntryToFile -Entry $entry -DestinationPath $destination

            if ($exists) {
                $stats.Overwritten++
            } else {
                $stats.New++
            }

            [void]$manifest.Add($destination)
        }
    } finally {
        $zip.Dispose()
    }

    $manifest = Get-UniqueOrdered $manifest
    [System.IO.File]::WriteAllLines($ManifestFile, $manifest, (New-Object System.Text.UTF8Encoding($true)))
    Write-Log -Level 'INFO' -Message ("解压完成: 新增={0}, 覆盖={1}, 跳过={2}" -f $stats.New, $stats.Overwritten, $stats.Skipped)

    return [pscustomobject]@{
        Manifest = $manifest
        Stats    = [pscustomobject]$stats
        GameDir  = $gameDir
    }
}

function Invoke-PostProcess {
    param(
        [string[]]$Manifest,
        [string]$BotKey
    )

    Write-Step '执行后处理'

    $files = Get-UniqueOrdered (@($Manifest) | Where-Object {
        Test-Path -LiteralPath $_ -PathType Leaf
    })

    $botCount = 0
    $renameCount = 0

    if (-not [string]::IsNullOrWhiteSpace($BotKey)) {
        $userIniFiles = @($files | Where-Object { [System.IO.Path]::GetFileName($_) -ieq 'user.ini' })
        foreach ($file in $userIniFiles) {
            [System.IO.File]::WriteAllText($file, $BotKey, (New-Object System.Text.UTF8Encoding($false)))
            $botCount++
        }
        Write-Log -Level 'INFO' -Message ("写入 BOT 卡密的 user.ini 数量: {0}" -f $botCount)
    } else {
        Write-Log -Level 'INFO' -Message '未输入 BOT 卡密，跳过 user.ini 写入。'
    }

    $dllFiles = @($files | Where-Object { [System.IO.Path]::GetFileName($_) -ieq 'core_cn.dll' })
    foreach ($file in $dllFiles) {
        $dir = Split-Path -LiteralPath $file
        $dest = Join-Path $dir 'hid.dll'

        if (Test-Path -LiteralPath $dest -PathType Leaf) {
            Remove-Item -LiteralPath $dest -Force
        }

        Rename-Item -LiteralPath $file -NewName 'hid.dll' -Force
        $renameCount++
    }

    Write-Log -Level 'INFO' -Message ("重命名 core_cn.dll -> hid.dll 数量: {0}" -f $renameCount)

    return [pscustomobject]@{
        BotWritten = $botCount
        Renamed    = $renameCount
    }
}

try {
    if ([string]::IsNullOrWhiteSpace($ScriptRoot)) {
        $ScriptRoot = $env:__WG_SCRIPT_DIR
    }
    if ([string]::IsNullOrWhiteSpace($WorkDir)) {
        $WorkDir = $env:__WG_WORK_DIR
    }
    if ([string]::IsNullOrWhiteSpace($ZipPath)) {
        $ZipPath = $env:__WG_ZIP_PATH
    }

    $ScriptRoot = Resolve-FullPathStrict -Path $ScriptRoot -Name 'ScriptRoot'
    $WorkDir    = Resolve-FullPathStrict -Path $WorkDir -Name 'WorkDir'

    if (-not (Test-Path -LiteralPath $WorkDir -PathType Container)) {
        [System.IO.Directory]::CreateDirectory($WorkDir) | Out-Null
    }

    $script:ConfigFile = Join-Path $ScriptRoot 'config.ini'
    $script:LogFile    = Join-Path $ScriptRoot ('wg_installer_{0}.log' -f (Get-Date -Format 'yyyyMMdd_HHmmss'))
    $ManifestFile      = Join-Path $WorkDir 'manifest.txt'

    Cleanup-OldWorkDirs -Root (Split-Path -LiteralPath $WorkDir) -Current $WorkDir
    Show-Banner

    Write-Log -Level 'INFO' -Message '==== 运行开始 ===='
    Write-Log -Level 'INFO' -Message ("脚本目录: {0}" -f $ScriptRoot)
    Write-Log -Level 'INFO' -Message ("工作目录: {0}" -f $WorkDir)

    $zipFile = Resolve-ZipInput -InitialZip $ZipPath
    if (-not $zipFile) {
        throw '未选择 ZIP 文件。'
    }

    Write-Step '已选择 ZIP 文件'
    Write-InfoLine $zipFile
    Write-Log -Level 'INFO' -Message ("ZIP 文件: {0}" -f $zipFile)

    $targetRoot = Resolve-TargetRoot
    if (-not $targetRoot) {
        throw '未选择有效的目标目录。'
    }

    Write-Step '目标目录已确定'
    Write-InfoLine $targetRoot

    $botKey = Read-Host "`n请输入 BOT 卡密（直接回车跳过）"
    if ([string]::IsNullOrWhiteSpace($botKey)) {
        $botKey = $null
    }

    $mode = Prompt-OverwriteMode
    Write-Log -Level 'INFO' -Message ("覆盖模式: {0}" -f $mode)

    $result = Extract-GameZipSubset -ZipPath $zipFile -TargetRoot $targetRoot -OverwriteMode $mode -ManifestFile $ManifestFile
    $post = Invoke-PostProcess -Manifest $result.Manifest -BotKey $botKey

    Save-ConfigValues -Path $script:ConfigFile -Values @{ 'last_path' = $targetRoot }
    Write-Log -Level 'INFO' -Message ("已保存 last_path: {0}" -f $targetRoot)

    Write-Step '处理完成'
    Write-Host ('    目标 Game 目录: {0}' -f $result.GameDir) -ForegroundColor Green
    Write-Host ('    新增文件: {0}' -f $result.Stats.New) -ForegroundColor Green
    Write-Host ('    覆盖文件: {0}' -f $result.Stats.Overwritten) -ForegroundColor Green
    Write-Host ('    跳过文件: {0}' -f $result.Stats.Skipped) -ForegroundColor Green
    if ($botKey) {
        Write-Host ('    写入 BOT 卡密: {0}' -f $post.BotWritten) -ForegroundColor Green
    }
    Write-Host ('    DLL 重命名: {0}' -f $post.Renamed) -ForegroundColor Green
    Write-Host ('    日志文件: {0}' -f $script:LogFile) -ForegroundColor Green

    Write-Log -Level 'INFO' -Message '==== 运行成功结束 ===='
    exit 0
}
catch {
    $msg = $_.Exception.Message
    Write-Host ""
    Write-Host ('[ERROR] {0}' -f $msg) -ForegroundColor Red
    try {
        Write-Log -Level 'ERROR' -Message $msg
        if ($_.ScriptStackTrace) {
            Write-Log -Level 'ERROR' -Message $_.ScriptStackTrace
        }
    } catch {
    }
    if (-not [string]::IsNullOrWhiteSpace($script:LogFile)) {
        Write-Host ('日志文件: {0}' -f $script:LogFile) -ForegroundColor Red
    }
    exit 1
}