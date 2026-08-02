#!/usr/bin/env bash
# Build and install CnR using CMake.
# Usage:
#   ./scripts/build.sh              # build + install to /usr/local (needs sudo)
#   ./scripts/build.sh --user       # install to ~/.local instead (no sudo)
#   ./scripts/build.sh --portable   # disable -march=native (safe to redistribute)
set -euo pipefail

PREFIX="/usr/local"
CMAKE_EXTRA=()
NEED_SUDO=1

for arg in "$@"; do
  case "$arg" in
    --user)
      PREFIX="$HOME/.local"
      NEED_SUDO=0
      ;;
    --portable)
      CMAKE_EXTRA+=(-DCNR_NATIVE_ARCH=OFF)
      ;;
    *)
      echo "Unknown option: $arg" >&2
      exit 1
      ;;
  esac
done

BUILD_DIR="build"
rm -rf "$BUILD_DIR"
cmake -S . -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  "${CMAKE_EXTRA[@]}"

cmake --build "$BUILD_DIR" --parallel "$(nproc 2>/dev/null || sysctl -n hw.ncpu)"

if [[ "$NEED_SUDO" -eq 1 ]]; then
  echo "Installing to $PREFIX (requires sudo)..."
  sudo cmake --install "$BUILD_DIR"
else
  mkdir -p "$PREFIX/bin"
  cmake --install "$BUILD_DIR"
  if [[ ":$PATH:" != *":$PREFIX/bin:"* ]]; then
    echo
    echo "NOTE: $PREFIX/bin is not on your PATH yet. Add this to your ~/.bashrc or ~/.zshrc:"
    echo "  export PATH=\"$PREFIX/bin:\$PATH\""
  fi
fi

echo "Done. Try:  CnR \"args\""
