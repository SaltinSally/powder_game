cmake_minimum_required(VERSION 3.20)
project(PowderGameCpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(PowderGame WIN32
    src/main.cpp
    src/sim.cpp
    src/sim.hpp
)

# Windows-specific definitions
if (WIN32)
  target_compile_definitions(PowderGame PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
endif()

# Optimize by default for Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
