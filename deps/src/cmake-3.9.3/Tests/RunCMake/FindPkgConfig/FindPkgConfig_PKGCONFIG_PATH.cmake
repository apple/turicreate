# Needed for CMAKE_SYSTEM_NAME, CMAKE_LIBRARY_ARCHITECTURE, FIND_LIBRARY_USE_LIB32_PATHS and FIND_LIBRARY_USE_LIB64_PATHS
enable_language(C)

# Prepare environment and variables
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/pc-foo")
if(WIN32)
    set(PKG_CONFIG_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}\\dummy-pkg-config.bat")
    set(ENV{CMAKE_PREFIX_PATH} "${CMAKE_CURRENT_SOURCE_DIR}\\pc-bar;X:\\this\\directory\\should\\not\\exist\\in\\the\\filesystem")
    set(ENV{PKG_CONFIG_PATH} "C:\\baz")
else()
    set(PKG_CONFIG_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/dummy-pkg-config.sh")
    set(ENV{CMAKE_PREFIX_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/pc-bar:/this/directory/should/not/exist/in/the/filesystem")
    set(ENV{PKG_CONFIG_PATH} "/baz")
endif()


find_package(PkgConfig)


if(NOT DEFINED CMAKE_SYSTEM_NAME
    OR (CMAKE_SYSTEM_NAME MATCHES "^(Linux|kFreeBSD|GNU)$"
    AND NOT CMAKE_CROSSCOMPILING))
  if(EXISTS "/etc/debian_version") # is this a debian system ?
    if(CMAKE_LIBRARY_ARCHITECTURE MATCHES "^(i386-linux-gnu|x86_64-linux-gnu)$")
      # Cannot create directories for all the existing architectures...
      set(expected_path "/baz:${CMAKE_CURRENT_SOURCE_DIR}/pc-foo/lib/${CMAKE_LIBRARY_ARCHITECTURE}/pkgconfig:${CMAKE_CURRENT_SOURCE_DIR}/pc-foo/lib/pkgconfig:${CMAKE_CURRENT_SOURCE_DIR}/pc-bar/lib/${CMAKE_LIBRARY_ARCHITECTURE}/pkgconfig:${CMAKE_CURRENT_SOURCE_DIR}/pc-bar/lib/pkgconfig")
    else()
      set(expected_path "/baz:${CMAKE_CURRENT_SOURCE_DIR}/pc-foo/lib/pkgconfig:${CMAKE_CURRENT_SOURCE_DIR}/pc-bar/lib/pkgconfig")
    endif()
  else()
    # not debian, check the FIND_LIBRARY_USE_LIB32_PATHS and FIND_LIBRARY_USE_LIB64_PATHS propertie
    get_property(uselibx32 GLOBAL PROPERTY FIND_LIBRARY_USE_LIBX32_PATHS)
    get_property(uselib32 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB32_PATHS)
    get_property(uselib64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
    if(uselibx32 AND CMAKE_INTERNAL_PLATFORM_ABI STREQUAL "ELF X32")
      set(expected_path "/baz:${CMAKE_CURRENT_SOURCE_DIR}/pc-foo/libx32/pkgconfig:${CMAKE_CURRENT_SOURCE_DIR}/pc-foo/lib/pkgconfig:${CMAKE_CURRENT_SOURCE_DIR}/pc-bar/libx32/pkgconfig:${CMAKE_CURRENT_SOURCE_DIR}/pc-bar/lib/pkgconfig")
    elseif(uselib32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(expected_path "/baz:${CMAKE_CURRENT_SOURCE_DIR}/pc-foo/lib32/pkgconfig:${CMAKE_CURRENT_SOURCE_DIR}/pc-foo/lib/pkgconfig:${CMAKE_CURRENT_SOURCE_DIR}/pc-bar/lib32/pkgconfig:${CMAKE_CURRENT_SOURCE_DIR}/pc-bar/lib/pkgconfig")
    elseif(uselib64 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(expected_path "/baz:${CMAKE_CURRENT_SOURCE_DIR}/pc-foo/lib64/pkgconfig:${CMAKE_CURRENT_SOURCE_DIR}/pc-foo/lib/pkgconfig:${CMAKE_CURRENT_SOURCE_DIR}/pc-bar/lib64/pkgconfig:${CMAKE_CURRENT_SOURCE_DIR}/pc-bar/lib/pkgconfig")
    else()
      set(expected_path "/baz:${CMAKE_CURRENT_SOURCE_DIR}/pc-foo/lib/pkgconfig:${CMAKE_CURRENT_SOURCE_DIR}/pc-bar/lib/pkgconfig")
    endif()
  endif()
else()
  if(WIN32)
    set(expected_path "C:\\baz;${CMAKE_CURRENT_SOURCE_DIR}\\pc-foo\\lib\\pkgconfig;${CMAKE_CURRENT_SOURCE_DIR}\\pc-bar\\lib\\pkgconfig")
  else()
    set(expected_path "/baz:${CMAKE_CURRENT_SOURCE_DIR}/pc-foo/lib/pkgconfig:${CMAKE_CURRENT_SOURCE_DIR}/pc-bar/lib/pkgconfig")
  endif()
endif()


pkg_check_modules(FOO "${expected_path}")

if(NOT FOO_FOUND)
  message(FATAL_ERROR "Expected PKG_CONFIG_PATH: \"${expected_path}\".")
endif()
