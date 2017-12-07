cmake_minimum_required(VERSION 2.8)
project(FindLibraryTest NONE)

set(CMAKE_FIND_DEBUG_MODE 1)

macro(test_find_library desc expected)
  unset(LIB CACHE)
  find_library(LIB ${ARGN} NO_DEFAULT_PATH)
  if(LIB)
    # Convert to relative path for comparison to expected location.
    file(RELATIVE_PATH REL_LIB "${CMAKE_CURRENT_SOURCE_DIR}" "${LIB}")

    # Check and report failure.
    if(NOT "${REL_LIB}" STREQUAL "${expected}")
      message(SEND_ERROR "Library ${expected} found as [${REL_LIB}]${desc}")
    elseif(CMAKE_FIND_DEBUG_MODE)
      message(STATUS "Library ${expected} found as [${REL_LIB}]${desc}")
    endif()
  else()
    message(SEND_ERROR "Library ${expected} NOT FOUND${desc}")
  endif()
endmacro()

macro(test_find_library_subst expected)
  get_filename_component(dir ${expected} PATH)
  get_filename_component(name ${expected} NAME)
  string(REGEX REPLACE "lib/?[36Xx][24Y3][Z2]*" "lib" dir "${dir}")
  test_find_library(", searched as ${dir}" "${expected}"
    NAMES ${name}
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}
    )
endmacro()

set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIBX32_PATHS TRUE)
set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB32_PATHS TRUE)
set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS TRUE)

set(CMAKE_INTERNAL_PLATFORM_ABI "ELF")
set(CMAKE_SIZEOF_VOID_P 4)
foreach(lib
    lib/32/libtest5.a
    lib/A/lib/libtest1.a
    lib/A/lib32/libtest3.a
    lib/A/libtest1.a
    lib/libtest1.a
    lib/libtest2.a
    lib/libtest3.a
    lib/libtest3.a
    lib32/A/lib/libtest2.a
    lib32/A/lib32/libtest4.a
    lib32/A/libtest4.a
    lib32/libtest4.a
    )
  test_find_library_subst(${lib})
endforeach()

set(CMAKE_SIZEOF_VOID_P 8)
foreach(lib64
    lib/64/libtest2.a
    lib/A/lib64/libtest3.a
    lib/libtest3.a
    lib64/A/lib/libtest2.a
    lib64/A/lib64/libtest1.a
    lib64/A/libtest1.a
    lib64/libtest1.a
    )
  test_find_library_subst(${lib64})
endforeach()

set(CMAKE_INTERNAL_PLATFORM_ABI "ELF X32")
set(CMAKE_SIZEOF_VOID_P 4)
foreach(libx32
    lib/x32/libtest2.a
    lib/A/libx32/libtest3.a
    lib/libtest3.a
    libx32/A/lib/libtest2.a
    libx32/A/libx32/libtest1.a
    libx32/A/libtest1.a
    libx32/libtest1.a
    )
  test_find_library_subst(${libx32})
endforeach()

test_find_library("" A/libtestA.a
  NAMES testA testB
  PATHS ${CMAKE_CURRENT_SOURCE_DIR}/A ${CMAKE_CURRENT_SOURCE_DIR}/B
  )
test_find_library("" B/libtestB.a
  NAMES testB testA
  PATHS ${CMAKE_CURRENT_SOURCE_DIR}/A ${CMAKE_CURRENT_SOURCE_DIR}/B
  )
test_find_library("" A/libtestA.a
  NAMES testB testA NAMES_PER_DIR
  PATHS ${CMAKE_CURRENT_SOURCE_DIR}/A ${CMAKE_CURRENT_SOURCE_DIR}/B
  )

set(CMAKE_FIND_LIBRARY_CUSTOM_LIB_SUFFIX "XYZ")
foreach(libXYZ
    lib/XYZ/libtest1.a
    lib/A/libXYZ/libtest2.a
    lib/libtest3.a
    libXYZ/A/lib/libtest4.a
    libXYZ/A/libXYZ/libtest5.a
    libXYZ/A/libtest6.a
    libXYZ/libtest7.a
    )
  test_find_library_subst(${libXYZ})
endforeach()
