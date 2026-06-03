@echo off
:: Comprobar si ya tenemos privilegios de administrador
net session >nul 2>&1
if %errorLevel% == 0 (
    goto :run_game
) else (
    goto :elevate
)

:elevate
:: Usar PowerShell para pedir UAC y volver a ejecutar este mismo bat como admin
powershell -Command "Start-Process '%~f0' -Verb RunAs"
exit /b

:run_game
:: Una vez como admin, ejecutamos el juego renombrado
if exist "GeometryDash666.exe" (
    start "" "GeometryDash666.exe"
) else (
    echo Error: No se encuentra GeometryDash666.exe en este directorio.
    pause
)
exit
