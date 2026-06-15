@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
set "GAME_EXE=%ROOT%bin\x64\Release\game.exe"

if not exist "%GAME_EXE%" (
    echo release game executable was not found:
    echo %GAME_EXE%
    echo.
    echo Run build_and_run_x64_release.bat first to build the game.
    pause
    exit /b 1
)

pushd "%ROOT%" >nul
start "" "%GAME_EXE%" %*
popd >nul
exit /b 0
