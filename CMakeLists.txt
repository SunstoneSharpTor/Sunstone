cmake_minimum_required(VERSION 3.27.4)
project(Sunstone VERSION 1.15.0 LANGUAGES CXX)

set(OSBitness 64)
set(FullOutputDir "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${FullOutputDir}/static_libs")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${FullOutputDir}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${FullOutputDir}")

include_directories(./src)

set(SOURCE_FILES
    src/board.cpp
    src/engine.cpp
    src/main.cpp
    src/search.cpp
    src/transpositionTable.cpp)

add_executable(sunstone ${SOURCE_FILES})


if (MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif(MINGW)