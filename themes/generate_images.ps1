Add-Type -AssemblyName System.Drawing

$flatDir = "D:\horus_ui\themes\flat"
$pixelDir = "D:\horus_ui\themes\pixel"

function New-FlatImage {
    param($name, $width, $height, [ScriptBlock]$draw)
    $bmp = New-Object System.Drawing.Bitmap($width, $height)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
    $g.Clear([System.Drawing.Color]::Transparent)
    & $draw $g $width $height
    $g.Dispose()
    $bmp.Save("$flatDir\$name.png", [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}

function New-PixelImage {
    param($name, $width, $height, [ScriptBlock]$setPixels)
    $bmp = New-Object System.Drawing.Bitmap($width, $height)
    for ($y = 0; $y -lt $height; $y++) {
        for ($x = 0; $x -lt $width; $x++) {
            $bmp.SetPixel($x, $y, [System.Drawing.Color]::Transparent)
        }
    }
    & $setPixels $bmp $width $height
    $bmp.Save("$pixelDir\$name.png", [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}

function Set-Px {
    param($bmp, $x, $y, $r, $g, $b, $a)
    if ($x -ge 0 -and $x -lt $bmp.Width -and $y -ge 0 -and $y -lt $bmp.Height) {
        $bmp.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($a, $r, $g, $b))
    }
}

# ========================
# 1x1 pixel (placeholder for solid color)
# ========================
$bmp = New-Object System.Drawing.Bitmap(1, 1)
$bmp.SetPixel(0, 0, [System.Drawing.Color]::White)
$bmp.Save("$flatDir\pixel.png", [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Save("$pixelDir\pixel.png", [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()

# ========================
# FLAT THEME IMAGES
# ========================
Write-Host "Generating flat theme images..."

# dropdown_arrow - down triangle 9x6
New-FlatImage "dropdown_arrow" 9 6 {
    param($g, $w, $h)
    $brush = [System.Drawing.Brushes]::White
    $points = @(
        [System.Drawing.Point]::new(0, 0),
        [System.Drawing.Point]::new($w-1, 0),
        [System.Drawing.Point]::new([Math]::Floor($w/2), $h-1)
    )
    $g.FillPolygon($brush, $points)
}

# expandable_expanded_arrow / treenode_expanded_arrow - down triangle
New-FlatImage "expandable_expanded_arrow" 9 6 {
    param($g, $w, $h)
    $brush = [System.Drawing.Brushes]::White
    $points = @(
        [System.Drawing.Point]::new(0, 0),
        [System.Drawing.Point]::new($w-1, 0),
        [System.Drawing.Point]::new([Math]::Floor($w/2), $h-1)
    )
    $g.FillPolygon($brush, $points)
}
Copy-Item "$flatDir\expandable_expanded_arrow.png" "$flatDir\treenode_expanded_arrow.png"

# expandable_collapsed_arrow / treenode_collapsed_arrow - right triangle
New-FlatImage "expandable_collapsed_arrow" 6 9 {
    param($g, $w, $h)
    $brush = [System.Drawing.Brushes]::White
    $points = @(
        [System.Drawing.Point]::new(0, 0),
        [System.Drawing.Point]::new(0, $h-1),
        [System.Drawing.Point]::new($w-1, [Math]::Floor($h/2))
    )
    $g.FillPolygon($brush, $points)
}
Copy-Item "$flatDir\expandable_collapsed_arrow.png" "$flatDir\treenode_collapsed_arrow.png"

# check_mark - check mark shape
New-FlatImage "check_mark" 8 8 {
    param($g, $w, $h)
    $pen = [System.Drawing.Pens]::White
    $g.DrawLine($pen, 1, 5, 3, 7)
    $g.DrawLine($pen, 3, 7, 7, 1)
}

# radio_body - circle outline
New-FlatImage "radio_body" 16 16 {
    param($g, $w, $h)
    $g.DrawEllipse([System.Drawing.Pens]::White, 1, 1, 13, 13)
}

# radio_mark - filled circle
New-FlatImage "radio_mark" 8 8 {
    param($g, $w, $h)
    $g.FillEllipse([System.Drawing.Brushes]::White, 0, 0, 7, 7)
}

# slider - horizontal track
New-FlatImage "slider" 3 16 {
    param($g, $w, $h)
    $g.FillRectangle([System.Drawing.Brushes]::White, 0, 6, $w, 4)
}

# slider_filled - filled track
New-FlatImage "slider_filled" 3 16 {
    param($g, $w, $h)
    $g.FillRectangle([System.Drawing.Brushes]::White, 0, 6, $w, 4)
}

# slider_knob - filled circle
New-FlatImage "slider_knob" 8 8 {
    param($g, $w, $h)
    $g.FillEllipse([System.Drawing.Brushes]::White, 1, 1, 6, 6)
}

# comboslider_left_arrow
New-FlatImage "comboslider_left_arrow" 5 9 {
    param($g, $w, $h)
    $brush = [System.Drawing.Brushes]::White
    $points = @(
        [System.Drawing.Point]::new($w-1, 0),
        [System.Drawing.Point]::new($w-1, $h-1),
        [System.Drawing.Point]::new(0, [Math]::Floor($h/2))
    )
    $g.FillPolygon($brush, $points)
}

# comboslider_right_arrow
New-FlatImage "comboslider_right_arrow" 5 9 {
    param($g, $w, $h)
    $brush = [System.Drawing.Brushes]::White
    $points = @(
        [System.Drawing.Point]::new(0, 0),
        [System.Drawing.Point]::new(0, $h-1),
        [System.Drawing.Point]::new($w-1, [Math]::Floor($h/2))
    )
    $g.FillPolygon($brush, $points)
}

# rotaryslider_body - circle
New-FlatImage "rotaryslider_body" 32 32 {
    param($g, $w, $h)
    $g.DrawEllipse([System.Drawing.Pens]::White, 1, 1, 29, 29)
}

# clear - X shape
New-FlatImage "clear" 8 8 {
    param($g, $w, $h)
    $pen = [System.Drawing.Pens]::White
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
    $g.DrawLine($pen, 0, 0, 7, 7)
    $g.DrawLine($pen, 7, 0, 0, 7)
}

# multiline_text_input_wordwrap
New-FlatImage "multiline_text_input_wordwrap" 12 8 {
    param($g, $w, $h)
    $pen = [System.Drawing.Pens]::White
    $g.DrawLine($pen, 0, 0, 7, 0)
    $g.DrawLine($pen, 0, 2, 7, 2)
    $g.DrawLine($pen, 0, 4, 7, 4)
    $g.DrawLine($pen, 9, 0, 11, 2)
    $g.DrawLine($pen, 11, 2, 9, 4)
}

# submenu_item_arrow - right triangle
New-FlatImage "submenu_item_arrow" 5 9 {
    param($g, $w, $h)
    $brush = [System.Drawing.Brushes]::White
    $points = @(
        [System.Drawing.Point]::new(0, 0),
        [System.Drawing.Point]::new(0, $h-1),
        [System.Drawing.Point]::new($w-1, [Math]::Floor($h/2))
    )
    $g.FillPolygon($brush, $points)
}

# menu_item_checkmark
Copy-Item "$flatDir\check_mark.png" "$flatDir\menu_item_checkmark.png"

# sign_error - X in circle
New-FlatImage "sign_error" 16 16 {
    param($g, $w, $h)
    $g.DrawEllipse([System.Drawing.Pens]::White, 1, 1, 13, 13)
    $g.DrawLine([System.Drawing.Pens]::White, 5, 5, 11, 11)
    $g.DrawLine([System.Drawing.Pens]::White, 11, 5, 5, 11)
}

# sign_info - i in circle
New-FlatImage "sign_info" 16 16 {
    param($g, $w, $h)
    $g.DrawEllipse([System.Drawing.Pens]::White, 1, 1, 13, 13)
    $g.DrawRectangle([System.Drawing.Pens]::White, 7, 5, 2, 2)
    $g.DrawRectangle([System.Drawing.Pens]::White, 7, 9, 2, 5)
}

# sign_question - ? in circle
New-FlatImage "sign_question" 16 16 {
    param($g, $w, $h)
    $g.DrawEllipse([System.Drawing.Pens]::White, 1, 1, 13, 13)
    $g.DrawArc([System.Drawing.Pens]::White, 5, 4, 6, 5, 180, 180)
    $g.DrawLine([System.Drawing.Pens]::White, 8, 9, 8, 10)
    $g.DrawLine([System.Drawing.Pens]::White, 8, 12, 8, 12)
}

# sign_warning - ! in triangle
New-FlatImage "sign_warning" 16 16 {
    param($g, $w, $h)
    $brush = [System.Drawing.Brushes]::White
    $points = @(
        [System.Drawing.Point]::new(8, 1),
        [System.Drawing.Point]::new(1, 14),
        [System.Drawing.Point]::new(15, 14)
    )
    $g.FillPolygon($brush, $points)
}

# xaxis
New-FlatImage "xaxis" 16 16 {
    param($g, $w, $h)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
    $g.DrawLine([System.Drawing.Pens]::White, 0, 8, 14, 2)
    $g.DrawLine([System.Drawing.Pens]::White, 14, 2, 10, 1)
    $g.DrawLine([System.Drawing.Pens]::White, 14, 2, 13, 6)
    $g.DrawLine([System.Drawing.Pens]::White, 8, 0, 2, 14)
    $g.DrawLine([System.Drawing.Pens]::White, 2, 14, 6, 13)
    $g.DrawLine([System.Drawing.Pens]::White, 2, 14, 1, 10)
}

# yaxis
New-FlatImage "yaxis" 16 16 {
    param($g, $w, $h)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
    $g.DrawLine([System.Drawing.Pens]::White, 8, 14, 8, 1)
    $g.DrawLine([System.Drawing.Pens]::White, 8, 1, 12, 3)
    $g.DrawLine([System.Drawing.Pens]::White, 8, 1, 4, 3)
    $g.DrawLine([System.Drawing.Pens]::White, 1, 8, 12, 8)
    $g.DrawLine([System.Drawing.Pens]::White, 1, 8, 4, 9)
    $g.DrawLine([System.Drawing.Pens]::White, 1, 8, 3, 5)
}

# zaxis
New-FlatImage "zaxis" 16 16 {
    param($g, $w, $h)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
    $g.DrawLine([System.Drawing.Pens]::White, 2, 0, 0, 2)
    $g.DrawLine([System.Drawing.Pens]::White, 2, 0, 14, 0)
    $g.DrawLine([System.Drawing.Pens]::White, 14, 0, 12, 2)
    $g.DrawLine([System.Drawing.Pens]::White, 12, 2, 4, 10)
    $g.DrawLine([System.Drawing.Pens]::White, 4, 10, 4, 12)
    $g.DrawLine([System.Drawing.Pens]::White, 2, 14, 0, 12)
    $g.DrawLine([System.Drawing.Pens]::White, 2, 14, 14, 14)
    $g.DrawLine([System.Drawing.Pens]::White, 14, 14, 12, 12)
}

# target - crosshair
New-FlatImage "target" 16 16 {
    param($g, $w, $h)
    $g.DrawEllipse([System.Drawing.Pens]::White, 4, 4, 8, 8)
    $g.DrawLine([System.Drawing.Pens]::White, 8, 0, 8, 5)
    $g.DrawLine([System.Drawing.Pens]::White, 8, 11, 8, 15)
    $g.DrawLine([System.Drawing.Pens]::White, 0, 8, 5, 8)
    $g.DrawLine([System.Drawing.Pens]::White, 11, 8, 15, 8)
}

# checkers - checkerboard pattern
New-FlatImage "checkers" 8 8 {
    param($g, $w, $h)
    $b1 = [System.Drawing.Brushes]::White
    $b2 = [System.Drawing.Brushes]::Gray
    for ($y = 0; $y -lt $h; $y++) {
        for ($x = 0; $x -lt $w; $x++) {
            $b = if (($x + $y) % 2 -eq 0) { $b1 } else { $b2 }
            $g.FillRectangle($b, $x, $y, 1, 1)
        }
    }
}

Write-Host "Flat theme images generated."

# ========================
# PIXEL THEME IMAGES
# ========================
Write-Host "Generating pixel theme images..."

# dropdown_arrow - pixel art down triangle
New-PixelImage "dropdown_arrow" 7 5 {
    param($b, $w, $h)
    Set-Px $b 3 0 255 255 255 255
    Set-Px $b 2 1 255 255 255 255; Set-Px $b 3 1 255 255 255 255; Set-Px $b 4 1 255 255 255 255
    Set-Px $b 1 2 255 255 255 255; Set-Px $b 2 2 255 255 255 255; Set-Px $b 3 2 255 255 255 255; Set-Px $b 4 2 255 255 255 255; Set-Px $b 5 2 255 255 255 255
    Set-Px $b 0 3 255 255 255 255; Set-Px $b 1 3 255 255 255 255; Set-Px $b 2 3 255 255 255 255; Set-Px $b 3 3 255 255 255 255; Set-Px $b 4 3 255 255 255 255; Set-Px $b 5 3 255 255 255 255; Set-Px $b 6 3 255 255 255 255
    Set-Px $b 3 4 255 255 255 255
}

# expandable_expanded_arrow / treenode_expanded_arrow
Copy-Item "$pixelDir\dropdown_arrow.png" "$pixelDir\expandable_expanded_arrow.png"
Copy-Item "$pixelDir\dropdown_arrow.png" "$pixelDir\treenode_expanded_arrow.png"

# expandable_collapsed_arrow / treenode_collapsed_arrow - right triangle
New-PixelImage "expandable_collapsed_arrow" 5 7 {
    param($b, $w, $h)
    Set-Px $b 3 0 255 255 255 255
    Set-Px $b 2 1 255 255 255 255; Set-Px $b 3 1 255 255 255 255
    Set-Px $b 1 2 255 255 255 255; Set-Px $b 2 2 255 255 255 255; Set-Px $b 3 2 255 255 255 255
    Set-Px $b 0 3 255 255 255 255; Set-Px $b 1 3 255 255 255 255; Set-Px $b 2 3 255 255 255 255; Set-Px $b 3 3 255 255 255 255
    Set-Px $b 1 4 255 255 255 255; Set-Px $b 2 4 255 255 255 255; Set-Px $b 3 4 255 255 255 255
    Set-Px $b 2 5 255 255 255 255; Set-Px $b 3 5 255 255 255 255
    Set-Px $b 3 6 255 255 255 255
}
Copy-Item "$pixelDir\expandable_collapsed_arrow.png" "$pixelDir\treenode_collapsed_arrow.png"

# check_mark - pixel check mark
New-PixelImage "check_mark" 7 7 {
    param($b, $w, $h)
    Set-Px $b 1 4 255 255 255 255
    Set-Px $b 2 5 255 255 255 255
    Set-Px $b 3 3 255 255 255 255; Set-Px $b 3 4 255 255 255 255; Set-Px $b 3 5 255 255 255 255
    Set-Px $b 4 2 255 255 255 255; Set-Px $b 4 3 255 255 255 255
    Set-Px $b 5 1 255 255 255 255; Set-Px $b 5 2 255 255 255 255
    Set-Px $b 6 0 255 255 255 255; Set-Px $b 6 1 255 255 255 255
}

# radio_body - pixel circle
New-PixelImage "radio_body" 13 13 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 1 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 0 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 1 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 4 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 5 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 6 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 7 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 8 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 9 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 9 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 10 $c[0] $c[1] $c[2] $c[3]
}

# radio_mark - filled pixel circle
New-PixelImage "radio_mark" 7 7 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 1 1 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 1 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 1 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 1 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 4 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 5 $c[0] $c[1] $c[2] $c[3]
}

# slider - pixel track
New-PixelImage "slider" 3 13 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 1 4 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 5 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 6 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 7 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 8 $c[0] $c[1] $c[2] $c[3]
}

# slider_filled
Copy-Item "$pixelDir\slider.png" "$pixelDir\slider_filled.png"

# slider_knob - pixel square
New-PixelImage "slider_knob" 7 7 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 0 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 0 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 1 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 4 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 5 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 6 $c[0] $c[1] $c[2] $c[3]
}

# comboslider_left_arrow
New-PixelImage "comboslider_left_arrow" 5 7 {
    param($b, $w, $h)
    Set-Px $b 3 0 255 255 255 255; Set-Px $b 3 1 255 255 255 255; Set-Px $b 2 1 255 255 255 255; Set-Px $b 3 2 255 255 255 255; Set-Px $b 2 2 255 255 255 255; Set-Px $b 1 2 255 255 255 255; Set-Px $b 3 3 255 255 255 255; Set-Px $b 2 3 255 255 255 255; Set-Px $b 1 3 255 255 255 255; Set-Px $b 0 3 255 255 255 255; Set-Px $b 3 4 255 255 255 255; Set-Px $b 2 4 255 255 255 255; Set-Px $b 1 4 255 255 255 255; Set-Px $b 3 5 255 255 255 255; Set-Px $b 2 5 255 255 255 255; Set-Px $b 3 6 255 255 255 255
}

# comboslider_right_arrow
New-PixelImage "comboslider_right_arrow" 5 7 {
    param($b, $w, $h)
    Set-Px $b 1 0 255 255 255 255; Set-Px $b 1 1 255 255 255 255; Set-Px $b 2 1 255 255 255 255; Set-Px $b 1 2 255 255 255 255; Set-Px $b 2 2 255 255 255 255; Set-Px $b 3 2 255 255 255 255; Set-Px $b 1 3 255 255 255 255; Set-Px $b 2 3 255 255 255 255; Set-Px $b 3 3 255 255 255 255; Set-Px $b 4 3 255 255 255 255; Set-Px $b 1 4 255 255 255 255; Set-Px $b 2 4 255 255 255 255; Set-Px $b 3 4 255 255 255 255; Set-Px $b 1 5 255 255 255 255; Set-Px $b 2 5 255 255 255 255; Set-Px $b 1 6 255 255 255 255
}

# rotaryslider_body - pixel circle
New-PixelImage "rotaryslider_body" 19 19 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 8 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 0 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 7 1 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 11 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 6 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 12 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 5 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 13 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 4 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 14 4 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 3 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 15 5 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 16 6 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 16 7 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 16 8 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 9 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 16 9 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 16 10 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 3 11 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 15 11 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 4 12 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 14 12 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 5 13 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 13 13 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 6 14 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 12 14 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 7 15 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 11 15 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 8 16 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 16 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 16 $c[0] $c[1] $c[2] $c[3]
}

# clear - pixel X
New-PixelImage "clear" 7 7 {
    param($b, $w, $h)
    Set-Px $b 0 0 255 255 255 255; Set-Px $b 1 1 255 255 255 255; Set-Px $b 2 2 255 255 255 255; Set-Px $b 3 3 255 255 255 255; Set-Px $b 4 4 255 255 255 255; Set-Px $b 5 5 255 255 255 255; Set-Px $b 6 6 255 255 255 255
    Set-Px $b 6 0 255 255 255 255; Set-Px $b 5 1 255 255 255 255; Set-Px $b 4 2 255 255 255 255; Set-Px $b 2 4 255 255 255 255; Set-Px $b 1 5 255 255 255 255; Set-Px $b 0 6 255 255 255 255
}

# submenu_item_arrow
Copy-Item "$pixelDir\expandable_collapsed_arrow.png" "$pixelDir\submenu_item_arrow.png"

# menu_item_checkmark
Copy-Item "$pixelDir\check_mark.png" "$pixelDir\menu_item_checkmark.png"

# sign_error - pixel X in circle
New-PixelImage "sign_error" 11 11 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 4 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 0 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 1 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 7 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 4 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 7 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 5 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 6 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 7 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 7 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 8 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 9 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 9 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 4 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 10 $c[0] $c[1] $c[2] $c[3]
}

# sign_info - pixel i in circle
New-PixelImage "sign_info" 11 11 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 4 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 0 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 1 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 4 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 5 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 6 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 7 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 8 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 9 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 9 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 4 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 10 $c[0] $c[1] $c[2] $c[3]
}

