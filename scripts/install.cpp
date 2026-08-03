// install.cpp — compiles to a real install.exe for Windows.
// Copies CnR.exe (must sit next to this installer) into
// %LOCALAPPDATA%\Programs\CnR and permanently adds that folder to the
// current user's PATH via the registry -- no admin rights required,
// since HKCU\Environment is per-user.
//
// Build (from Linux with MinGW, or natively on Windows with MinGW/MSVC):
//   x86_64-w64-mingw32-g++ -std=c++17 -O2 -static install.cpp -o install.exe -municode
//
// -municode makes main() receive wide-char argv via wWinMain-style entry,
// but we keep it simple with regular main() + Win32 A (ANSI) registry
// calls, which is sufficient here since paths involved contain no
// unusual characters.

#include <windows.h>
#include <iostream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// Reads the current user's PATH from HKCU\Environment. Returns empty
// string if unset (rare, but possible on a fresh account).
static std::string readUserPath() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return "";

    DWORD type = 0, size = 0;
    if (RegQueryValueExA(hKey, "Path", nullptr, &type, nullptr, &size) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return "";
    }

    std::string value(size, '\0');
    RegQueryValueExA(hKey, "Path", nullptr, &type, reinterpret_cast<BYTE*>(value.data()), &size);
    RegCloseKey(hKey);

    // Trim trailing null(s) RegQueryValueEx may include in the buffer.
    while (!value.empty() && value.back() == '\0') value.pop_back();
    return value;
}

// Writes the new PATH value back to HKCU\Environment, then broadcasts
// WM_SETTINGCHANGE so already-open Explorer windows notice -- new
// terminal windows will pick it up automatically either way.
static bool writeUserPath(const std::string& newPath) {
    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Environment", 0, nullptr, 0,
                         KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
        return false;

    LONG result = RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ,
                                  reinterpret_cast<const BYTE*>(newPath.c_str()),
                                  static_cast<DWORD>(newPath.size() + 1));
    RegCloseKey(hKey);

    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                         reinterpret_cast<LPARAM>("Environment"),
                         SMTO_ABORTIFHUNG, 5000, nullptr);

    return result == ERROR_SUCCESS;
}

int main() {
    std::cout << "CnR installer\n" << std::string(40, '=') << "\n\n";

    // Installer expects CnR.exe sitting next to itself.
    fs::path exeDir = fs::current_path();
    fs::path srcExe = exeDir / "CnR.exe";

    if (!fs::exists(srcExe)) {
        std::cerr << "ERROR: CnR.exe not found next to install.exe.\n"
                     "Build CnR.exe first, place it in this folder, then re-run.\n";
        std::cin.get();
        return 1;
    }

    char* localAppData = nullptr;
    size_t len = 0;
    _dupenv_s(&localAppData, &len, "LOCALAPPDATA");
    if (!localAppData) {
        std::cerr << "ERROR: could not read %LOCALAPPDATA%.\n";
        std::cin.get();
        return 1;
    }
    fs::path installDir = fs::path(localAppData) / "Programs" / "CnR";
    free(localAppData);

    std::cout << "Installing to: " << installDir.string() << "\n";

    std::error_code ec;
    fs::create_directories(installDir, ec);
    if (ec) {
        std::cerr << "ERROR: could not create install directory: " << ec.message() << "\n";
        std::cin.get();
        return 1;
    }

    fs::copy_file(srcExe, installDir / "CnR.exe", fs::copy_options::overwrite_existing, ec);
    if (ec) {
        std::cerr << "ERROR: could not copy CnR.exe: " << ec.message() << "\n";
        std::cin.get();
        return 1;
    }

    std::string currentPath = readUserPath();
    std::string installDirStr = installDir.string();

    bool alreadyOnPath = currentPath.find(installDirStr) != std::string::npos;
    if (alreadyOnPath) {
        std::cout << installDirStr << " is already on your PATH.\n";
    } else {
        std::string newPath = currentPath.empty()
            ? installDirStr
            : currentPath + ";" + installDirStr;

        if (writeUserPath(newPath)) {
            std::cout << "Added " << installDirStr << " to your user PATH.\n"
                          "Close and reopen your terminal for this to take effect.\n";
        } else {
            std::cerr << "WARNING: could not update PATH automatically.\n"
                          "Add this folder to your PATH manually:\n  " << installDirStr << "\n";
        }
    }

    std::cout << "\nDone. Once your terminal is reopened, try:  CnR \"args\"\n\n"
                  "Press Enter to exit...";
    std::cin.get();
    return 0;
}