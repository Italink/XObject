cmake_minimum_required(VERSION 3.14)
project(RttrVSLua LANGUAGES CXX)

add_executable(RttrVSLua
    Test.h 
    Utils.h
    main.cpp
)

add_custom_command(TARGET RttrVSLua POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:RTTR::Core> $<TARGET_FILE_DIR:RttrVSLua>
    COMMAND_EXPAND_LISTS
)

target_xht_warp(RttrVSLua Test.h)

target_link_libraries(RttrVSLua PUBLIC XObject)