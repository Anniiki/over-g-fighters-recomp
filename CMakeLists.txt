# overg - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(overg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(OVERG_SOURCES
    src/gamepad_keyboard.cpp
    src/language.cpp
    src/main.cpp
)

if(WIN32)
    add_executable(overg WIN32 ${OVERG_SOURCES})
else()
    add_executable(overg ${OVERG_SOURCES})
endif()

rexglue_setup_target(overg GPU_PLUGINS xenos)
