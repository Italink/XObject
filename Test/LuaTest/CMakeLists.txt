cmake_minimum_required(VERSION 3.14)
project(LuaTest LANGUAGES CXX)

add_executable(LuaTest 
    Test.h 
    main.cpp
)
add_custom_command(TARGET LuaTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:RTTR::Core> $<TARGET_FILE_DIR:LuaTest>
    COMMAND_EXPAND_LISTS
)
target_xht_warp(LuaTest Test.h)

target_link_libraries(LuaTest PUBLIC XObject)

