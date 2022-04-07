cmake_minimum_required(VERSION 3.14)
project(RttrTest LANGUAGES CXX)

add_executable(RttrTest 
    Test.h 
    main.cpp
)

add_custom_command(TARGET RttrTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:RTTR::Core> $<TARGET_FILE_DIR:RttrTest>
    COMMAND_EXPAND_LISTS
)

target_xht_warp(RttrTest Test.h)

target_link_libraries(RttrTest PUBLIC XObject)
