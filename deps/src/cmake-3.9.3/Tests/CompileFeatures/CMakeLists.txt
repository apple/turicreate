
cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0057 NEW)

project(CompileFeatures)

macro(run_test feature lang)
  if (${feature} IN_LIST CMAKE_${lang}_COMPILE_FEATURES)
    add_library(test_${feature} OBJECT ${feature})
    set_property(TARGET test_${feature}
      PROPERTY COMPILE_FEATURES "${feature}"
    )
  else()
    list(APPEND ${lang}_non_features ${feature})
  endif()
endmacro()

get_property(c_features GLOBAL PROPERTY CMAKE_C_KNOWN_FEATURES)
list(FILTER c_features EXCLUDE REGEX "^c_std_[0-9][0-9]")
foreach(feature ${c_features})
  run_test(${feature} C)
endforeach()

get_property(cxx_features GLOBAL PROPERTY CMAKE_CXX_KNOWN_FEATURES)
list(FILTER cxx_features EXCLUDE REGEX "^cxx_std_[0-9][0-9]")
foreach(feature ${cxx_features})
  run_test(${feature} CXX)
endforeach()

if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"
    AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.1)
  # AppleClang prior to 5.1 does not set any preprocessor define to distinguish
  # c++1y from c++11, so CMake does not support c++1y features before AppleClang 5.1.
  list(REMOVE_ITEM CXX_non_features
    cxx_attribute_deprecated
    cxx_binary_literals
  )
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"
    AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.2)
  # AppleClang prior to 4.1 reports false for __has_feature(cxx_local_type_template_args)
  # and __has_feature(cxx_unrestricted_unions) but it happens to pass these tests.
  list(REMOVE_ITEM CXX_non_features
    cxx_local_type_template_args
    cxx_unrestricted_unions
  )
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL SunPro)
  list(REMOVE_ITEM CXX_non_features
    cxx_attribute_deprecated
    cxx_contextual_conversions
    cxx_extended_friend_declarations
    cxx_long_long_type
    cxx_sizeof_member
    cxx_variadic_macros
  )
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
    AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.5)
  # The cxx_raw_string_literals feature happens to work in some distributions
  # of GNU 4.4, but it is first documented as available with GNU 4.5.
  list(REMOVE_ITEM CXX_non_features
    cxx_raw_string_literals
  )
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
    AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.6)
  # The cxx_constexpr feature happens to work (for *this* testcase) with
  # GNU 4.5, but it is first documented as available with GNU 4.6.
  list(REMOVE_ITEM CXX_non_features
    cxx_constexpr
  )
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
    AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
  # The cxx_alignof feature happens to work (for *this* testcase) with
  # GNU 4.7, but it is first documented as available with GNU 4.8.
  list(REMOVE_ITEM CXX_non_features
    cxx_alignof
  )
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
    AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
  # GNU prior to 4.9 does not set any preprocessor define to distinguish
  # c++1y from c++11, so CMake does not support c++1y features before GNU 4.9.
  list(REMOVE_ITEM CXX_non_features
    # GNU 4.8 knows cxx_attributes, but doesn't know [[deprecated]]
    # and warns that it is unknown and ignored.
    cxx_attribute_deprecated
    cxx_binary_literals
    cxx_lambda_init_captures
    cxx_return_type_deduction
  )
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 18.0)
    list(REMOVE_ITEM CXX_non_features
      # The cxx_contextual_conversions feature happens to work
      # (for *this* testcase) with VS 2010 and VS 2012, but
      # they do not document support until VS 2013.
      cxx_contextual_conversions
      )
  elseif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0)
    list(REMOVE_ITEM CXX_non_features
      # The cxx_deleted_functions and cxx_nonstatic_member_init
      # features happen to work (for *this* testcase) with VS 2013,
      # but they do not document support until VS 2015.
      cxx_deleted_functions
      cxx_nonstatic_member_init
      )
  endif()
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16.0)
    if (CMAKE_CXX_COMIPLER_VERSION VERSION_EQUAL 15.0)
      list(REMOVE_ITEM CXX_non_features
        # The cxx_contextual_conversions feature happens to work
        # (for *this* testcase) with Intel 13/14/15, but they do not
        # document support until 16.
        cxx_contextual_conversions
        )

    elseif (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14.0)
      list(REMOVE_ITEM CXX_non_features
        cxx_alignof

        # not supposed to work until 15
        cxx_attribute_deprecated

        # The cxx_contextual_conversions feature happens to work
        # (for *this* testcase) with Intel 13/14/15, but they do not
        # document support until 16.
        cxx_contextual_conversions
        )

    elseif (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 12.1)
      list(REMOVE_ITEM CXX_non_features
        # These features happen to work but aren't documented to
        # do so until 14.0
        cxx_constexpr
        cxx_enum_forward_declarations
        cxx_sizeof_member
        cxx_strong_enums
        cxx_unicode_literals

        # not supposed to work until 15
        cxx_attribute_deprecated
        cxx_nonstatic_member_init

        # The cxx_contextual_conversions feature happens to work
        # (for *this* testcase) with Intel 13/14/15, but they do not
        # document support until 16.
        cxx_contextual_conversions

        # This is an undocumented feature; it does not work in future versions
        cxx_aggregate_default_initializers
        )
    endif()
  endif()