# sign_question - pixel ? in circle
New-PixelImage "sign_question" 11 11 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 4 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 0 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 1 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 4 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 5 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 6 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 7 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 8 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 9 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 9 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 4 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 10 $c[0] $c[1] $c[2] $c[3]
}

# sign_warning - pixel ! in triangle
New-PixelImage "sign_warning" 11 11 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 5 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 4 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 3 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 7 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 7 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 4 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 7 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 5 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 6 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 7 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 8 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 5 9 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 4 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 10 $c[0] $c[1] $c[2] $c[3]
}

# xaxis - pixel X arrow
New-PixelImage "xaxis" 11 11 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 5 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 4 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 5 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 5 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 6 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 7 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 8 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 9 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 9 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 5 10 $c[0] $c[1] $c[2] $c[3]
}

# yaxis - pixel Y arrow
New-PixelImage "yaxis" 11 11 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 5 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 4 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 5 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 5 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 5 6 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 4 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 7 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 3 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 7 8 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 9 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 9 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 9 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 9 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 9 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 7 9 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 9 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 7 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 10 $c[0] $c[1] $c[2] $c[3]
}

# zaxis - pixel Z arrow
New-PixelImage "zaxis" 11 11 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 1 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 7 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 0 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 9 1 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 8 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 7 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 5 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 3 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 4 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 5 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 2 6 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 6 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 3 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 7 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 7 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 5 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 7 8 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 8 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 9 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 9 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 1 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 7 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 10 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 10 $c[0] $c[1] $c[2] $c[3]
}

