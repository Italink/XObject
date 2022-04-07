cmake_minimum_required(VERSION 3.14)

project(KeywordsGen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(KeywordsGen
    KeywordsGen.cpp
)

target_compile_definitions(KeywordsGen PRIVATE HEADER_TOOL_SOURCE_DIR="${HEADER_TOOL_SOURCE_DIR}")
