
add_executable(test_target_executable_properties "${CMAKE_CURRENT_SOURCE_DIR}/../pic_main.cpp")
set_target_properties(test_target_executable_properties
  PROPERTIES
    POSITION_INDEPENDENT_CODE True
)

add_library(test_target_shared_library_properties SHARED "${CMAKE_CURRENT_SOURCE_DIR}/../pic_lib.cpp")
set_target_properties(test_target_shared_library_properties
  PROPERTIES
    POSITION_INDEPENDENT_CODE True
    DEFINE_SYMBOL PIC_TEST_BUILD_DLL
)

add_library(test_target_static_library_properties STATIC "${CMAKE_CURRENT_SOURCE_DIR}/../pic_lib.cpp")
set_target_properties(test_target_static_library_properties
  PROPERTIES
    POSITION_INDEPENDENT_CODE True
    COMPILE_DEFINITIONS PIC_TEST_STATIC_BUILD
)