endif()

if (CMAKE_C_COMPILER_ID STREQUAL "Intel")
  if (CMAKE_C_COMPILER_VERSION VERSION_LESS 15.0.2)
    # This works on some pre-15.0.2 versions and not others.
    list(REMOVE_ITEM C_non_features
      c_static_assert
      )
  endif()
endif()

if (CMAKE_C_COMPILE_FEATURES)
  set(C_expected_features ${CMAKE_C_COMPILE_FEATURES})
  list(FILTER C_expected_features EXCLUDE REGEX "^c_std_[0-9][0-9]")
endif()
if (CMAKE_CXX_COMPILE_FEATURES)
  set(CXX_expected_features ${CMAKE_CXX_COMPILE_FEATURES})
  list(FILTER CXX_expected_features EXCLUDE REGEX "^cxx_std_[0-9][0-9]")
endif ()
set(C_ext c)
set(C_standard_flag 11)
set(CXX_ext cpp)
set(CXX_standard_flag 14)
foreach(lang CXX C)
  if (${lang}_expected_features)
    foreach(feature ${${lang}_non_features})
      message("Testing feature : ${feature}")
      try_compile(${feature}_works
        "${CMAKE_CURRENT_BINARY_DIR}/${feature}_test"
        "${CMAKE_CURRENT_SOURCE_DIR}/feature_test.${${lang}_ext}"
        COMPILE_DEFINITIONS "-DTEST=${feature}.${${lang}_ext}"
        CMAKE_FLAGS "-DCMAKE_${lang}_STANDARD=${${lang}_standard_flag}"
          "-DINCLUDE_DIRECTORIES=${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE OUTPUT
      )
      if (${feature}_works)
        message(SEND_ERROR
          "Feature ${feature} expected not to work for ${lang} ${CMAKE_${lang}_COMPILER_ID}-${CMAKE_${lang}_COMPILER_VERSION}.
  Update the supported features or blacklist it.\n${OUTPUT}")
      else()
        message("Testing feature : ${feature} -- Fails, as expected.")
      endif()
    endforeach()
  endif()
endforeach()

if (C_expected_features)
  if (CMAKE_C_STANDARD_DEFAULT)
    string(FIND "${CMAKE_C_FLAGS}" "-std=" std_flag_idx)
    if (std_flag_idx EQUAL -1)
      add_executable(default_dialect_C default_dialect.c)
      target_compile_definitions(default_dialect_C PRIVATE
        DEFAULT_C11=$<EQUAL:${CMAKE_C_STANDARD_DEFAULT},11>
        DEFAULT_C99=$<EQUAL:${CMAKE_C_STANDARD_DEFAULT},99>
        DEFAULT_C90=$<EQUAL:${CMAKE_C_STANDARD_DEFAULT},90>
      )
    endif()
  endif()

  add_executable(CompileFeaturesGenex_C genex_test.c)
  set_property(TARGET CompileFeaturesGenex_C PROPERTY C_STANDARD 11)

  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.6)
      list(APPEND expected_defs
        EXPECT_C_STATIC_ASSERT=1
      )
    else()
      list(APPEND expected_defs
        EXPECT_C_STATIC_ASSERT=0
      )
    endif()
  elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang"
      OR CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
    list(APPEND expected_defs
      EXPECT_C_STATIC_ASSERT=1
    )
  elseif (CMAKE_C_COMPILER_ID STREQUAL "Intel")
    if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 15)
      list(APPEND expected_defs
        EXPECT_C_STATIC_ASSERT=1
        )
    else()
      list(APPEND expected_defs
        EXPECT_C_STATIC_ASSERT=0
        )
    endif()
  elseif (CMAKE_C_COMPILER_ID STREQUAL "SunPro")
    if (NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 5.13)
      list(APPEND expected_defs
        EXPECT_C_STATIC_ASSERT=1
        )
    else()
      list(APPEND expected_defs
        EXPECT_C_STATIC_ASSERT=0
        )
    endif()
  endif()

  list(APPEND expected_defs
    EXPECT_C_FUNCTION_PROTOTYPES=1
    EXPECT_C_RESTRICT=1
  )

  target_compile_definitions(CompileFeaturesGenex_C PRIVATE
    HAVE_C_FUNCTION_PROTOTYPES=$<COMPILE_FEATURES:c_function_prototypes>
    HAVE_C_RESTRICT=$<COMPILE_FEATURES:c_restrict>
    HAVE_C_STATIC_ASSERT=$<COMPILE_FEATURES:c_static_assert>
    ${expected_defs}
  )
