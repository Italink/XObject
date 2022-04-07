cmake_minimum_required(VERSION 3.14)
project(XObject LANGUAGES CXX)


file(GLOB_RECURSE PROJECT_SOURCES FILES *.h *.cpp *.hpp)

add_library(XObject ${PROJECT_SOURCES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PROJECT_SOURCES})

set_target_properties(XObject PROPERTIES FOLDER "XObject")

target_link_libraries(XObject PUBLIC RTTR::Core Lua sol2)

target_include_directories(XObject PUBLIC .)
target_include_directories(XObject PUBLIC Serialization)

function(target_xht_warp PROJECT_TARGET INPUT_FILE_PATH)            
    get_filename_component(INPUT_FILE_NAME ${INPUT_FILE_PATH} NAME_WE)                           #获取不带扩展名的文件名
    set(OUTPUT_FILE_PATH ${CMAKE_CURRENT_BINARY_DIR}/AutoGenFiles/XHT_${INPUT_FILE_NAME}.cpp)   
    add_custom_command(
        OUTPUT ${OUTPUT_FILE_PATH}                                                               #指定输出文件
        COMMAND XHT ${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE_PATH} -o ${OUTPUT_FILE_PATH}      #命令行指令
        MAIN_DEPENDENCY ${INPUT_FILE_PATH}                                                       #指定依赖，当该文件变动时，自动调用该指令
    )          
    set_property(TARGET ${PROJECT_TARGET} APPEND PROPERTY SOURCES ${OUTPUT_FILE_PATH})                     #添加到构建目标中
    source_group("Generated Files" FILES ${OUTPUT_FILE_PATH})                                    #文件分组
endfunction()

function(target_xht_auto PROJECT_TARGET)
    get_target_property(TARGET_SOURCES ${PROJECT_TARGET} SOURCES)
    message(WARNING "FILES ${TARGET_SOURCES}")
    foreach(FILE ${TARGET_SOURCES})
        get_filename_component(FILE_EXT ${FILE} EXT)     
        if(FILE_EXT STREQUAL ".hxx")
            target_xht_warp(${PROJECT_TARGET} ${FILE})
        endif()
    endforeach()
endfunction()
