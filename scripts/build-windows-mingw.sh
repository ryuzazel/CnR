#!/usr/bin/env bash
# Cross-compile CnR.exe for Windows from Linux using MinGW-w64.
# Install first:  sudo pacman -S mingw-w64-gcc        (Arch)
#                 sudo apt install mingw-w64           (Debian/Ubuntu)
#                 sudo dnf install mingw64-gcc-c++      (Fedora)
set -euo pipefail

x86_64-w64-mingw32-g++ \
  -std=c++17 -O3 -pthread -static \
  CnR.cpp -o CnR.exe

echo "Built CnR.exe (statically linked, portable — copy it anywhere on Windows and it'll run)."
