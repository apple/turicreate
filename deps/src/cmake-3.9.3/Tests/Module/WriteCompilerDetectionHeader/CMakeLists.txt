cmake_minimum_required(VERSION 3.1.0)
project(WriteCompilerDetectionHeader)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(WriteCompilerDetectionHeader)
include(CheckCXXSourceCompiles)

get_property(cxx_known_features GLOBAL PROPERTY CMAKE_CXX_KNOWN_FEATURES)
get_property(c_known_features GLOBAL PROPERTY CMAKE_C_KNOWN_FEATURES)

write_compiler_detection_header(
  FILE "${CMAKE_CURRENT_BINARY_DIR}/test_compiler_detection.h"
  PREFIX TEST
  COMPILERS GNU Clang AppleClang MSVC SunPro Intel
  VERSION 3.1
  PROLOG "// something"
  EPILOG "// more"
  FEATURES
    ${cxx_known_features} ${c_known_features}
)

write_compiler_detection_header(
  FILE "${CMAKE_CURRENT_BINARY_DIR}/compiler_multi_files/multi_file_compiler_detection.h"
  PREFIX MULTI
  OUTPUT_FILES_VAR multi_files
  OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/compiler_multi_files/compiler_support"
  COMPILERS GNU Clang AppleClang MSVC SunPro Intel
  VERSION 3.1
  FEATURES
    ${cxx_known_features} ${c_known_features}
)

macro(set_defines target true_defs false_defs)
  set(defines)
  foreach(def ${true_defs})
    list(APPEND defines ${def}=1)
  endforeach()
  foreach(def ${false_defs})
    list(APPEND defines ${def}=0)
  endforeach()
  target_compile_definitions(${target}
    PRIVATE
      ${defines}
      EXPECTED_COMPILER_VERSION_MAJOR=${COMPILER_VERSION_MAJOR}
      EXPECTED_COMPILER_VERSION_MINOR=${COMPILER_VERSION_MINOR}
      EXPECTED_COMPILER_VERSION_PATCH=${COMPILER_VERSION_PATCH}
  )
endmacro()

# Only run the compiler detection header test for compilers with
# detailed features tables, not just meta-features

if (CMAKE_C_COMPILE_FEATURES)
  set(C_expected_features ${CMAKE_C_COMPILE_FEATURES})
  list(FILTER C_expected_features EXCLUDE REGEX "^c_std_[0-9][0-9]")