# target - pixel crosshair
New-PixelImage "target" 11 11 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 5 0 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 5 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 5 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 5 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 6 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 7 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 8 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 9 5 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 10 5 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 5 8 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 5 9 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 5 10 $c[0] $c[1] $c[2] $c[3]
}

# checkers - pixel checkerboard
New-PixelImage "checkers" 8 8 {
    param($b, $w, $h)
    for ($y = 0; $y -lt $h; $y++) {
        for ($x = 0; $x -lt $w; $x++) {
            $v = if (($x + $y) % 2 -eq 0) { 255 } else { 100 }
            Set-Px $b $x $y $v $v $v 255
        }
    }
}

# ========================
# Shared pixel 9-slice border images
# ========================
# button_normal - 9-slice with pixel border
New-PixelImage "button_normal" 5 5 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 0 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 0 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 4 $c[0] $c[1] $c[2] $c[3]
}
# For pixels without a proper border, just use the pixel image
New-PixelImage "button_pressed" 5 5 {
    param($b, $w, $h)
    $c = @(200,200,200,255)
    Set-Px $b 0 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 0 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 4 $c[0] $c[1] $c[2] $c[3]
}

# button_group segments
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\button_group_left_normal.png"
Copy-Item "$pixelDir\button_pressed.png" "$pixelDir\button_group_left_pressed.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\button_group_middle_normal.png"
Copy-Item "$pixelDir\button_pressed.png" "$pixelDir\button_group_middle_pressed.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\button_group_right_normal.png"
Copy-Item "$pixelDir\button_pressed.png" "$pixelDir\button_group_right_pressed.png"

