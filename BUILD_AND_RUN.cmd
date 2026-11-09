@echo off
setlocal EnableExtensions DisableDelayedExpansion
TITLE Huffman Compression Tool - Build and Run

REM This file stays beside CMakeLists.txt. Can run from any folder/location.
cd /d "%~dp0"
if errorlevel 1 (
  echo ERROR: Cannot open project directory.
  pause
  exit /b 1
)
if not exist "CMakeLists.txt" (
  echo ERROR: CMakeLists.txt not found next to BUILD_AND_RUN.cmd.
  pause
  exit /b 1
)

REM MSYS2 root is normally C:\msys64; an override is supported.
if not defined MSYS2_ROOT set "MSYS2_ROOT=C:\msys64"
if not exist "%MSYS2_ROOT%\usr\bin\bash.exe" (
  echo.
  echo ERROR: MSYS2 was not found: %MSYS2_ROOT%
  echo Install MSYS2 first, then run INSTALL_BUILD_TOOLS.cmd once.
  echo Website: https://www.msys2.org/
  echo For a non-default path, set the MSYS2_ROOT environment variable.
  echo.
  pause
  exit /b 1
)

REM Launch a proper UCRT64 login shell, preserving this project directory.
set "MSYSTEM=UCRT64"
set "CHERE_INVOKING=1"
echo [BUILD] Project: %CD%
"%MSYS2_ROOT%\usr\bin\bash.exe" --login -c "bash ./scripts/build-and-run-ucrt64.sh"
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" (
  echo.
  echo [FAILED] Build, tests, or application exited with code %RESULT%.
  echo Check the error above. This window will remain open.
  echo.
  pause
  exit /b %RESULT%
)
echo.
echo [DONE] Huffman GUI has closed.
exit /b 0