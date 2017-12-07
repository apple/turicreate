cmake_minimum_required(VERSION 2.8.7)

project(dmlc C CXX)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/build/private/local_config.cmake)
  include(${CMAKE_CURRENT_SOURCE_DIR}/build/private/local_config.cmake)
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)

include(cmake/Utils.cmake)
#include(cmake/dmlccore.cmake)

# Options
dmlccore_option(USE_HDFS "Build with HDFS support" OFF)
dmlccore_option(USE_AZURE "Build with AZURE support" OFF)
dmlccore_option(USE_S3 "Build with S3 support" OFF)
dmlccore_option(USE_OPENMP "Build with OpenMP" ON)

# include path
include_directories("include")

set(dmlccore_LINKER_LIBS "")
# HDFS configurations
if(USE_HDFS)
 find_package(HDFS REQUIRED)
 find_package(JNI REQUIRED)
 include_directories(${HDFS_INCLUDE_DIR})
 list(APPEND dmlccore_LINKER_LIBS ${HDFS_LIBRARIES} ${JAVA_JVM_LIBRARY})
 add_definitions(-DDMLC_USE_HDFS=1)
else()
 add_definitions(-DDMLC_USE_HDFS=0)
endif()
# S3 configurations
if(USE_S3)
 find_package(CURL REQUIRED)
 include_directories(SYSTEM ${CURL_INCLUDE_DIR})
 list(APPEND dmlccore_LINKER_LIBS ${CURL_LIBRARY})

 find_package(OpenSSL REQUIRED)
 include_directories(SYSTEM ${OPENSSL_INCLUDE_DIR})
 list(APPEND dmlccore_LINKER_LIBS ${OPENSSL_LIBRARY} ${OPENSSL_LIBRARIES})

 find_package(Crypto REQUIRED)
 include_directories(SYSTEM ${CRYPTO_INCLUDE_DIR})
 list(APPEND dmlccore_LINKER_LIBS ${CRYPTO_LIBRARY})

 add_definitions(-DDMLC_USE_S3=1)
else()
 add_definitions(-DDMLC_USE_S3=0)
endif()
# Azure configurations
if(USE_AZURE)
  add_definitions(-DDMLC_USE_AZURE=1)
else()
  add_definitions(-DDMLC_USE_AZURE=0)
endif()

# Older stdc++ enable c++11 items
add_definitions(-D__USE_XOPEN2K8)

# compile
if(MSVC)
  add_definitions(-DDMLC_USE_CXX11)
  foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MD")
  endforeach(flag_var)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
else(MSVC)
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-std=c++0x"    SUPPORT_CXX0X)
  check_cxx_compiler_flag("-msse2"        SUPPORT_MSSE2)
  set(CMAKE_C_FLAGS "-O3 -Wall -Wno-unknown-pragmas -fPIC")
  if(SUPPORT_CXX0X)
  	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c++0x")
  endif()
  if(SUPPORT_MSSE2)
  	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2")
  endif()
  set(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})
endif(MSVC)

if(USE_OPENMP)
  if(MSVC)
    set(OPENMP_FOUND ON)
    set(OpenMP_C_FLAGS "-openmp")
    set(OpenMP_CXX_FLAGS ${OpenMP_C_FLAGS})
  else()
    find_package(OpenMP REQUIRED)
  endif()
  if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
  endif()
endif()

FILE(GLOB SOURCE "src/*.cc")
FILE(GLOB_RECURSE SOURCE_INCLUDE "include/*")
list(APPEND SOURCE ${SOURCE_INCLUDE})
list(APPEND SOURCE "src/io/line_split.cc")
list(APPEND SOURCE "src/io/recordio_split.cc")
list(APPEND SOURCE "src/io/indexed_recordio_split.cc")
list(APPEND SOURCE "src/io/input_split_base.cc")
list(APPEND SOURCE "src/io/local_filesys.cc")

if(USE_HDFS)
  list(APPEND SOURCE "src/io/hdfs_filesys.cc")
endif()
if(USE_S3)
  list(APPEND SOURCE "src/io/s3_filesys.cc")
endif()
if(USE_AZURE)
  list(APPEND SOURCE "src/io/azure_filesys.cc")
endif()

add_library(dmlc ${SOURCE})
target_link_libraries(dmlc ${dmlccore_LINKER_LIBS})

# ---[ Install Includes
if(INSTALL_INCLUDE_DIR)
  add_custom_command(TARGET dmlc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/include ${INSTALL_INCLUDE_DIR}/
    )
endif()

# ---[ Install the archive static lib and header files
install(TARGETS dmlc ARCHIVE DESTINATION lib${LIB_SUFFIX} LIBRARY DESTINATION lib${LIB_SUFFIX})
install(DIRECTORY include DESTINATION .)
install(DIRECTORY doc DESTINATION .)

# ---[ Linter target
if(MSVC)
  find_package(PythonInterp)
  set(PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE} CACHE FILEPATH "Path to the python 2.x executable")
endif()
set(LINT_DIRS include src scripts)
add_custom_target(dmlc_lint COMMAND ${CMAKE_COMMAND} -DMSVC=${MSVC} -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}  -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR} -DLINT_DIRS=${LINT_DIRS} -DPROJECT_NAME=dmlc -P ${PROJECT_SOURCE_DIR}/cmake/lint.cmake)
