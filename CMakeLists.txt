cmake_minimum_required(VERSION 3.20)
project(CnR LANGUAGES CXX)

# ---- Options ----
option(CNR_NATIVE_ARCH "Build with -march=native (disable for portable builds/packaging)" ON)
option(CNR_LTO "Enable interprocedural / link-time optimization" ON)

# ---- Standard ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---- Sanity check: CnR.cpp is a unity build that #includes math_bridge.inc
# and the mathlib_flat/ sources — those files must sit alongside it. ----
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CnR.cpp")
  message(FATAL_ERROR "CnR.cpp not found in ${CMAKE_CURRENT_SOURCE_DIR}. "
                       "Place CnR.cpp, math_bridge.inc, and mathlib_flat/ here before configuring.")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/math_bridge.inc")
  message(WARNING "math_bridge.inc not found next to CnR.cpp — the unity build will fail to compile "
                   "unless CnR.cpp includes it via a different relative path.")
endif()

# ---- Executable ----
add_executable(CnR CnR.cpp)

# ---- Optimization flags (mirrors: g++ -std=c++17 -O3 -march=native -pthread) ----
target_compile_options(CnR PRIVATE
  $<$<CONFIG:Release>:-O3>
)

if(CNR_NATIVE_ARCH)
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-march=native" CNR_HAS_MARCH_NATIVE)
  if(CNR_HAS_MARCH_NATIVE)
    target_compile_options(CnR PRIVATE -march=native)
  else()
    message(WARNING "-march=native not supported by this compiler; skipping (use CNR_NATIVE_ARCH=OFF on non-x86/non-GCC-like compilers e.g. MSVC)")
  endif()
endif()

# ---- Threads (-pthread) ----
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(CnR PRIVATE Threads::Threads)

# ---- LTO ----
if(CNR_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT CNR_IPO_OK OUTPUT CNR_IPO_MSG)
  if(CNR_IPO_OK)
    set_property(TARGET CnR PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()
endif()

# ---- Install (this is what puts `CnR` on PATH) ----
include(GNUInstallDirs)
install(TARGETS CnR RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---- Packaging (CPack: feeds the .deb/.rpm generation below) ----
set(CPACK_PACKAGE_NAME "CnR")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_CONTACT "you@example.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CnR (Compile N Run) — mathematical, concurrent scripting language runtime")
set(CPACK_GENERATOR "TGZ;DEB;RPM")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "you@example.com")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
include(CPack)

message(STATUS "CnR configured: STD=C++26 O3 march-native=${CNR_NATIVE_ARCH} LTO=${CNR_LTO}")
