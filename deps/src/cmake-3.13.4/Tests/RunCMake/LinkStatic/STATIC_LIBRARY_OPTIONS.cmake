
enable_language(C)

set(obj "${CMAKE_C_OUTPUT_EXTENSION}")
if(BORLAND)
  set(pre -)
endif()

add_library(StaticLinkOptions STATIC LinkOptionsLib.c)
set_property(TARGET StaticLinkOptions PROPERTY STATIC_LIBRARY_OPTIONS ${pre}BADFLAG${obj})

# static library with generator expression
add_library(StaticLinkOptions_genex STATIC LinkOptionsLib.c)
set_property(TARGET StaticLinkOptions_genex PROPERTY STATIC_LIBRARY_OPTIONS
  $<$<CONFIG:Release>:${pre}BADFLAG_RELEASE${obj}>
  "SHELL:" # produces no options
  )

# shared library do not use property STATIC_LIBRARY_OPTIONS
add_library(SharedLinkOptions SHARED LinkOptionsLib.c)
set_property(TARGET SharedLinkOptions PROPERTY STATIC_LIBRARY_OPTIONS ${pre}BADFLAG${obj})
