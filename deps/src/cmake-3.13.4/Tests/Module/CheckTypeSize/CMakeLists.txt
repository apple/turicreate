cmake_minimum_required(VERSION 2.8.1 FATAL_ERROR)
project(CheckTypeSize)

# Check C types
include(CheckTypeSize)
check_type_size("void*"     SIZEOF_DATA_PTR)
check_type_size(char        SIZEOF_CHAR)
check_type_size(short       SIZEOF_SHORT)
check_type_size(int         SIZEOF_INT)
check_type_size(long        SIZEOF_LONG)
check_type_size("long long" SIZEOF_LONG_LONG)
check_type_size(__int64     SIZEOF___INT64)
check_type_size(size_t      SIZEOF_SIZE_T)
check_type_size(ssize_t     SIZEOF_SSIZE_T)

set(CMAKE_REQUIRED_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_EXTRA_INCLUDE_FILES somestruct.h)
check_type_size("((struct somestruct*)0)->someint" SIZEOF_STRUCTMEMBER_INT)
check_type_size("((struct somestruct*)0)->someptr" SIZEOF_STRUCTMEMBER_PTR)
check_type_size("((struct somestruct*)0)->somechar" SIZEOF_STRUCTMEMBER_CHAR)

# Check CXX types
check_type_size(bool        SIZEOF_BOOL LANGUAGE CXX)

set(CMAKE_EXTRA_INCLUDE_FILES someclass.hxx)
check_type_size("((ns::someclass*)0)->someint" SIZEOF_NS_CLASSMEMBER_INT LANGUAGE CXX)
check_type_size("((ns::someclass*)0)->someptr" SIZEOF_NS_CLASSMEMBER_PTR LANGUAGE CXX)
check_type_size("((ns::someclass*)0)->somechar" SIZEOF_NS_CLASSMEMBER_CHAR LANGUAGE CXX)
check_type_size("((ns::someclass*)0)->somebool" SIZEOF_NS_CLASSMEMBER_BOOL LANGUAGE CXX)

configure_file(config.h.in config.h)
configure_file(config.hxx.in config.hxx)

include_directories("${CheckTypeSize_BINARY_DIR}")

add_executable(CheckTypeSize CheckTypeSize.c)
add_executable(CheckTypeSizeCXX CheckTypeSize.cxx)
