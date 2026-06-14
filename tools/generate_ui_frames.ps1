Add-Type -AssemblyName System.Drawing

$ErrorActionPreference = 'Stop'
$outputDir = Join-Path $PSScriptRoot '..\assets\ui'
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

$frames = @(
    @{ Name = 'panel_default.png'; Width = 420; Height = 240; Seed = 1 },
    @{ Name = 'panel_inventory.png'; Width = 520; Height = 310; Seed = 2; Inventory = $true },
    @{ Name = 'panel_log.png'; Width = 690; Height = 310; Seed = 3 },
    @{ Name = 'panel_map.png'; Width = 440; Height = 210; Seed = 4 },
    @{ Name = 'panel_craft.png'; Width = 440; Height = 390; Seed = 5 },
    @{ Name = 'panel_status.png'; Width = 440; Height = 350; Seed = 6 },
    @{ Name = 'panel_network.png'; Width = 650; Height = 382; Seed = 7 },
    @{ Name = 'panel_tooltip.png'; Width = 360; Height = 160; Seed = 8 },
    @{ Name = 'panel_game.png'; Width = 1280; Height = 720; Seed = 9; TransparentInterior = $true }
)

function New-UiColor($r, $g, $b, $a = 255) {
    return [System.Drawing.Color]::FromArgb($a, $r, $g, $b)
}

function New-UiBrush($r, $g, $b, $a = 255) {
    return New-Object System.Drawing.SolidBrush (New-UiColor $r $g $b $a)
}

function Fill-UiRect($graphics, $brush, $x, $y, $width, $height) {
    if ($width -gt 0 -and $height -gt 0) {
        $graphics.FillRectangle($brush, [int]$x, [int]$y, [int]$width, [int]$height)
    }
}

function Draw-UiBolt($graphics, $darkBrush, $lightBrush, $x, $y) {
    Fill-UiRect $graphics $darkBrush $x $y 12 12
    Fill-UiRect $graphics $lightBrush ($x + 2) ($y + 2) 8 8
    Fill-UiRect $graphics $darkBrush ($x + 5) ($y + 5) 3 3
}

function Draw-PanelCorners($graphics, $black, $edgeLight, $lavender, $lavenderDark, $width, $height) {
    Draw-UiBolt $graphics $lavenderDark $lavender 3 3
    Draw-UiBolt $graphics $lavenderDark $lavender ($width - 15) 3
    Draw-UiBolt $graphics $lavenderDark $lavender 3 ($height - 15)
    Draw-UiBolt $graphics $lavenderDark $lavender ($width - 15) ($height - 15)

    Fill-UiRect $graphics $black 19 20 5 5
    Fill-UiRect $graphics $edgeLight 20 20 3 3
    Fill-UiRect $graphics $black ($width - 24) 20 5 5
    Fill-UiRect $graphics $edgeLight ($width - 23) 20 3 3
    Fill-UiRect $graphics $black 19 ($height - 25) 5 5
    Fill-UiRect $graphics $edgeLight 20 ($height - 24) 3 3
    Fill-UiRect $graphics $black ($width - 24) ($height - 25) 5 5
    Fill-UiRect $graphics $edgeLight ($width - 23) ($height - 24) 3 3
}

function Draw-TitleBar($graphics, $outer, $bar, $barDark, $edgeLight, $edgeDark, $lavender, $lavenderDark, $x, $y, $width) {
    Fill-UiRect $graphics $outer $x $y $width 42
    Fill-UiRect $graphics $bar ($x + 8) ($y + 5) ($width - 16) 30
    Fill-UiRect $graphics $edgeLight ($x + 10) ($y + 6) ($width - 20) 3
    Fill-UiRect $graphics $barDark ($x + 10) ($y + 32) ($width - 20) 4
    Fill-UiRect $graphics $edgeDark ($x + 4) ($y + 38) ($width - 8) 3
    Fill-UiRect $graphics $lavender ($x + 3) ($y + 2) 7 7
    Fill-UiRect $graphics $lavenderDark ($x + 5) ($y + 4) 3 3
    Fill-UiRect $graphics $lavender ($x + $width - 10) ($y + 2) 7 7
    Fill-UiRect $graphics $lavenderDark ($x + $width - 8) ($y + 4) 3 3
}

