#!/usr/bin/env bash
# Build .deb (Debian/Ubuntu) and .rpm (Fedora/RHEL/openSUSE) packages,
# plus a portable .tar.gz — all from the same CMakeLists.txt via CPack.
# Requires: cmake, and for .rpm on Debian-based systems: `sudo apt install rpm`
set -euo pipefail

BUILD_DIR="build-packages"
rm -rf "$BUILD_DIR"

cmake -S . -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCNR_NATIVE_ARCH=OFF   # portable: package will run on other machines with different CPUs

cmake --build "$BUILD_DIR" --parallel
cd "$BUILD_DIR"
cpack

echo
echo "Packages built:"
ls -1 *.deb *.rpm *.tar.gz 2>/dev/null || true
echo
echo "Install with:"
echo "  Debian/Ubuntu:  sudo dpkg -i CnR-*.deb"
echo "  Fedora/RHEL:    sudo rpm -i CnR-*.rpm     (or: sudo dnf install ./CnR-*.rpm)"
echo "  Any Linux:      tar xzf CnR-*.tar.gz  (then move CnR/bin/CnR wherever's on PATH)"
