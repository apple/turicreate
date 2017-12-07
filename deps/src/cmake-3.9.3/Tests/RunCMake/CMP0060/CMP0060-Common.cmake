# Always build in a predictable configuration.  For multi-config
# generators we depend on RunCMakeTest.cmake to do this for us.
if(NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Convince CMake that it can instruct the linker to search for the
# library of the proper linkage type, but do not really pass flags.
set(CMAKE_EXE_LINK_STATIC_C_FLAGS " ")
set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS " ")

# Make a link line asking for the linker to search for the library
# look like a missing object file so we will get predictable content
# in the error message.  This also ensures that cases expected to use
# the full path can be verified by confirming that they link.
set(CMAKE_LINK_LIBRARY_FLAG LINKFLAG_)
set(CMAKE_LINK_LIBRARY_SUFFIX _LINKSUFFIX${CMAKE_C_OUTPUT_EXTENSION})

# Convince CMake that our library is in an implicit linker search directory.
list(APPEND CMAKE_C_IMPLICIT_LINK_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR}/lib)

# Create a simple library file.  Place it in our library directory.
add_library(CMP0060 STATIC cmp0060.c)
set_property(TARGET CMP0060 PROPERTY
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/lib)

# Add a target to link the library file by full path.
add_executable(main1 main.c)
target_link_libraries(main1 $<TARGET_FILE:CMP0060>)
add_dependencies(main1 CMP0060)

# Add a second target to verify the warning only appears once.
add_executable(main2 main.c)
target_link_libraries(main2 $<TARGET_FILE:CMP0060>)
add_dependencies(main2 CMP0060)