function Draw-InnerPanel($graphics, $bodyDark, $innerDark, $edgeLight, $edgeDark, $x, $y, $width, $height) {
    Fill-UiRect $graphics $bodyDark $x $y $width $height
    Fill-UiRect $graphics $innerDark ($x + 9) ($y + 9) ($width - 18) ($height - 18)
    Fill-UiRect $graphics $edgeLight ($x + 6) ($y + 5) ($width - 12) 2
    Fill-UiRect $graphics $edgeLight ($x + 5) ($y + 6) 2 ($height - 12)
    Fill-UiRect $graphics $edgeDark ($x + 6) ($y + $height - 7) ($width - 12) 2
    Fill-UiRect $graphics $edgeDark ($x + $width - 7) ($y + 6) 2 ($height - 12)
}

function Draw-InventorySidebar($graphics, $outer, $bar, $barDark, $innerDark, $edgeLight, $edgeDark, $lavender, $lavenderDark, $cyan, $height) {
    $sidebarWidth = 90
    Fill-UiRect $graphics $outer 0 0 $sidebarWidth $height
    Fill-UiRect $graphics $bar 8 14 72 ($height - 28)
    Fill-UiRect $graphics $barDark 16 22 48 ($height - 44)
    Fill-UiRect $graphics $edgeLight 10 16 2 ($height - 32)
    Fill-UiRect $graphics $edgeDark 78 16 2 ($height - 32)

    Fill-UiRect $graphics $edgeDark 38 30 22 22
    Fill-UiRect $graphics $bar 42 34 14 14
    Fill-UiRect $graphics $edgeLight 43 34 12 2
    Fill-UiRect $graphics $edgeDark 43 47 12 2

    Fill-UiRect $graphics $cyan 28 72 34 34
    Fill-UiRect $graphics $innerDark 33 77 24 24
    Fill-UiRect $graphics $cyan 63 81 9 16
    Fill-UiRect $graphics $cyan 72 85 8 8
    Fill-UiRect $graphics $edgeLight 79 87 4 4

    for ($i = 0; $i -lt 2; ++$i) {
        $slotY = 122 + $i * 52
        Fill-UiRect $graphics $lavender 31 $slotY 35 35
        Fill-UiRect $graphics $lavenderDark 35 ($slotY + 4) 27 27
        Fill-UiRect $graphics $innerDark 39 ($slotY + 8) 19 19
    }

    $arrowY = $height - 70
    Fill-UiRect $graphics $edgeLight 41 $arrowY 20 4
    Fill-UiRect $graphics $edgeLight 45 ($arrowY + 4) 12 4
    Fill-UiRect $graphics $edgeLight 49 ($arrowY + 8) 4 4
    Fill-UiRect $graphics $barDark 41 ($arrowY + 12) 20 4

    Fill-UiRect $graphics $edgeLight 0 ($height - 38) $sidebarWidth 2
}

function Draw-FrameChrome($graphics, $shadow, $black, $outer, $bar, $edgeLight, $edgeDark, $lavender, $lavenderDark, $width, $height, $transparentInterior) {
    if ($transparentInterior) {
        Fill-UiRect $graphics $shadow 8 8 ($width - 8) 22
        Fill-UiRect $graphics $shadow 8 ($height - 30) ($width - 8) 22
        Fill-UiRect $graphics $shadow 8 8 22 ($height - 8)
        Fill-UiRect $graphics $shadow ($width - 30) 8 22 ($height - 8)
    }
    else {
        Fill-UiRect $graphics $shadow 8 8 ($width - 8) ($height - 8)
    }

    if ($transparentInterior) {
        Fill-UiRect $graphics $outer 0 0 $width 28
        Fill-UiRect $graphics $outer 0 ($height - 28) $width 28
        Fill-UiRect $graphics $outer 0 0 28 $height
        Fill-UiRect $graphics $outer ($width - 28) 0 28 $height
        Fill-UiRect $graphics $black 18 18 ($width - 36) 8
        Fill-UiRect $graphics $black 18 ($height - 26) ($width - 36) 8
        Fill-UiRect $graphics $black 18 18 8 ($height - 36)
        Fill-UiRect $graphics $black ($width - 26) 18 8 ($height - 36)
    }
    else {
        Fill-UiRect $graphics $outer 0 12 $width ($height - 24)
        Fill-UiRect $graphics $outer 12 0 ($width - 24) $height
        Fill-UiRect $graphics $black 18 18 ($width - 36) ($height - 36)
    }

    Fill-UiRect $graphics $bar 8 8 ($width - 16) 16
    Fill-UiRect $graphics $bar 8 ($height - 24) ($width - 16) 16
    Fill-UiRect $graphics $bar 8 8 16 ($height - 16)
    Fill-UiRect $graphics $bar ($width - 24) 8 16 ($height - 16)

    Fill-UiRect $graphics $edgeLight 10 8 ($width - 20) 3
    Fill-UiRect $graphics $edgeLight 8 10 3 ($height - 20)
    Fill-UiRect $graphics $edgeDark 10 ($height - 12) ($width - 20) 3
    Fill-UiRect $graphics $edgeDark ($width - 12) 10 3 ($height - 20)
    Draw-PanelCorners $graphics $black $edgeLight $lavender $lavenderDark $width $height
}

