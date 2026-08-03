# CnR — build & install options

Put your `CnR.cpp` in this folder alongside `CMakeLists.txt`. Then pick one:

## 1. CMake (recommended — puts `CnR` on PATH)
```bash
chmod +x scripts/build.sh
./scripts/build.sh              # installs to /usr/local/bin (sudo)
./scripts/build.sh --user       # installs to ~/.local/bin (no sudo)
./scripts/build.sh --portable   # no -march=native, safe to share the binary
```
Run it anywhere after: `CnR "args"`

## 2. Easy alternative (no CMake at all)
```bash
chmod +x scripts/quick-install.sh
./scripts/quick-install.sh
```
Straight `g++` compile + copy to `~/.local/bin`, adds it to PATH for you.

## 3. Arch Linux
```bash
cd packaging/arch
cp ../../CnR.cpp ../../CMakeLists.txt .
makepkg -si                     # portable build, safe for AUR
# or, for a build tuned to *this* machine only:
makepkg -p PKGBUILD-native -si
```

## 4. Debian/Ubuntu (.deb) and Fedora/RHEL (.rpm)
```bash
chmod +x scripts/build-packages.sh
./scripts/build-packages.sh
sudo dpkg -i build-packages/CnR-*.deb        # Debian/Ubuntu
sudo rpm -i build-packages/CnR-*.rpm         # Fedora/RHEL
```
Also produces a portable `.tar.gz` that works on any Linux distro.

## 5. Windows

**Option A — native MSVC (from Windows, Developer PowerShell):**
```powershell
.\scripts\build-windows.ps1
```
Installs to `%LOCALAPPDATA%\Programs\CnR` and adds it to your user PATH.

**Option B — cross-compile from Linux with MinGW:**
```js
Open ./scripts/install.bat
Installs and add to your user PATH.
```


## Notes
- `-march=native` only makes sense when building and running on the *same*
  CPU. Any package meant for other machines (Arch/deb/rpm, Windows cross-build)
  uses `-DCNR_NATIVE_ARCH=OFF` instead.
- C++26 requires a recent compiler: GCC 14+ or Clang 18+ recommended.
