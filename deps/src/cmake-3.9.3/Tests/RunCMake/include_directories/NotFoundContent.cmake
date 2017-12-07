
include_directories(NotThere1-NOTFOUND)

include_directories($<1:There1-NOTFOUND>)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp" "int main(int,char**) { return 0; }\n")
add_executable(dummy "${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp")
set_property(TARGET dummy APPEND PROPERTY INCLUDE_DIRECTORIES "NotThere2-NOTFOUND")
set_property(TARGET dummy APPEND PROPERTY INCLUDE_DIRECTORIES "$<1:There2-NOTFOUND>")
