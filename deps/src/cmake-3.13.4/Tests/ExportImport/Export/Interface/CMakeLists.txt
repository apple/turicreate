
add_library(headeronly INTERFACE)
set_property(TARGET headeronly PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/headeronly>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include/headeronly>"
)
set_property(TARGET headeronly PROPERTY INTERFACE_COMPILE_DEFINITIONS "HEADERONLY_DEFINE")

include(GenerateExportHeader)
add_library(sharedlib SHARED sharedlib.cpp)
generate_export_header(sharedlib)
set_property(TARGET sharedlib PROPERTY INCLUDE_DIRECTORIES
  "${CMAKE_CURRENT_SOURCE_DIR}/sharedlib"
  "${CMAKE_CURRENT_BINARY_DIR}"
)
set_property(TARGET sharedlib PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sharedlib;${CMAKE_CURRENT_BINARY_DIR}>"
  "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/sharedlib>"
)

set_property(TARGET sharedlib PROPERTY INTERFACE_COMPILE_DEFINITIONS "SHAREDLIB_DEFINE")

add_library(sharediface INTERFACE)
target_link_libraries(sharediface INTERFACE sharedlib)

add_library(use_auto_type INTERFACE)
target_compile_features(use_auto_type INTERFACE cxx_auto_type)

add_library(use_c_restrict INTERFACE)
target_compile_features(use_c_restrict INTERFACE c_restrict)

add_library(source_target INTERFACE)
target_sources(source_target INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source_target.cpp>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/src/source_target_for_install.cpp>
)
install(FILES
  source_target_for_install.cpp
  DESTINATION src
)

add_library(cmakeonly INTERFACE)
set_property(TARGET cmakeonly PROPERTY INTERFACE_COMPILE_DEFINITIONS [[DEF="\"\$\B"]])

install(TARGETS headeronly sharediface use_auto_type use_c_restrict source_target
                cmakeonly
  EXPORT expInterface
)
install(TARGETS sharedlib
  EXPORT expInterface
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib NAMELINK_SKIP
  ARCHIVE DESTINATION lib
  FRAMEWORK DESTINATION Frameworks
  BUNDLE DESTINATION Applications
)
install(FILES
  headeronly/headeronly.h
  DESTINATION include/headeronly
)
install(FILES
  sharedlib/sharedlib.h
  "${CMAKE_CURRENT_BINARY_DIR}/sharedlib_export.h"
  DESTINATION include/sharedlib
)

install(EXPORT expInterface NAMESPACE exp:: DESTINATION lib/exp)

export(EXPORT expInterface
  NAMESPACE bld::
  FILE ../ExportInterfaceBuildTree.cmake
)
