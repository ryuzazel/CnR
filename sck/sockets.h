
#pragma once

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>

  #pragma comment(lib, "Ws2_32.lib")

  // ---- POSIX close() -> Winsock closesocket() ----
  // CnR.cpp calls close(fd) on socket descriptors throughout. On POSIX,
  // close() works for any fd (files or sockets); on Windows, sockets are a
  // SOCKET type and must be released with closesocket(), not the CRT's
  // file-oriented _close(). Since every close() call site in the socket
  // code operates on a socket fd, this #define is safe here.
  #define close(fd) closesocket(fd)

  // ---- POSIX ssize_t -> Windows equivalent ----
  #ifndef _SSIZE_T_DEFINED
    typedef long long ssize_t;
    #define _SSIZE_T_DEFINED
  #endif

  // ---- POSIX socklen_t (Winsock's is already `int`-compatible) ----
  #ifndef _SOCKLEN_T_DEFINED
    typedef int socklen_t;
    #define _SOCKLEN_T_DEFINED
  #endif

  // ---- gai_strerror returns wchar_t* on Windows; CnR.cpp expects char*.
  // ws2tcpip.h already #defines gai_strerror to __mingw_gai_strerrorA (via
  // __MINGW_NAME_AW), so it must be undef'd before this redefinition or
  // the compiler warns about a macro collision. ----
  inline std::string cnrGaiStrerror(int err) {
      char buf[256];
      DWORD n = FormatMessageA(
          FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
          nullptr, err, 0, buf, sizeof(buf), nullptr);
      return n ? std::string(buf) : ("getaddrinfo error " + std::to_string(err));
  }
  #ifdef gai_strerror
    #undef gai_strerror
  #endif
  #define gai_strerror(e) cnrGaiStrerror(e).c_str()

  // ---- Winsock requires explicit startup/shutdown; POSIX does not.
  // A static initializer runs this once before any socket code executes,
  // so CnR.cpp doesn't need to call anything extra itself. ----
  struct CnrWinsockInit {
      CnrWinsockInit() {
          WSADATA wsaData;
          WSAStartup(MAKEWORD(2, 2), &wsaData);
      }
      ~CnrWinsockInit() {
          WSACleanup();
      }
  };
  inline CnrWinsockInit cnrWinsockInitInstance;

  // ---- POSIX setsockopt() takes `const void*` for optval; Winsock's
  // takes `const char*`. CnR.cpp calls setsockopt(fd, SOL_SOCKET,
  // SO_REUSEADDR, &opt, sizeof(opt)) with an int*, which POSIX accepts
  // implicitly but Winsock's stricter char* signature rejects. This
  // wrapper adds the cast so the call site in CnR.cpp needs no change. ----
  inline int cnrSetsockopt(SOCKET s, int level, int optname, const void* optval, int optlen) {
      return setsockopt(s, level, optname, reinterpret_cast<const char*>(optval), optlen);
  }
  #define setsockopt(s, level, optname, optval, optlen) \
      cnrSetsockopt((s), (level), (optname), (optval), (optlen))

  // ---- SIGPIPE doesn't exist on Windows (no POSIX-style broken-pipe
  // signal; a failed send() just returns an error instead). CnR.cpp calls
  // std::signal(SIGPIPE, SIG_IGN) once at startup to prevent the process
  // being killed by a client disconnect on Linux. On Windows that
  // scenario can't happen, so this call just needs to compile and be a
  // no-op -- NOT reuse a real signal like SIGTERM, which could later be
  // sent for an actual reason. 0 is not a valid raised signal number on
  // Windows, so std::signal(0, SIG_IGN) registers harmlessly. ----
  #ifndef SIGPIPE
    #define SIGPIPE 0
  #endif

#else
  // ---- Non-Windows (Linux/macOS): use the real POSIX headers, unchanged ----
  #include <sys/socket.h>
  #include <sys/types.h>
  #include <netdb.h>
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
#endif