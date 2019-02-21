
enable_language(C)

set(obj "${CMAKE_C_OUTPUT_EXTENSION}")
if(BORLAND)
  set(pre -)
endif()

# basic configuration
add_library(LinkOptions SHARED LinkOptionsLib.c)
target_link_options(LinkOptions
  PRIVATE ${pre}BADFLAG_PRIVATE${obj}
  INTERFACE ${pre}BADFLAG_INTERFACE${obj})


# INTERFACE_LINK_OPTIONS
add_library(LinkOptions_producer SHARED LinkOptionsLib.c)
target_link_options(LinkOptions_producer
  INTERFACE ${pre}BADFLAG_INTERFACE${obj})

add_executable(LinkOptions_consumer LinkOptionsExe.c)
target_link_libraries(LinkOptions_consumer PRIVATE LinkOptions_producer)


# static library with INTERFACE_LINK_OPTIONS
add_library(LinkOptions_producer_static STATIC LinkOptionsLib.c)
target_link_options(LinkOptions_producer_static
  INTERFACE ${pre}BADFLAG_INTERFACE${obj})

add_executable(LinkOptions_consumer_static LinkOptionsExe.c)
target_link_libraries(LinkOptions_consumer_static PRIVATE LinkOptions_producer_static)


# static library with generator expression
add_library(LinkOptions_static STATIC LinkOptionsLib.c)
target_link_options(LinkOptions_static PRIVATE $<$<CONFIG:Release>:${pre}BADFLAG_RELEASE${obj}>
  "SHELL:" # produces no options
  )


# shared library with generator expression
add_library(LinkOptions_shared SHARED LinkOptionsLib.c)
target_link_options(LinkOptions_shared PRIVATE $<$<CONFIG:Release>:${pre}BADFLAG_RELEASE${obj}>
  "SHELL:" # produces no options
  )


# module library with generator expression
add_library(LinkOptions_mod MODULE LinkOptionsLib.c)
target_link_options(LinkOptions_mod PRIVATE $<$<CONFIG:Release>:${pre}BADFLAG_RELEASE${obj}>)


# executable with generator expression
add_executable(LinkOptions_exe LinkOptionsExe.c)
target_link_options(LinkOptions_exe PRIVATE $<$<CONFIG:Release>:${pre}BADFLAG_RELEASE${obj}>)
