@echo off
:: Check for administrative privileges
net session >nul 2>&1
if %errorLevel% == 0 (
    goto :run_game
) else (
    goto :elevate
)

:elevate
:: Use PowerShell to request UAC and restart this script as admin
echo Requesting administrative privileges...
powershell -Command "Start-Process '%~f0' -Verb RunAs"
exit /b

:run_game
:: Once running as admin, execute the renamed game
if exist "GeometryDash666.exe" (
    echo Launching GeometryDash666.exe with high privileges...
    :: Use /wait to keep the script running until the game closes
    start /wait "" "GeometryDash666.exe"
) else (
    echo Error: GeometryDash666.exe not found in this directory.
    pause
)
exit
