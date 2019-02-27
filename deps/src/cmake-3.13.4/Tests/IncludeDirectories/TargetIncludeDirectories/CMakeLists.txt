
cmake_minimum_required(VERSION 2.8)

project(TargetIncludeDirectories)

macro(create_header _name)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${_name}")
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${_name}/${_name}.h" "/* ${_name}.h */\n")
endmacro()

create_header(bar)
create_header(bat)
create_header(foo)
create_header(baz)
create_header(bang)
create_header(bing)
create_header(bung)
create_header(arguments)
create_header(list)
create_header(target)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories("${CMAKE_CURRENT_BINARY_DIR}/bar")
include_directories("$<1:${CMAKE_CURRENT_BINARY_DIR}/bang>")

add_executable(TargetIncludeDirectories main.cpp)

set_property(TARGET TargetIncludeDirectories APPEND PROPERTY INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/bat")
set_property(TARGET TargetIncludeDirectories APPEND PROPERTY INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/foo")
set_property(TARGET TargetIncludeDirectories APPEND PROPERTY
    INCLUDE_DIRECTORIES "$<1:${CMAKE_CURRENT_BINARY_DIR}/bing>")

include_directories("${CMAKE_CURRENT_BINARY_DIR}/baz")
include_directories("$<1:${CMAKE_CURRENT_BINARY_DIR}/bung>")
include_directories("sing$<1:/ting>")

include_directories("$<1:${CMAKE_CURRENT_BINARY_DIR}/arguments;${CMAKE_CURRENT_BINARY_DIR}/list>")

create_header(fee)
create_header(fiy)
create_header(foh)
create_header(fum)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/lib1.cpp" "#include \"fee.h\"\n")
add_library(lib1 "${CMAKE_CURRENT_BINARY_DIR}/lib1.cpp")
set_property(TARGET lib1 APPEND PROPERTY INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/fee")
set_property(TARGET lib1 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/fiy")
set_property(TARGET lib1 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${CMAKE_CURRENT_BINARY_DIR}/foh>")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/lib2.cpp" "#include \"fiy.h\"\n")
add_library(lib2 "${CMAKE_CURRENT_BINARY_DIR}/lib2.cpp")
set_property(TARGET lib2 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/fum;$<TARGET_PROPERTY:lib1,INTERFACE_INCLUDE_DIRECTORIES>")
set_property(TARGET lib2 APPEND PROPERTY INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:lib1,INTERFACE_INCLUDE_DIRECTORIES>")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/main3.cpp" "#include \"fiy.h\"\n#include \"foh.h\"\n#include \"fum.h\"\nint main(int,char**) { return 0; }\n")
add_executable(exe3 "${CMAKE_CURRENT_BINARY_DIR}/main3.cpp")
set_property(TARGET exe3 APPEND PROPERTY INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:lib2,INTERFACE_INCLUDE_DIRECTORIES>")

# Test cycles
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/lib3.cpp" "#include \"fiy.h\"\n#include \"foh.h\"\n")
add_library(lib3 "${CMAKE_CURRENT_BINARY_DIR}/lib3.cpp")
set_property(TARGET lib3 APPEND PROPERTY INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/fiy;$<TARGET_PROPERTY:lib4,INTERFACE_INCLUDE_DIRECTORIES>")
set_property(TARGET lib3 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/fiy;$<TARGET_PROPERTY:lib4,INTERFACE_INCLUDE_DIRECTORIES>")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/lib4.cpp" "#include \"fiy.h\"\n#include \"foh.h\"\n")
add_library(lib4 "${CMAKE_CURRENT_BINARY_DIR}/lib4.cpp")
set_property(TARGET lib4 APPEND PROPERTY INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/foh;$<TARGET_PROPERTY:lib3,INTERFACE_INCLUDE_DIRECTORIES>")
set_property(TARGET lib4 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/foh;$<TARGET_PROPERTY:lib3,INTERFACE_INCLUDE_DIRECTORIES>")

add_library(somelib::withcolons UNKNOWN IMPORTED)
set_property(TARGET somelib::withcolons PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/target")
set_property(TARGET somelib::withcolons PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/target")

set_property(TARGET TargetIncludeDirectories
  APPEND PROPERTY INCLUDE_DIRECTORIES
  "$<TARGET_PROPERTY:somelib::withcolons,INTERFACE_INCLUDE_DIRECTORIES>"
)

add_custom_target(test_custom_target
        "some_bogus_custom_tool"
        $<TARGET_PROPERTY:TargetIncludeDirectories,COMPILE_DEFINITIONS>
        WORKING_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bad")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/bad/common.h" "#error Should not be included\n")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/good")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/good/common.h" "#include \"othergood.h\"\n")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/othergood")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/othergood/othergood.h" "/* No error */\n")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/libothergood.cpp" "// No content \n")
add_library(libothergood "${CMAKE_CURRENT_BINARY_DIR}/libothergood.cpp")
set_property(TARGET libothergood APPEND PROPERTY
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/othergood"
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/libgood.cpp" "// No content \n")
add_library(libgood "${CMAKE_CURRENT_BINARY_DIR}/libgood.cpp")
set_property(TARGET libgood APPEND PROPERTY
  INTERFACE_INCLUDE_DIRECTORIES
    "${CMAKE_CURRENT_BINARY_DIR}/good;$<TARGET_PROPERTY:libothergood,INTERFACE_INCLUDE_DIRECTORIES>"
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/libbad.cpp" "// No content \n")
add_library(libbad "${CMAKE_CURRENT_BINARY_DIR}/libbad.cpp")
set_property(TARGET libbad APPEND PROPERTY
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/bad"
)

add_library(lib6 SHARED other.cpp)
add_library(lib7 SHARED other.cpp)
target_link_libraries(lib7 $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:lib6>)
target_include_directories(lib7 PUBLIC $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:/lib7/include>)
add_custom_target(head_target_test "some_bogus_custom_tool \"$<TARGET_PROPERTY:lib7,INCLUDE_DIRECTORIES>$<TARGET_PROPERTY:lib7,INTERFACE_INCLUDE_DIRECTORIES>\"")

add_library(includes_source_good empty.cpp)
target_include_directories(includes_source_good
  INTERFACE
    "${CMAKE_CURRENT_BINARY_DIR}/good"
    "${CMAKE_CURRENT_BINARY_DIR}/othergood/"
)
add_library(includes_source_bad empty.cpp)
target_include_directories(includes_source_bad
  INTERFACE
    "${CMAKE_CURRENT_BINARY_DIR}/bad"
)

add_library(includes_proxy SHARED empty.cpp)
target_link_libraries(includes_proxy includes_source_good)
target_link_libraries(includes_proxy LINK_INTERFACE_LIBRARIES includes_source_bad)
add_executable(copy_includes copy_includes.cpp)
target_include_directories(copy_includes PRIVATE $<TARGET_PROPERTY:includes_proxy,INCLUDE_DIRECTORIES>)


file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/lib5.cpp" "#include \"common.h\"\n")
add_library(lib5 "${CMAKE_CURRENT_BINARY_DIR}/lib5.cpp")

# Assuming the link order must be:
target_link_libraries(lib5 libbad libgood)

# Oops!.
# As include directory order and link order are the same when using target_link_libraries, we have to
# get the libgood includes in before the libbad includes.
# We do that with this command:
target_include_directories(lib5
  BEFORE PRIVATE $<TARGET_PROPERTY:libgood,INTERFACE_INCLUDE_DIRECTORIES>
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/prefix_foo/prefix_bar/prefix_bat")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/prefix_foo/prefix_bar/prefix_bat/prefix_foo_bar_bat.h" "/* prefix_foo_bar_bat.h */\n")

target_include_directories(TargetIncludeDirectories PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/prefix_$<JOIN:foo;bar;bat,/prefix_>")

# Test that the language generator expressions work
set_property(TARGET TargetIncludeDirectories
  APPEND PROPERTY INCLUDE_DIRECTORIES
  "$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,C>:${CMAKE_CURRENT_BINARY_DIR}/bad>"
  "$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,CXX>:${CMAKE_CURRENT_BINARY_DIR}/good>"
  "$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,CXX>:${CMAKE_CURRENT_BINARY_DIR}/othergood/>"
)

add_executable(TargetIncludeDirectories_C main.c)
set_property(TARGET TargetIncludeDirectories_C
  APPEND PROPERTY INCLUDE_DIRECTORIES
  "$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,CXX>:${CMAKE_CURRENT_BINARY_DIR}/bad>"
  "$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,C>:${CMAKE_CURRENT_BINARY_DIR}/good>"
  "$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,C>:${CMAKE_CURRENT_BINARY_DIR}/othergood/>"
)
