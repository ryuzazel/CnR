#!/usr/bin/env bash
# Cross-compile install.exe (the native PATH-installer for CnR) from Linux
# using MinGW-w64. Produces a standalone Windows .exe -- no dependencies,
# no admin rights needed to run it (writes to HKCU, the per-user registry).
#
# After this AND build-windows-mingw.sh have both run, copy both CnR.exe
# and install.exe to the same folder on a Windows machine and double-click
# install.exe.
set -euo pipefail

x86_64-w64-mingw32-g++ \
  -std=c++17 -O2 -static \
  scripts/install.cpp -o install.exe -lole32 -luuid

echo "Built install.exe."
echo "Copy it alongside CnR.exe on Windows and double-click install.exe to install + add to PATH."