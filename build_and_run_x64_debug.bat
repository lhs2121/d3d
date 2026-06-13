@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
set "GAME_EXE=%ROOT%bin\x64\Debug\game.exe"
set "VS_INSTALLER=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
set "VS_MSBUILD_CMD="
set "NEEDS_BUILD=1"

call :checkUpToDate
if "%NEEDS_BUILD%"=="0" (
    echo build is up to date. running game...
    goto :runGame
)

call :buildAll
if errorlevel 1 (
    echo.
    echo build failed. Check the messages above.
    pause
    exit /b 1
)

:runGame
if not exist "%GAME_EXE%" (
    echo game executable was not found:
    echo %GAME_EXE%
    pause
    exit /b 1
)

pushd "%ROOT%" >nul
start "" "%GAME_EXE%" %*
popd >nul
exit /b 0

:checkUpToDate
set "ROOT_DIR=%ROOT%"
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$ErrorActionPreference='Stop';" ^
    "$root=$env:ROOT_DIR;" ^
    "$outputs=@('bin\x64\Debug\game.exe','bin\x64\Debug\commonlib.dll','bin\x64\Debug\inputlib.dll','bin\x64\Debug\collib.dll','bin\x64\Debug\windowlib.dll','bin\x64\Debug\rendererlib.dll') | ForEach-Object { Join-Path $root $_ };" ^
    "foreach($output in $outputs){ if(!(Test-Path -LiteralPath $output)){ exit 1 } }" ^
    "$oldestOutput=($outputs | ForEach-Object { (Get-Item -LiteralPath $_).LastWriteTimeUtc } | Sort-Object | Select-Object -First 1);" ^
    "$roots=@('commonlib','inputlib','collib','windowlib','rendererlib','game','build') | ForEach-Object { Join-Path $root $_ };" ^
    "$extensions=@('.cpp','.c','.h','.hpp','.hlsl','.vcxproj','.filters','.sln','.props');" ^
    "foreach($sourceRoot in $roots){ if(!(Test-Path -LiteralPath $sourceRoot)){ exit 1 } $newer=Get-ChildItem -LiteralPath $sourceRoot -Recurse -File | Where-Object { $extensions -contains $_.Extension.ToLowerInvariant() -and $_.LastWriteTimeUtc -gt $oldestOutput } | Select-Object -First 1; if($newer){ exit 1 } }" ^
    "exit 0"
if errorlevel 1 (
    set "NEEDS_BUILD=1"
) else (
    set "NEEDS_BUILD=0"
)
exit /b 0

:buildAll
pushd "%VS_INSTALLER%" >nul 2>nul
if errorlevel 1 goto UseDefaultVsPath

for /f "tokens=*" %%i in ('vswhere.exe -version [18.0^,19.0^) -requires Microsoft.Component.MSBuild -property installationPath') do (
    set "VS_MSBUILD_CMD=%%i\Common7\Tools\VsMSBuildCmd.bat"
)

popd >nul

:UseDefaultVsPath
if "%VS_MSBUILD_CMD%"=="" (
    set "VS_MSBUILD_CMD=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsMSBuildCmd.bat"
)

if not exist "%VS_MSBUILD_CMD%" (
    echo Visual Studio build environment was not found:
    echo %VS_MSBUILD_CMD%
    exit /b 1
)

call "%VS_MSBUILD_CMD%" -arch=x64
if errorlevel 1 (
    echo Failed to initialize Visual Studio build environment.
    exit /b 1
)

call :buildProject "commonlib" "%ROOT%commonlib\commonlib.sln"
if errorlevel 1 exit /b 1

call :buildProject "inputlib" "%ROOT%inputlib\inputlib.sln"
if errorlevel 1 exit /b 1

call :buildProject "collib" "%ROOT%collib\collib.sln"
if errorlevel 1 exit /b 1

call :buildProject "windowlib" "%ROOT%windowlib\windowlib.sln"
if errorlevel 1 exit /b 1

call :buildProject "rendererlib" "%ROOT%rendererlib\rendererlib.sln"
if errorlevel 1 exit /b 1

call :buildProject "game" "%ROOT%game\game.sln"
if errorlevel 1 exit /b 1

echo.
echo build succeeded.
exit /b 0

:buildProject
echo.
echo ============================================================
echo building %~1
echo %~2
echo ============================================================

if not exist "%~2" (
    echo Missing project or solution: %~2
    exit /b 1
)

msbuild "%~2" /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
if errorlevel 1 (
    echo.
    echo build failed: %~1
    exit /b 1
)

echo build succeeded: %~1
exit /b 0
