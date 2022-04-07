add_subdirectory(LuaTest)
add_subdirectory(RttrTest)
add_subdirectory(RttrVSLua)
add_subdirectory(SerializationTest)

set_target_properties(LuaTest PROPERTIES FOLDER "Test")
set_target_properties(RttrTest PROPERTIES FOLDER "Test")
set_target_properties(RttrVSLua PROPERTIES FOLDER "Test")
set_target_properties(SerializationTest PROPERTIES FOLDER "Test")