# dropdown
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\dropdown.png"

# check_normal
New-PixelImage "check_normal" 5 5 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 0 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 0 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 4 $c[0] $c[1] $c[2] $c[3]
}

# text_input variants
New-PixelImage "text_input_normal" 5 5 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 0 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 0 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 1 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 2 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 3 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 4 $c[0] $c[1] $c[2] $c[3]
}
Copy-Item "$pixelDir\text_input_normal.png" "$pixelDir\text_input_focused.png"
Copy-Item "$pixelDir\text_input_normal.png" "$pixelDir\multiline_text_input_linenumbg.png"

# line
New-PixelImage "line" 3 1 {
    param($b, $w, $h)
    Set-Px $b 0 0 255 255 255 255; Set-Px $b 1 0 255 255 255 255; Set-Px $b 2 0 255 255 255 255
}

# expandable
New-PixelImage "expandable" 5 5 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 0 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 0 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 4 $c[0] $c[1] $c[2] $c[3]
}

# menu images
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\menu_bar.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\menu_body.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\menu_item_body.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\menu_item_separator.png"

# scrollview
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\scrollview.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\scrollview_scrollbar.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\scrollview_scrollbar_h.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\scrollview_scrollthumb.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\scrollview_scrollthumb_h.png"

# window related
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\window_body.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\window_dock_guide_vertical_split.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\window_dock_guide_horizontal_split.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\window_dock_guide_as_tab.png"

