Add-Type -AssemblyName System.Drawing

$ErrorActionPreference = 'Stop'
$outputDir = Join-Path $PSScriptRoot '..\assets\ui'
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

$frames = @(
    @{ Name = 'panel_default.png'; Width = 420; Height = 240; Seed = 1 },
    @{ Name = 'panel_inventory.png'; Width = 388; Height = 192; Seed = 2 },
    @{ Name = 'panel_log.png'; Width = 600; Height = 192; Seed = 3 },
    @{ Name = 'panel_map.png'; Width = 360; Height = 132; Seed = 4 },
    @{ Name = 'panel_craft.png'; Width = 360; Height = 330; Seed = 5 },
    @{ Name = 'panel_status.png'; Width = 360; Height = 220; Seed = 6 },
    @{ Name = 'panel_network.png'; Width = 650; Height = 382; Seed = 7 },
    @{ Name = 'panel_tooltip.png'; Width = 360; Height = 160; Seed = 8 },
    @{ Name = 'panel_game.png'; Width = 1280; Height = 720; Seed = 9; TransparentInterior = $true }
)

function New-UiColor($r, $g, $b, $a = 255) {
    return [System.Drawing.Color]::FromArgb($a, $r, $g, $b)
}

function Fill-UiRect($graphics, $brush, $x, $y, $width, $height) {
    if ($width -gt 0 -and $height -gt 0) {
        $graphics.FillRectangle($brush, [int]$x, [int]$y, [int]$width, [int]$height)
    }
}

function Draw-UiFrame($path, $width, $height, $seed, $transparentInterior = $false) {
    $bitmap = New-Object System.Drawing.Bitmap $width, $height, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
    $graphics.Clear((New-UiColor 0 0 0 0))

    $black = New-Object System.Drawing.SolidBrush (New-UiColor 0 0 0 248)
    $shadow = New-Object System.Drawing.SolidBrush (New-UiColor 0 0 0 185)
    $blueDark = New-Object System.Drawing.SolidBrush (New-UiColor 24 34 42)
    $blueMid = New-Object System.Drawing.SolidBrush (New-UiColor 58 76 84)
    $cream = New-Object System.Drawing.SolidBrush (New-UiColor 230 224 196)
    $cut = New-Object System.Drawing.SolidBrush (New-UiColor 0 0 0 255)

    if (-not $transparentInterior) {
        Fill-UiRect $graphics $black 8 8 ($width - 16) ($height - 16)
    }

    Fill-UiRect $graphics $shadow 5 5 ($width - 10) 8
    Fill-UiRect $graphics $shadow 5 ($height - 13) ($width - 10) 8
    Fill-UiRect $graphics $shadow 5 5 8 ($height - 10)
    Fill-UiRect $graphics $shadow ($width - 13) 5 8 ($height - 10)

    Fill-UiRect $graphics $blueDark 3 3 ($width - 6) 6
    Fill-UiRect $graphics $blueDark 3 ($height - 9) ($width - 6) 6
    Fill-UiRect $graphics $blueDark 3 3 6 ($height - 6)
    Fill-UiRect $graphics $blueDark ($width - 9) 3 6 ($height - 6)

    Fill-UiRect $graphics $blueMid 5 5 ($width - 10) 3
    Fill-UiRect $graphics $blueMid 5 ($height - 8) ($width - 10) 3
    Fill-UiRect $graphics $blueMid 5 5 3 ($height - 10)
    Fill-UiRect $graphics $blueMid ($width - 8) 5 3 ($height - 10)

    Fill-UiRect $graphics $cream 1 1 ($width - 2) 3
    Fill-UiRect $graphics $cream 1 ($height - 4) ($width - 2) 3
    Fill-UiRect $graphics $cream 1 1 3 ($height - 2)
    Fill-UiRect $graphics $cream ($width - 4) 1 3 ($height - 2)

    $random = New-Object System.Random $seed
    for ($x = 24 + $random.Next(0, 18); $x -lt $width - 32; $x += $random.Next(54, 94)) {
        $chipWidth = $random.Next(5, 13)
        Fill-UiRect $graphics $blueDark $x 0 ($chipWidth + 3) 5
        Fill-UiRect $graphics $cream ($x + 1) 1 $chipWidth 2
        if ($random.NextDouble() -lt 0.35) {
            Fill-UiRect $graphics $cut ($x + 6) 2 ($random.Next(3, 7)) 2
        }
        if ($random.NextDouble() -lt 0.45) {
            Fill-UiRect $graphics $blueDark ($x + 10) ($height - 5) ($chipWidth + 2) 5
            Fill-UiRect $graphics $cream ($x + 11) ($height - 3) $chipWidth 2
        }
    }

    for ($y = 24 + $random.Next(0, 18); $y -lt $height - 32; $y += $random.Next(54, 94)) {
        $chipHeight = $random.Next(5, 13)
        Fill-UiRect $graphics $blueDark 0 $y 5 ($chipHeight + 3)
        Fill-UiRect $graphics $cream 1 ($y + 1) 2 $chipHeight
        if ($random.NextDouble() -lt 0.40) {
            Fill-UiRect $graphics $blueDark ($width - 5) ($y + 8) 5 ($chipHeight + 2)
            Fill-UiRect $graphics $cream ($width - 3) ($y + 9) 2 $chipHeight
        }
    }

    Fill-UiRect $graphics $blueDark 0 0 16 16
    Fill-UiRect $graphics $blueDark ($width - 16) 0 16 16
    Fill-UiRect $graphics $blueDark 0 ($height - 16) 16 16
    Fill-UiRect $graphics $blueDark ($width - 16) ($height - 16) 16 16
    Fill-UiRect $graphics $cream 1 1 11 3
    Fill-UiRect $graphics $cream 1 1 3 11
    Fill-UiRect $graphics $cream ($width - 12) 1 11 3
    Fill-UiRect $graphics $cream ($width - 4) 1 3 11
    Fill-UiRect $graphics $cream 1 ($height - 4) 11 3
    Fill-UiRect $graphics $cream 1 ($height - 12) 3 11
    Fill-UiRect $graphics $cream ($width - 12) ($height - 4) 11 3
    Fill-UiRect $graphics $cream ($width - 4) ($height - 12) 3 11
    Fill-UiRect $graphics $cut 6 6 8 8
    Fill-UiRect $graphics $cut ($width - 14) 6 8 8
    Fill-UiRect $graphics $cut 6 ($height - 14) 8 8
    Fill-UiRect $graphics $cut ($width - 14) ($height - 14) 8 8

    $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)

    $graphics.Dispose()
    $bitmap.Dispose()
    $black.Dispose()
    $shadow.Dispose()
    $blueDark.Dispose()
    $blueMid.Dispose()
    $cream.Dispose()
    $cut.Dispose()
}

foreach ($frame in $frames) {
    Draw-UiFrame (Join-Path $outputDir $frame.Name) $frame.Width $frame.Height $frame.Seed ($frame.TransparentInterior -eq $true)
}