function Draw-UiFrame($path, $width, $height, $seed, $transparentInterior = $false, $inventory = $false) {
    $bitmap = New-Object System.Drawing.Bitmap $width, $height, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
    $graphics.Clear((New-UiColor 0 0 0 0))

    $shadow = New-UiBrush 2 5 7 165
    $black = New-UiBrush 5 12 13 245
    $outer = New-UiBrush 47 69 72 255
    $bar = New-UiBrush 65 89 92 255
    $barDark = New-UiBrush 35 55 59 255
    $bodyDark = New-UiBrush 23 43 45 242
    $innerDark = New-UiBrush 8 20 21 230
    $edgeLight = New-UiBrush 143 167 171 255
    $edgeDark = New-UiBrush 24 39 43 255
    $lavender = New-UiBrush 168 158 199 255
    $lavenderDark = New-UiBrush 92 84 118 255
    $cyan = New-UiBrush 190 237 229 255

    Draw-FrameChrome $graphics $shadow $black $outer $bar $edgeLight $edgeDark $lavender $lavenderDark $width $height $transparentInterior

    if (-not $transparentInterior) {
        if ($inventory) {
            Draw-InventorySidebar $graphics $outer $bar $barDark $innerDark $edgeLight $edgeDark $lavender $lavenderDark $cyan $height
            Draw-TitleBar $graphics $outer $bar $barDark $edgeLight $edgeDark $lavender $lavenderDark 92 0 ($width - 112)
            Draw-InnerPanel $graphics $bodyDark $innerDark $edgeLight $edgeDark 104 54 ($width - 124) ($height - 76)
        }
        else {
            Draw-TitleBar $graphics $outer $bar $barDark $edgeLight $edgeDark $lavender $lavenderDark 24 0 ($width - 48)
            Draw-InnerPanel $graphics $bodyDark $innerDark $edgeLight $edgeDark 24 54 ($width - 48) ($height - 78)
        }
    }

    $random = New-Object System.Random $seed
    for ($x = 58 + $random.Next(0, 20); $x -lt $width - 80; $x += $random.Next(96, 154)) {
        Fill-UiRect $graphics $edgeDark $x 23 28 3
        Fill-UiRect $graphics $edgeLight ($x + 2) 24 20 1
        Fill-UiRect $graphics $edgeDark ($x + 18) ($height - 25) 28 3
        Fill-UiRect $graphics $edgeLight ($x + 20) ($height - 24) 20 1
    }

    for ($y = 72 + $random.Next(0, 24); $y -lt $height - 90; $y += $random.Next(96, 156)) {
        Fill-UiRect $graphics $edgeDark 23 $y 3 28
        Fill-UiRect $graphics $edgeLight 24 ($y + 2) 1 20
        Fill-UiRect $graphics $edgeDark ($width - 26) ($y + 18) 3 28
        Fill-UiRect $graphics $edgeLight ($width - 25) ($y + 20) 1 20
    }

    $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)

    $graphics.Dispose()
    $bitmap.Dispose()
    $shadow.Dispose()
    $black.Dispose()
    $outer.Dispose()
    $bar.Dispose()
    $barDark.Dispose()
    $bodyDark.Dispose()
    $innerDark.Dispose()
    $edgeLight.Dispose()
    $edgeDark.Dispose()
    $lavender.Dispose()
    $lavenderDark.Dispose()
    $cyan.Dispose()
}

foreach ($frame in $frames) {
    Draw-UiFrame (Join-Path $outputDir $frame.Name) $frame.Width $frame.Height $frame.Seed ($frame.TransparentInterior -eq $true) ($frame.Inventory -eq $true)
}
