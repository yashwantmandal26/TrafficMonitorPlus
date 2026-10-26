@echo off
:: Self-elevate to Administrator
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Requesting Administrator privileges to apply formatting and colors...
    powershell -Command "Start-Process cmd -ArgumentList '/c \"\"%~f0\"\"' -Verb RunAs"
    exit /b
)

echo Stopping TrafficMonitorPlus+...
taskkill /F /IM TrafficMonitorPlus+.exe >nul 2>&1
taskkill /F /IM TrafficMonitor.exe >nul 2>&1
timeout /t 1 /nobreak >nul

echo Applying white internet speed, white CPU/GPU labels, green normal values, and clean formatting...

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$progDir = 'C:\Program Files\TrafficMonitorPlus+';" ^
    "$configPath = \"$progDir\config.ini\";" ^
    "if (Test-Path $configPath) {" ^
    "    $content = [System.IO.File]::ReadAllText($configPath, [System.Text.Encoding]::UTF8);" ^
    "    $content = [System.Text.RegularExpressions.Regex]::Replace($content, 'language\s*=\s*.*', 'language = en-US|English|');" ^
    "    $content = [System.Text.RegularExpressions.Regex]::Replace($content, 'value_right_align\s*=\s*\w+', 'value_right_align = false');" ^
    "    $content = [System.Text.RegularExpressions.Regex]::Replace($content, 'item_space\s*=\s*\d+', 'item_space = 8');" ^
    "    $content = [System.Text.RegularExpressions.Regex]::Replace($content, 'tbar_display_item\s*=\s*\d+', 'tbar_display_item = 119');" ^
    "    $content = [System.Text.RegularExpressions.Regex]::Replace($content, 'hardware_monitor_item\s*=\s*\d+', 'hardware_monitor_item = 3');" ^
    "    $upArrow = [char]0x2191 + ' ';" ^
    "    $downArrow = [char]0x2193 + ' ';" ^
    "    $content = [System.Text.RegularExpressions.Regex]::Replace($content, 'up_string\s*=\s*.*', \"up_string = `\"$upArrow`\"\");" ^
    "    $content = [System.Text.RegularExpressions.Regex]::Replace($content, 'down_string\s*=\s*.*', \"down_string = `\"$downArrow`\"\");" ^
    "    $content = [System.Text.RegularExpressions.Regex]::Replace($content, 'cpu_string\s*=\s*.*', 'cpu_string = \"CPU: \"');" ^
    "    $content = [System.Text.RegularExpressions.Regex]::Replace($content, 'gpu_string\s*=\s*.*', 'gpu_string = \"GPU: \"');" ^
    "    $content = [System.Text.RegularExpressions.Regex]::Replace($content, 'cpu_temp_string\s*=\s*.*', 'cpu_temp_string = \"\"');" ^
    "    $content = [System.Text.RegularExpressions.Regex]::Replace($content, 'gpu_temp_string\s*=\s*.*', 'gpu_temp_string = \"\"');" ^
    "    $content = [System.Text.RegularExpressions.Regex]::Replace($content, 'task_bar_speed_short_mode\s*=\s*\w+', 'task_bar_speed_short_mode = true');" ^
    "    $content = [System.Text.RegularExpressions.Regex]::Replace($content, 'separate_value_unit_with_space\s*=\s*\w+', 'separate_value_unit_with_space = false');" ^
    "    $content = [System.Text.RegularExpressions.Regex]::Replace($content, 'digits_number\s*=\s*\d+', 'digits_number = 3');" ^
    "    $content = [System.Text.RegularExpressions.Regex]::Replace($content, 'specify_each_item_color\s*=\s*\w+', 'specify_each_item_color = true');" ^
    "    $itemColors = '16777215,16777215,16777215,16777215,16777215,6609228,16777215,16777215,16777215,6609228,16777215,10542160,16777215,10542160,16777215,16777215,16777215,16777215,16777215,16777215,16777215,16777215,16777215,16777215,16777215,16777215,';" ^
    "    $content = [System.Text.RegularExpressions.Regex]::Replace($content, 'task_bar_text_color\s*=\s*[\d,]+', \"task_bar_text_color = $itemColors\");" ^
    "    [System.IO.File]::WriteAllText($configPath, $content, [System.Text.Encoding]::UTF8);" ^
    "    Write-Host 'config.ini updated successfully!';" ^
    "};" ^
    "$langDir = \"$progDir\language\";" ^
    "$engIni = \"$langDir\English.ini\";" ^
    "if (Test-Path $engIni) {" ^
    "    Copy-Item $engIni \"$langDir\Simplified_Chinese.ini\" -Force -ErrorAction SilentlyContinue;" ^
    "    Copy-Item $engIni \"$langDir\Traditional_Chinese.ini\" -Force -ErrorAction SilentlyContinue;" ^
    "    Copy-Item $engIni \"$langDir\Japanese.ini\" -Force -ErrorAction SilentlyContinue;" ^
    "};" ^
    "Start-Process \"$progDir\TrafficMonitorPlus+.exe\";" ^
    "Write-Host 'TrafficMonitorPlus+ restarted successfully!';"

echo Done!
timeout /t 2 /nobreak >nul
exit
