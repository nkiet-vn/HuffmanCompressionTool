@echo off
setlocal EnableExtensions DisableDelayedExpansion
TITLE Huffman Compression Tool - Install Build Dependencies
if not defined MSYS2_ROOT set "MSYS2_ROOT=C:\msys64"
if not exist "%MSYS2_ROOT%\usr\bin\bash.exe" (
  echo ERROR: Install MSYS2 from https://www.msys2.org/ first.
  echo Expected location: %MSYS2_ROOT%
  pause
  exit /b 1
)
set "MSYSTEM=UCRT64"
set "CHERE_INVOKING=1"
REM One-time developer dependencies. The receiving PC needs network for package install.
"%MSYS2_ROOT%\usr\bin\bash.exe" --login -c "pacman -S --needed mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-declarative mingw-w64-ucrt-x86_64-qt6-tools"
if errorlevel 1 (
  echo.
  echo Installation incomplete. If MSYS2 needs updating, launch MSYS2 UCRT64
  echo and run pacman -Syu, restart the shell, then retry this installer.
  pause
  exit /b 1
)
echo.
echo Toolchain ready. Double-click BUILD_AND_RUN.cmd to build and launch.
pause
exit /b 0