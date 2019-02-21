cmake_minimum_required(VERSION 3.3)
project(target_compile_features)

set(CMAKE_VERBOSE_MAKEFILE ON)

if (c_restrict IN_LIST CMAKE_C_COMPILE_FEATURES)
  add_executable(c_target_compile_features_specific main.c)
  target_compile_features(c_target_compile_features_specific
    PRIVATE c_restrict
  )

  add_library(c_lib_restrict_specific lib_restrict.c)
  target_compile_features(c_lib_restrict_specific
    PUBLIC c_restrict
  )

  add_executable(c_restrict_user_specific restrict_user.c)
  target_link_libraries(c_restrict_user_specific c_lib_restrict_specific)
endif()

if (c_std_99 IN_LIST CMAKE_C_COMPILE_FEATURES AND
    NOT "${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
  add_executable(c_target_compile_features_meta main.c)
  target_compile_features(c_target_compile_features_meta
    PRIVATE c_std_99
  )

  add_library(c_lib_restrict_meta lib_restrict.c)
  target_compile_features(c_lib_restrict_meta
    PUBLIC c_std_99
  )

  add_executable(c_restrict_user_meta restrict_user.c)
  target_link_libraries(c_restrict_user_meta c_lib_restrict_meta)
endif()

if (cxx_auto_type IN_LIST CMAKE_CXX_COMPILE_FEATURES)
  add_executable(cxx_target_compile_features_specific main.cpp)
  target_compile_features(cxx_target_compile_features_specific
    PRIVATE cxx_auto_type
  )

  add_library(cxx_lib_auto_type_specific lib_auto_type.cpp)
  target_compile_features(cxx_lib_auto_type_specific
    PUBLIC cxx_auto_type
  )

  add_executable(cxx_lib_user_specific lib_user.cpp)
  target_link_libraries(cxx_lib_user_specific cxx_lib_auto_type_specific)
endif()

if (cxx_std_11 IN_LIST CMAKE_CXX_COMPILE_FEATURES)
  add_executable(cxx_target_compile_features_meta main.cpp)
  target_compile_features(cxx_target_compile_features_meta
    PRIVATE cxx_std_11
  )

  add_library(cxx_lib_auto_type_meta lib_auto_type.cpp)
  target_compile_features(cxx_lib_auto_type_meta
    PUBLIC cxx_std_11
  )

  add_executable(cxx_lib_user_meta lib_user.cpp)
  target_link_libraries(cxx_lib_user_meta cxx_lib_auto_type_meta)
endif()
