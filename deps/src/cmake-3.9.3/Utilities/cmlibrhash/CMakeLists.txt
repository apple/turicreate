project(librhash C)

# Disable warnings to avoid changing 3rd party code.
if(CMAKE_C_COMPILER_ID MATCHES
    "^(GNU|Clang|AppleClang|XL|VisualAge|SunPro|MIPSpro|HP|Intel)$")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
elseif(CMAKE_C_COMPILER_ID STREQUAL "PathScale")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -woffall")
endif()

set(librhash_sources
  librhash/algorithms.c
  librhash/algorithms.h
  librhash/byte_order.c
  librhash/byte_order.h
  librhash/hex.c
  librhash/hex.h
  librhash/md5.c
  librhash/md5.h
  librhash/rhash.c
  librhash/rhash.h
  librhash/sha1.c
  librhash/sha1.h
  librhash/sha256.c
  librhash/sha256.h
  librhash/sha3.c
  librhash/sha3.h
  librhash/sha512.c
  librhash/sha512.h
  librhash/ustd.h
  librhash/util.h
  )

include_directories(
  ${KWSYS_HEADER_ROOT}
  )

add_library(cmlibrhash ${librhash_sources})

install(FILES COPYING README DESTINATION ${CMAKE_DOC_DIR}/cmlibrhash)
