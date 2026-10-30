#!/usr/bin/env bash
# Build current source (incremental) and run the real Qt GUI on Windows.
# Called from BUILD_AND_RUN.cmd; can also be run in an MSYS2 UCRT64 terminal.
set -Eeuo pipefail

fail() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }
[[ "${MSYSTEM:-}" == 'UCRT64' ]] || fail 'Use MSYS2 UCRT64, not an MSYS, MINGW64, or ordinary Git Bash shell.'

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
project_dir="$(cd -- "$script_dir/.." && pwd -P)"
cd -- "$project_dir"
[[ -f CMakeLists.txt ]] || fail 'CMakeLists.txt not found in project directory.'

# Ensure UCRT64 Qt and the UCRT64 GCC are used rather than programs from other PATHs.
export PATH="/ucrt64/bin:/usr/bin:$PATH"
for tool in cmake ninja g++ ; do
  command -v "$tool" >/dev/null 2>&1 || fail "Missing $tool. Run INSTALL_BUILD_TOOLS.cmd once."
done

build_dir="$project_dir/build-ucrt64"
# CMake caches absolute source paths. Invalidate a cache copied from another PC
# or moved/renamed source folder, but never delete the source itself.
if [[ -f "$build_dir/CMakeCache.txt" ]]; then
  cached_source="$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "$build_dir/CMakeCache.txt" | head -n 1)"
  cached_generator="$(sed -n 's/^CMAKE_GENERATOR:INTERNAL=//p' "$build_dir/CMakeCache.txt" | head -n 1)"
  cached_posix=""
  if [[ -n "$cached_source" ]]; then
    cached_posix="$(cygpath -u "$cached_source" 2>/dev/null || printf '%s' "$cached_source")"
  fi
  if [[ "$cached_posix" != "$project_dir" || "$cached_generator" != 'Ninja' ]]; then
    echo '[INFO] Old CMake cache belongs to another path/generator; resetting generated build directory.'
    cmake -E rm -rf "$build_dir"
  fi
fi

printf '\n[1/3] Configure Qt 6 GUI (current source)\n'
cmake -S "$project_dir" -B "$build_dir" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DHUFFMAN_BUILD_GUI=ON \
  -DHUFFMAN_BUILD_TESTS=ON || fail 'Configuration failed. Ensure Qt 6 UCRT64 packages are installed.'

printf '\n[2/3] Build all changed C++/QML files\n'
cmake --build "$build_dir" --parallel 4 || fail 'C++/QML compilation failed; review diagnostics above.'

if [[ "${HUFFMAN_RUN_TESTS:-0}" == '1' ]]; then
  printf '\n[OPTIONAL] Run tests\n'
  ctest --test-dir "$build_dir" --output-on-failure || fail 'Tests failed.'
fi

printf '\n[3/3] Run newly rebuilt Qt GUI\n'
exe="$build_dir/gui/huffman_gui.exe"
[[ -f "$exe" ]] || fail "GUI executable not found: $exe"
# Calling from UCRT64 gives the GUI access to Qt/MinGW DLLs through PATH.
# The GUI has no dependency on Internet access.
"$exe" || fail 'The GUI failed or exited with an error.'