endif()

if (CMAKE_CXX_COMPILE_FEATURES)
  if (CMAKE_CXX_STANDARD_DEFAULT)
    string(FIND "${CMAKE_CXX_FLAGS}" "-std=" std_flag_idx)
    if (std_flag_idx EQUAL -1)
      add_executable(default_dialect default_dialect.cpp)
      target_compile_definitions(default_dialect PRIVATE
        DEFAULT_CXX17=$<EQUAL:${CMAKE_CXX_STANDARD_DEFAULT},17>
        DEFAULT_CXX14=$<EQUAL:${CMAKE_CXX_STANDARD_DEFAULT},14>
        DEFAULT_CXX11=$<EQUAL:${CMAKE_CXX_STANDARD_DEFAULT},11>
        DEFAULT_CXX98=$<EQUAL:${CMAKE_CXX_STANDARD_DEFAULT},98>
      )
    endif()
  endif()
endif ()

# always add a target "CompileFeatures"
if ((NOT CXX_expected_features) OR
    (NOT cxx_auto_type IN_LIST CXX_expected_features))
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp"
    "int main(int,char**) { return 0; }\n"
  )
  add_executable(CompileFeatures "${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp")
else()
  # these tests only work if at least cxx_auto_type is available
  add_executable(CompileFeatures main.cpp)
  set_property(TARGET CompileFeatures
    PROPERTY COMPILE_FEATURES "cxx_auto_type"
  )
  set_property(TARGET CompileFeatures
    PROPERTY CXX_STANDARD_REQUIRED TRUE
  )

  add_executable(GenexCompileFeatures main.cpp)
  set_property(TARGET GenexCompileFeatures
    PROPERTY COMPILE_FEATURES "$<1:cxx_auto_type>;$<0:not_a_feature>"
  )

  add_library(iface INTERFACE)
  set_property(TARGET iface
    PROPERTY INTERFACE_COMPILE_FEATURES "cxx_auto_type"
  )
  add_executable(IfaceCompileFeatures main.cpp)
  target_link_libraries(IfaceCompileFeatures iface)

  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
      add_definitions(
        -DEXPECT_OVERRIDE_CONTROL=1
        -DEXPECT_INHERITING_CONSTRUCTORS=1
        -DEXPECT_FINAL=1
        -DEXPECT_INHERITING_CONSTRUCTORS_AND_FINAL=1
      )
    elseif (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7)
      add_definitions(
        -DEXPECT_OVERRIDE_CONTROL=1
        -DEXPECT_INHERITING_CONSTRUCTORS=0
        -DEXPECT_FINAL=1
        -DEXPECT_INHERITING_CONSTRUCTORS_AND_FINAL=0
      )
    else()
      add_definitions(
        -DEXPECT_OVERRIDE_CONTROL=0
        -DEXPECT_INHERITING_CONSTRUCTORS=0
        -DEXPECT_FINAL=0
        -DEXPECT_INHERITING_CONSTRUCTORS_AND_FINAL=0
      )
    endif()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_definitions(
      -DEXPECT_OVERRIDE_CONTROL=1
      -DEXPECT_INHERITING_CONSTRUCTORS=1
      -DEXPECT_FINAL=1
      -DEXPECT_INHERITING_CONSTRUCTORS_AND_FINAL=1
    )
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
      add_definitions(
        -DEXPECT_OVERRIDE_CONTROL=1
        -DEXPECT_INHERITING_CONSTRUCTORS=1
        -DEXPECT_FINAL=1
        -DEXPECT_INHERITING_CONSTRUCTORS_AND_FINAL=1
      )
    else()
      add_definitions(
        -DEXPECT_OVERRIDE_CONTROL=1
        -DEXPECT_INHERITING_CONSTRUCTORS=0
        -DEXPECT_FINAL=1
        -DEXPECT_INHERITING_CONSTRUCTORS_AND_FINAL=0
      )
    endif()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0)
      add_definitions(
        -DEXPECT_OVERRIDE_CONTROL=1
        -DEXPECT_INHERITING_CONSTRUCTORS=1
        -DEXPECT_FINAL=1
        -DEXPECT_INHERITING_CONSTRUCTORS_AND_FINAL=1
      )
    elseif(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17.0)
      add_definitions(
        -DEXPECT_OVERRIDE_CONTROL=1
        -DEXPECT_INHERITING_CONSTRUCTORS=0
        -DEXPECT_FINAL=1
        -DEXPECT_INHERITING_CONSTRUCTORS_AND_FINAL=0
      )
    else()
      add_definitions(
        -DEXPECT_OVERRIDE_CONTROL=0
        -DEXPECT_INHERITING_CONSTRUCTORS=0
        -DEXPECT_FINAL=0
        -DEXPECT_INHERITING_CONSTRUCTORS_AND_FINAL=0
      )
    endif()
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "SunPro")
    add_definitions(
      -DEXPECT_OVERRIDE_CONTROL=1
      -DEXPECT_INHERITING_CONSTRUCTORS=1
      -DEXPECT_FINAL=1
      -DEXPECT_INHERITING_CONSTRUCTORS_AND_FINAL=1
    )
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 15)
      add_definitions(
        -DEXPECT_OVERRIDE_CONTROL=1
        -DEXPECT_INHERITING_CONSTRUCTORS=1
        -DEXPECT_FINAL=1
        -DEXPECT_INHERITING_CONSTRUCTORS_AND_FINAL=1
      )
    elseif (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14)
      add_definitions(
        -DEXPECT_OVERRIDE_CONTROL=1
        -DEXPECT_INHERITING_CONSTRUCTORS=0
        -DEXPECT_FINAL=1
        -DEXPECT_INHERITING_CONSTRUCTORS_AND_FINAL=0
      )
    else()
      add_definitions(
        -DEXPECT_OVERRIDE_CONTROL=0
        -DEXPECT_INHERITING_CONSTRUCTORS=0
        -DEXPECT_FINAL=0
        -DEXPECT_INHERITING_CONSTRUCTORS_AND_FINAL=0
      )
    endif()
  endif()

  add_executable(CompileFeaturesGenex genex_test.cpp)
  set_property(TARGET CompileFeaturesGenex PROPERTY CXX_STANDARD 11)
  target_compile_definitions(CompileFeaturesGenex PRIVATE
    HAVE_OVERRIDE_CONTROL=$<COMPILE_FEATURES:cxx_final,cxx_override>
    HAVE_AUTO_TYPE=$<COMPILE_FEATURES:cxx_auto_type>
    HAVE_INHERITING_CONSTRUCTORS=$<COMPILE_FEATURES:cxx_inheriting_constructors>
    HAVE_FINAL=$<COMPILE_FEATURES:cxx_final>
    HAVE_INHERITING_CONSTRUCTORS_AND_FINAL=$<COMPILE_FEATURES:cxx_inheriting_constructors,cxx_final>
  )

  add_executable(CompileFeaturesGenex2 genex_test.cpp)
  target_compile_features(CompileFeaturesGenex2 PRIVATE cxx_std_11)
  target_compile_definitions(CompileFeaturesGenex2 PRIVATE
    HAVE_OVERRIDE_CONTROL=$<COMPILE_FEATURES:cxx_final,cxx_override>
    HAVE_AUTO_TYPE=$<COMPILE_FEATURES:cxx_auto_type>
    HAVE_INHERITING_CONSTRUCTORS=$<COMPILE_FEATURES:cxx_inheriting_constructors>
    HAVE_FINAL=$<COMPILE_FEATURES:cxx_final>
    HAVE_INHERITING_CONSTRUCTORS_AND_FINAL=$<COMPILE_FEATURES:cxx_inheriting_constructors,cxx_final>
  )

  add_library(std_11_iface INTERFACE)
  target_compile_features(std_11_iface INTERFACE cxx_std_11)
  add_executable(CompileFeaturesGenex3 genex_test.cpp)
  target_link_libraries(CompileFeaturesGenex3 PRIVATE std_11_iface)
  target_compile_definitions(CompileFeaturesGenex3 PRIVATE
    HAVE_OVERRIDE_CONTROL=$<COMPILE_FEATURES:cxx_final,cxx_override>
    HAVE_AUTO_TYPE=$<COMPILE_FEATURES:cxx_auto_type>
    HAVE_INHERITING_CONSTRUCTORS=$<COMPILE_FEATURES:cxx_inheriting_constructors>
    HAVE_FINAL=$<COMPILE_FEATURES:cxx_final>
    HAVE_INHERITING_CONSTRUCTORS_AND_FINAL=$<COMPILE_FEATURES:cxx_inheriting_constructors,cxx_final>
  )
endif()
