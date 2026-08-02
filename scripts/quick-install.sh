#!/usr/bin/env bash
# Easiest possible install: no CMake, no package manager, just g++.
# Usage: ./scripts/quick-install.sh
set -euo pipefail

SRC="CnR.cpp"
DEST="${HOME}/.local/bin"
mkdir -p "$DEST"

echo "Compiling..."
g++ -std=c++17 -O3 -march=native -pthread "$SRC" -o "$DEST/CnR"
chmod +x "$DEST/CnR"

echo "Installed to $DEST/CnR"

if [[ ":$PATH:" != *":$DEST:"* ]]; then
  SHELL_RC="$HOME/.bashrc"
  [[ "$SHELL" == *zsh* ]] && SHELL_RC="$HOME/.zshrc"
  echo "export PATH=\"$DEST:\$PATH\"" >> "$SHELL_RC"
  echo "Added $DEST to PATH in $SHELL_RC — restart your terminal or run: source $SHELL_RC"
else
  echo "$DEST is already on PATH."
fi

echo "Try:  CnR \"args\""
