# Cross-compiles the Windows build with MinGW-w64 (brew install mingw-w64).
#   cmake -S platform/pc -B build-pc/windows --toolchain platform/pc/cmake/mingw-w64-x86_64.cmake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(MINGW_PREFIX x86_64-w64-mingw32)
set(CMAKE_C_COMPILER ${MINGW_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${MINGW_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${MINGW_PREFIX}-windres)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
