cmake_minimum_required(VERSION 3.14)

project(XHT LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(HEADER_TOOL_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(HEADER_TOOL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(XHT
    Src/main.cpp
    Src/CmdLine.h
    Src/DataDef.h
    Src/ParserBase.h
    Src/ParserBase.cpp
    Src/FileParser.h
    Src/FileParser.cpp
    Src/SymbolParser.h
    Src/SymbolParser.cpp
    Src/FileGenerator.h
    Src/FileGenerator.cpp
    Src/Symbols.h
    Src/Token.h
    Src/Utils.h
)

add_executable(KeywordsGen
    KeywordsGen/KeywordsGen.cpp
)
target_compile_definitions(KeywordsGen PRIVATE HEADER_TOOL_SOURCE_DIR="${HEADER_TOOL_SOURCE_DIR}")

set_target_properties(KeywordsGen PROPERTIES FOLDER "XHT")
set_target_properties(XHT PROPERTIES FOLDER "XHT")