# tab images
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\tabgroup.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\tab_active.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\tab_active_focused.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\tab_inactive.png"

# progress
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\progress_back.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\progress_fill.png"

# selectable
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\selectable.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\selectable_hovered.png"

# box / axis
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\box.png"
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\axis_box.png"

# tooltip
New-PixelImage "tooltip" 5 5 {
    param($b, $w, $h)
    $c = @(255,255,255,255)
    Set-Px $b 0 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 0 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 0 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 1 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 2 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 3 $c[0] $c[1] $c[2] $c[3]
    Set-Px $b 0 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 1 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 2 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 3 4 $c[0] $c[1] $c[2] $c[3]; Set-Px $b 4 4 $c[0] $c[1] $c[2] $c[3]
}

# popup
Copy-Item "$pixelDir\button_normal.png" "$pixelDir\popup.png"

# rotaryslider_mark and value_dot
New-PixelImage "rotaryslider_mark" 3 3 {
    param($b, $w, $h)
    Set-Px $b 0 0 255 255 255 255; Set-Px $b 1 0 255 255 255 255; Set-Px $b 2 0 255 255 255 255
    Set-Px $b 0 1 255 255 255 255; Set-Px $b 1 1 255 255 255 255; Set-Px $b 2 1 255 255 255 255
    Set-Px $b 0 2 255 255 255 255; Set-Px $b 1 2 255 255 255 255; Set-Px $b 2 2 255 255 255 255
}
New-PixelImage "rotaryslider_value_dot" 3 3 {
    param($b, $w, $h)
    Set-Px $b 0 0 255 255 255 255; Set-Px $b 1 0 255 255 255 255; Set-Px $b 2 0 255 255 255 255
    Set-Px $b 0 1 255 255 255 255; Set-Px $b 1 1 255 255 255 255; Set-Px $b 2 1 255 255 255 255
    Set-Px $b 0 2 255 255 255 255; Set-Px $b 1 2 255 255 255 255; Set-Px $b 2 2 255 255 255 255
}

# multiline_text_input_wordwrap
New-PixelImage "multiline_text_input_wordwrap" 7 5 {
    param($b, $w, $h)
    Set-Px $b 0 0 255 255 255 255; Set-Px $b 1 0 255 255 255 255; Set-Px $b 2 0 255 255 255 255
    Set-Px $b 0 1 255 255 255 255
    Set-Px $b 0 2 255 255 255 255; Set-Px $b 1 2 255 255 255 255
    Set-Px $b 0 3 255 255 255 255
    Set-Px $b 4 0 255 255 255 255; Set-Px $b 5 0 255 255 255 255; Set-Px $b 6 0 255 255 255 255
    Set-Px $b 5 1 255 255 255 255
    Set-Px $b 4 2 255 255 255 255
    Set-Px $b 5 3 255 255 255 255
    Set-Px $b 4 4 255 255 255 255; Set-Px $b 5 4 255 255 255 255; Set-Px $b 6 4 255 255 255 255
}

Write-Host "All pixel theme images generated."
Write-Host "Done! Generated images for both flat and pixel themes."
