cmake_minimum_required(VERSION 2.8.12)
project(VSNASM C ASM_NASM)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  add_definitions(-DTESTx64)
  string(APPEND CMAKE_ASM_NASM_FLAGS " -DTEST2x64")
else()
  add_definitions(-DTESTi386)
endif()

# Test quoting for definitions with spaces.
add_definitions("-DEAX_COMMA_SPACE_ZERO=eax, 0")

# Test quoting for file names with spaces. The file is generated because CMake
# itself cannot have files with spaces.
file(READ bar.asm BAR_ASM_CONTENTS)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/bar baz.asm" "${BAR_ASM_CONTENTS}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable(VSNASM main.c foo.asm "${CMAKE_CURRENT_BINARY_DIR}/bar baz.asm")
