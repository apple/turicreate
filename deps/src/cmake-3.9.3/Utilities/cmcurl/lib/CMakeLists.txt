set(LIB_NAME cmcurl)

configure_file(${CURL_SOURCE_DIR}/include/curl/curlbuild.h.cmake
  ${CURL_BINARY_DIR}/include/curl/curlbuild.h)
configure_file(curl_config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/curl_config.h)

transform_makefile_inc("Makefile.inc" "${CMAKE_CURRENT_BINARY_DIR}/Makefile.inc.cmake")
include(${CMAKE_CURRENT_BINARY_DIR}/Makefile.inc.cmake)

list(APPEND HHEADERS
  ${CMAKE_CURRENT_BINARY_DIR}/curl_config.h
  ${CURL_BINARY_DIR}/include/curl/curlbuild.h
  )

if(MSVC AND NOT CURL_STATICLIB)
  list(APPEND CSOURCES libcurl.rc)
endif()

# SET(CSOURCES
# #  memdebug.c -not used
# # nwlib.c - Not used
# # strtok.c - specify later
# # strtoofft.c - specify later
# )

# # if we have Kerberos 4, right now this is never on
# #OPTION(CURL_KRB4 "Use Kerberos 4" OFF)
# IF(CURL_KRB4)
# SET(CSOURCES ${CSOURCES}
# krb4.c
# security.c
# )
# ENDIF(CURL_KRB4)

# #OPTION(CURL_MALLOC_DEBUG "Debug mallocs in Curl" OFF)
# MARK_AS_ADVANCED(CURL_MALLOC_DEBUG)
# IF(CURL_MALLOC_DEBUG)
# SET(CSOURCES ${CSOURCES}
# memdebug.c
# )
# ENDIF(CURL_MALLOC_DEBUG)

# # only build compat strtoofft if we need to
# IF(NOT HAVE_STRTOLL AND NOT HAVE__STRTOI64)
# SET(CSOURCES ${CSOURCES}
# strtoofft.c
# )
# ENDIF(NOT HAVE_STRTOLL AND NOT HAVE__STRTOI64)


# The rest of the build

include_directories(${CMAKE_CURRENT_BINARY_DIR}/../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
if(USE_ARES)
  include_directories(${CARES_INCLUDE_DIR})
endif()

if(CURL_STATICLIB)
  # Static lib
  set(CURL_USER_DEFINED_DYNAMIC_OR_STATIC STATIC)
else()
  # DLL / so dynamic lib
  set(CURL_USER_DEFINED_DYNAMIC_OR_STATIC SHARED)
endif()

# For windows we want to install OPENSSL_LIBRARIES dlls
# and also copy them into the build tree so that testing
# can find them.
if(CMAKE_USE_OPENSSL AND OPENSSL_FOUND AND WIN32)
  find_file(CMAKE_EAY_DLL NAME libeay32.dll HINTS ${OPENSSL_INCLUDE_DIR}/..)
  find_file(CMAKE_SSL_DLL NAME ssleay32.dll HINTS ${OPENSSL_INCLUDE_DIR}/..)
  mark_as_advanced(CMAKE_EAY_DLL CMAKE_SSL_DLL)
  if(CMAKE_SSL_DLL AND CMAKE_EAY_DLL)
    set(CMAKE_CURL_SSL_DLLS ${CMake_BIN_DIR}/${CMAKE_CFG_INTDIR}/libeay32.dll
      ${CMake_BIN_DIR}/${CMAKE_CFG_INTDIR}/ssleay32.dll)
    add_custom_command(OUTPUT
      ${CMake_BIN_DIR}/${CMAKE_CFG_INTDIR}/libeay32.dll
      DEPENDS ${CMAKE_EAY_DLL}
      COMMAND ${CMAKE_COMMAND} -E copy  ${CMAKE_EAY_DLL}
      ${CMake_BIN_DIR}/${CMAKE_CFG_INTDIR}/libeay32.dll)
    add_custom_command(OUTPUT
      ${CMake_BIN_DIR}/${CMAKE_CFG_INTDIR}/ssleay32.dll
      DEPENDS ${CMAKE_SSL_DLL}
      COMMAND ${CMAKE_COMMAND} -E copy  ${CMAKE_SSL_DLL}
      ${CMake_BIN_DIR}/${CMAKE_CFG_INTDIR}/ssleay32.dll)
    install(PROGRAMS ${CMAKE_EAY_DLL} ${CMAKE_SSL_DLL} DESTINATION bin)
  endif()
endif()

add_library(
  ${LIB_NAME}
  ${CURL_USER_DEFINED_DYNAMIC_OR_STATIC}
  ${HHEADERS} ${CSOURCES}
  ${CMAKE_CURL_SSL_DLLS}
  )

target_link_libraries(${LIB_NAME} ${CURL_LIBS})

if(0) # This code not needed for building within CMake.
if(WIN32)
  add_definitions( -D_USRDLL )
endif()
endif()

set_target_properties(${LIB_NAME} PROPERTIES COMPILE_DEFINITIONS BUILDING_LIBCURL)

if(0) # This code not needed for building within CMake.
if(HIDES_CURL_PRIVATE_SYMBOLS)
  set_property(TARGET ${LIB_NAME} APPEND PROPERTY COMPILE_DEFINITIONS "CURL_HIDDEN_SYMBOLS")
  set_property(TARGET ${LIB_NAME} APPEND PROPERTY COMPILE_FLAGS ${CURL_CFLAG_SYMBOLS_HIDE})
endif()

# Remove the "lib" prefix since the library is already named "libcurl".
set_target_properties(${LIB_NAME} PROPERTIES PREFIX "")
set_target_properties(${LIB_NAME} PROPERTIES IMPORT_PREFIX "")

if(WIN32)
  if(NOT CURL_STATICLIB)
    # Add "_imp" as a suffix before the extension to avoid conflicting with the statically linked "libcurl.lib"
    set_target_properties(${LIB_NAME} PROPERTIES IMPORT_SUFFIX "_imp.lib")
  endif()
endif()

install(TARGETS ${LIB_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
endif()
