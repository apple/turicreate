cmake_minimum_required(VERSION 3.2)
cmake_policy(SET CMP0063 NEW)

project(Visibility)

add_library(hidden1 SHARED hidden.c)
set_property(TARGET hidden1 PROPERTY C_VISIBILITY_PRESET hidden)

add_library(hidden_object OBJECT hidden.c)
set_property(TARGET hidden_object PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET hidden_object PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(hidden_static STATIC hidden.c)
set_property(TARGET hidden_static PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET hidden_static PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(hidden2 SHARED $<TARGET_OBJECTS:hidden_object> shared.c)

add_library(hidden3 SHARED shared.c)
target_link_libraries(hidden3 hidden_static)

foreach(t
    hidden1
    hidden2
    hidden3
    )
  add_custom_command(TARGET ${t} POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    -DCMAKE_NM=${CMAKE_NM}
    -DTEST_LIBRARY_PATH=$<TARGET_FILE:${t}>
    -P ${CMAKE_CURRENT_SOURCE_DIR}/verify.cmake
    )
endforeach()


add_library(inlines_hidden1 SHARED foo.cpp bar.c)
set_property(TARGET inlines_hidden1 PROPERTY VISIBILITY_INLINES_HIDDEN ON)
target_compile_options(inlines_hidden1 PRIVATE -Werror)

add_library(inlines_hidden_object OBJECT foo.cpp bar.c)
set_property(TARGET inlines_hidden_object PROPERTY VISIBILITY_INLINES_HIDDEN ON)
set_property(TARGET inlines_hidden_object PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_options(inlines_hidden_object PRIVATE -Werror)

add_library(inlines_hidden_static STATIC foo.cpp bar.c)
set_property(TARGET inlines_hidden_static PROPERTY VISIBILITY_INLINES_HIDDEN ON)
set_property(TARGET inlines_hidden_static PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_options(inlines_hidden_static PRIVATE -Werror)

add_library(inlines_hidden2 SHARED $<TARGET_OBJECTS:inlines_hidden_object> shared.cpp)

add_library(inlines_hidden3 SHARED shared.cpp)
target_link_libraries(inlines_hidden3 inlines_hidden_static)

foreach(t
    inlines_hidden1
    inlines_hidden2
    inlines_hidden3
    )
  add_custom_command(TARGET ${t} POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    -DCMAKE_NM=${CMAKE_NM}
    -DTEST_LIBRARY_PATH=$<TARGET_FILE:${t}>
    -P ${CMAKE_CURRENT_SOURCE_DIR}/verify.cmake
    )
endforeach()
