cmake_minimum_required(VERSION 3.16)
project(earbud_listen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to an optimized build when no build type is given.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# miniaudio (single-header audio library, wraps CoreAudio on macOS)
#
# If a miniaudio.h sits next to this file, it is used as-is. Otherwise a pinned
# release is downloaded once into the build directory.
# ---------------------------------------------------------------------------
set(MINIAUDIO_VERSION "0.11.25" CACHE STRING "miniaudio release tag to download")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/miniaudio.h")
  set(MINIAUDIO_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  message(STATUS "Using local miniaudio.h from ${MINIAUDIO_DIR}")
else()
  set(MINIAUDIO_DIR "${CMAKE_BINARY_DIR}/_deps/miniaudio-${MINIAUDIO_VERSION}")
  if(NOT EXISTS "${MINIAUDIO_DIR}/miniaudio.h")
    message(STATUS "Downloading miniaudio ${MINIAUDIO_VERSION}...")
    file(DOWNLOAD
      "https://raw.githubusercontent.com/mackron/miniaudio/${MINIAUDIO_VERSION}/miniaudio.h"
      "${MINIAUDIO_DIR}/miniaudio.h"
      STATUS miniaudio_status
      TLS_VERIFY ON)
    list(GET miniaudio_status 0 miniaudio_code)
    list(GET miniaudio_status 1 miniaudio_msg)
    if(NOT miniaudio_code EQUAL 0)
      file(REMOVE "${MINIAUDIO_DIR}/miniaudio.h")
      message(FATAL_ERROR
        "Could not download miniaudio ${MINIAUDIO_VERSION}: ${miniaudio_msg}\n"
        "Download miniaudio.h manually and put it next to CMakeLists.txt.")
    endif()
  endif()
endif()

add_library(miniaudio INTERFACE)
# SYSTEM so the 4 MB header doesn't flood the build with its own warnings.
# TODO: What is SYSTEM???
target_include_directories(miniaudio SYSTEM INTERFACE "${MINIAUDIO_DIR}")

if(APPLE)
  target_link_libraries(miniaudio INTERFACE
    "-framework CoreFoundation"
    "-framework CoreAudio"
    "-framework AudioToolbox")
elseif(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(miniaudio INTERFACE Threads::Threads ${CMAKE_DL_LIBS} m)
endif()

# ---------------------------------------------------------------------------
# Programs
# ---------------------------------------------------------------------------
foreach(tool BluetoothListen WiredListen)
  add_executable(${tool} ${tool}.cpp)
  target_link_libraries(${tool} PRIVATE miniaudio)
  if(MSVC)
    target_compile_options(${tool} PRIVATE /W4)
  else()
    target_compile_options(${tool} PRIVATE -Wall -Wextra)
  endif()
endforeach()