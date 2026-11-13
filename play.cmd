@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "appdir=%~dp0out\build\win-amd64-relwithdebinfo"
set "game=%~dp0..\private\game"
set "profile=%LOCALAPPDATA%\Over G Fighters Recomp"
set "backend=xinput"
if /i "%~1"=="sdl" (
    set "backend=sdl"
    shift
) else if /i "%~1"=="xinput" (
    shift
)
set "gpu_option="
if /i "%~1"=="overg-resolve" (
    set "gpu_option=--gpu_plugin=overg-resolve"
    shift
)
if not [%1]==[] goto usage
if not exist "%appdir%\overg.exe" (
    echo Build the port with scripts\build.ps1 before playing.
    pause
    exit /b 1
)
if not exist "%game%\default.xex" (
    echo Prepare your disc with scripts\prepare-game.ps1 before playing.
    pause
    exit /b 1
)
if defined gpu_option if not exist "%appdir%\rexgpu-overg-resolverd.dll" (
    echo Build the optional plugin and stage it beside overg.exe as described in docs\empty-resolve.md.
    pause
    exit /b 1
)
if not exist "%profile%\logs\" mkdir "%profile%\logs"
if not exist "%profile%\logs\" (
    echo Could not create the player profile in "%profile%".
    pause
    exit /b 1
)
start "" /D "%appdir%" "%appdir%\overg.exe" "--game_data_root=%game%" "--user_data_root=%profile%" "--input_backend=%backend%" %gpu_option% "--log_file=%profile%\logs\play-%RANDOM%-%RANDOM%.log" --log_level=info --log_flush_interval=1
exit /b %errorlevel%

:usage
echo Usage: play.cmd [xinput^|sdl] [overg-resolve]
exit /b 2