endif()
if (C_expected_features)
  string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+.*" "\\1" COMPILER_VERSION_MAJOR "${CMAKE_C_COMPILER_VERSION}")
  string(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+.*" "\\1" COMPILER_VERSION_MINOR "${CMAKE_C_COMPILER_VERSION}")
  string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" COMPILER_VERSION_PATCH "${CMAKE_C_COMPILER_VERSION}")

  if (CMAKE_C_COMPILER_ID STREQUAL "GNU"
      OR CMAKE_C_COMPILER_ID STREQUAL "Clang"
      OR CMAKE_C_COMPILER_ID STREQUAL "AppleClang"
      OR CMAKE_C_COMPILER_ID STREQUAL "Intel")
    add_executable(WriteCompilerDetectionHeader_C11 main.c)
    set_property(TARGET WriteCompilerDetectionHeader_C11 PROPERTY C_STANDARD 11)
    set_defines(WriteCompilerDetectionHeader_C11 "EXPECTED_COMPILER_C_FUNCTION_PROTOTYPES;EXPECTED_COMPILER_C_RESTRICT" "")

    add_executable(WriteCompilerDetectionHeader_C11_multi main_multi.c)
    set_property(TARGET WriteCompilerDetectionHeader_C11_multi PROPERTY C_STANDARD 11)
    set_defines(WriteCompilerDetectionHeader_C11_multi "EXPECTED_COMPILER_C_FUNCTION_PROTOTYPES;EXPECTED_COMPILER_C_RESTRICT" "")
    target_include_directories(WriteCompilerDetectionHeader_C11_multi PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/compiler_multi_files)

    add_executable(C_undefined c_undefined.c)
    set_property(TARGET C_undefined PROPERTY C_STANDARD 90)
    include(CheckCCompilerFlag)
    check_c_compiler_flag(-Werror=undef use_error_undef)
    if (use_error_undef)
        target_compile_options(C_undefined PRIVATE -Werror=undef)
    endif()

    add_executable(WriteCompilerDetectionHeader_C main.c)
    set_property(TARGET WriteCompilerDetectionHeader_C PROPERTY C_STANDARD 90)
    set_defines(WriteCompilerDetectionHeader_C "EXPECTED_COMPILER_C_FUNCTION_PROTOTYPES" "EXPECTED_COMPILER_C_RESTRICT")

    add_executable(WriteCompilerDetectionHeader_C_multi main_multi.c)
    set_property(TARGET WriteCompilerDetectionHeader_C_multi PROPERTY C_STANDARD 90)
    set_defines(WriteCompilerDetectionHeader_C_multi "EXPECTED_COMPILER_C_FUNCTION_PROTOTYPES" "EXPECTED_COMPILER_C_RESTRICT")
    target_include_directories(WriteCompilerDetectionHeader_C_multi PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/compiler_multi_files)
  endif()
endif()

if (CMAKE_CXX_COMPILE_FEATURES)
  set(CXX_expected_features ${CMAKE_CXX_COMPILE_FEATURES})
  list(FILTER CXX_expected_features EXCLUDE REGEX "^cxx_std_[0-9][0-9]")
endif()
if (NOT CXX_expected_features)
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp"
    "int main(int,char**) { return 0; }\n"
  )
  add_executable(WriteCompilerDetectionHeader "${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp")

  if(UNIX OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    check_cxx_source_compiles("#include \"${CMAKE_CURRENT_BINARY_DIR}/test_compiler_detection.h\"\nint main() { return 0; }\n"
      file_include_works
    )
    if (file_include_works)
      message(SEND_ERROR "Inclusion of ${CMAKE_CURRENT_BINARY_DIR}/test_compiler_detection.h was expected to cause an error, but did not.")
    endif()
  endif()
  return()
endif()

string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+.*" "\\1" COMPILER_VERSION_MAJOR "${CMAKE_CXX_COMPILER_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+.*" "\\1" COMPILER_VERSION_MINOR "${CMAKE_CXX_COMPILER_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" COMPILER_VERSION_PATCH "${CMAKE_CXX_COMPILER_VERSION}")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
    OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
    OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"
    OR CMAKE_CXX_COMPILER_ID STREQUAL "SunPro"
    OR CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # False for C++98 mode.
  list(APPEND false_defs EXPECTED_COMPILER_CXX_DELEGATING_CONSTRUCTORS)
  list(APPEND false_defs EXPECTED_COMPILER_CXX_VARIADIC_TEMPLATES)
endif()

# for msvc the compiler version determines which c++11 features are available.
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  if(";${CMAKE_CXX_COMPILE_FEATURES};" MATCHES ";cxx_delegating_constructors;")
    list(APPEND true_defs EXPECTED_COMPILER_CXX_DELEGATING_CONSTRUCTORS)
    list(APPEND true_defs EXPECTED_COMPILER_CXX_VARIADIC_TEMPLATES)
  else()
    list(APPEND false_defs EXPECTED_COMPILER_CXX_DELEGATING_CONSTRUCTORS)
    list(APPEND false_defs EXPECTED_COMPILER_CXX_VARIADIC_TEMPLATES)
  endif()
endif()

add_executable(WriteCompilerDetectionHeader main.cpp)
set_property(TARGET WriteCompilerDetectionHeader PROPERTY CXX_STANDARD 98)
set_defines(WriteCompilerDetectionHeader "${true_defs}" "${false_defs}")

add_executable(multi_files multi_files.cpp)
set_property(TARGET multi_files PROPERTY CXX_STANDARD 98)
target_include_directories(multi_files PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/compiler_multi_files)
set_defines(multi_files "${true_defs}" "${false_defs}")

if(MSVC)
  return() # MSVC has only one mode.
endif()

# Since GNU 4.7
if (";${CMAKE_CXX_COMPILE_FEATURES};" MATCHES ";cxx_delegating_constructors;")
  list(APPEND true_defs EXPECTED_COMPILER_CXX_DELEGATING_CONSTRUCTORS)
  list(REMOVE_ITEM false_defs EXPECTED_COMPILER_CXX_DELEGATING_CONSTRUCTORS)
endif()

# Since GNU 4.4
if (";${CMAKE_CXX_COMPILE_FEATURES};" MATCHES ";cxx_variadic_templates;")
  list(APPEND true_defs EXPECTED_COMPILER_CXX_VARIADIC_TEMPLATES)
  list(REMOVE_ITEM false_defs EXPECTED_COMPILER_CXX_VARIADIC_TEMPLATES)
endif()

add_executable(WriteCompilerDetectionHeader_11 main.cpp)
set_property(TARGET WriteCompilerDetectionHeader_11 PROPERTY CXX_STANDARD 11)
set_defines(WriteCompilerDetectionHeader_11 "${true_defs}" "${false_defs}")

add_executable(multi_files_11 multi_files.cpp)
set_property(TARGET multi_files_11 PROPERTY CXX_STANDARD 11)
target_include_directories(multi_files_11 PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/compiler_multi_files)
set_defines(multi_files_11 "${true_defs}" "${false_defs}")

# test for ALLOW_UNKNOWN_COMPILERS

# use a compiler does not match the current one,
# so one always hits the fallback code
if (CMAKE_CXX_COMPILER_ID STREQUAL "SunPro")
  set(OTHER_CXX "Intel")
else()
  set(OTHER_CXX "SunPro")
endif()

write_compiler_detection_header(
  FILE "${CMAKE_CURRENT_BINARY_DIR}/test_compiler_detection_allow_unknown.h"
  PREFIX TEST
  COMPILERS ${OTHER_CXX}
  FEATURES cxx_nullptr
  ALLOW_UNKNOWN_COMPILERS
)

# intentionally abuse the TEST_NULLPR variable: this will only work
# with the fallback code.
check_cxx_source_compiles("#include \"${CMAKE_CURRENT_BINARY_DIR}/test_compiler_detection_allow_unknown.h\"
int main() {\n int i = TEST_NULLPTR;\n return 0; }\n"
  file_include_works_allow_unknown
)
if (NOT file_include_works_allow_unknown)
  message(SEND_ERROR "Inclusion of ${CMAKE_CURRENT_BINARY_DIR}/test_compiler_detection_allow_unknown.h was expected to work, but did not.")
endif()
