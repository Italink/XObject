project(Core LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(XHT)
add_subdirectory(XObject)
add_subdirectory(Test)
add_subdirectory(3rdParty)
