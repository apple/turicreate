project(Turi)

set(_nanomsg_dir nanomsg-1.1.5)

if(NOT APPLE)

  message("Building nanomsg using native build system.")

  set(NN_STATIC_LIB 1 CACHE BOOL "nanomsg static lib")
  set(NN_TESTS 0 CACHE BOOL "nanomsg tests.")
  set(NN_TOOLS 0 CACHE BOOL "nanomsg tools.")
  set(NN_ENABLE_DOC 0 CACHE BOOL "nanomsg docs.")

  # It doesn't seem that all Osx sdks compile correctly with GetAddrinfo_a
  set(NN_ENABLE_GETADDRINFO_A 0 CACHE BOOL "Disable Getaddrinfo_a.")
  
  set(NN_PACKAGE_VERSION 1.1.5)
 
  if (DEFINED ENV{TRAVIS_TAG})
    # Needed workaround for building this in our travis system. 
    unset(ENV{TRAVIS_TAG}) 
  endif()

  set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/${_nanomsg_dir}/cmake")

  add_subdirectory(${_nanomsg_dir})

else()
  
  message("Building nanomsg using predefined cross-compilation.")

  add_definitions(-DHAVE_USLEEP)
  add_definitions(-DNN_USE_OSX)
  add_definitions(-DNN_USE_PIPE)
  add_definitions(-DNN_USE_SOCKETPAIR)
  add_definitions(-DNN_USE_POLL)
  add_definitions(-DNN_USE_KQUEUE)
  add_definitions(-DNN_HAVE_KQUEUE)
  add_definitions(-DNN_HAVE_POLL)
  add_definitions(-DNN_HAVE_MSG_CONTROL)
  add_definitions(-DNN_USE_GCC_ATOMIC_BUILTINS)
  add_definitions(-DNN_MAX_SOCKETS=512)
  add_definitions(-DNN_STATIC_LIB)
  add_definitions(-DNN_USE_SEMAPHORE_PTHREAD)

  file(GLOB_RECURSE _nanomsg_src_files
    CONFIGURE_DEPENDS
    ${_nanomsg_dir}/src/*.c
    ${_nanomsg_dir}/src/*.cpp)

  make_library(nanomsg SOURCES ${_nanomsg_src_files})
endif()

