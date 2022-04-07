cmake_minimum_required(VERSION 3.14)
project(SerializationTest LANGUAGES CXX)

add_executable(SerializationTest 
    Test.h 
    main.cpp
)

add_custom_command(TARGET SerializationTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:RTTR::Core> $<TARGET_FILE_DIR:SerializationTest>
    COMMAND_EXPAND_LISTS
)

target_xht_warp(SerializationTest Test.h)

target_link_libraries(SerializationTest PUBLIC XObject)
