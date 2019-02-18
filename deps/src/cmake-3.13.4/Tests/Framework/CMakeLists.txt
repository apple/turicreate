cmake_minimum_required (VERSION 2.6)
project(Framework)

add_library(foo SHARED
  foo.cxx
  foo.h
  foo2.h
  fooExtensionlessResource
  fooPublic.h
  fooPublicExtensionlessHeader
  fooPrivate.h
  fooPrivateExtensionlessHeader
  fooNeither.h
  fooBoth.h
  test.lua
  fooDeepPublic.h
)
set_property(SOURCE fooDeepPublic.h
  PROPERTY MACOSX_PACKAGE_LOCATION Headers/Deep
  )
set(foo_ver ver4)

set_target_properties(foo PROPERTIES
  FRAMEWORK TRUE
  FRAMEWORK_VERSION ${foo_ver}
  PRIVATE_HEADER "fooPrivate.h;fooBoth.h;fooPrivateExtensionlessHeader"
  PUBLIC_HEADER "foo.h;foo2.h;fooPublic.h;fooBoth.h;fooPublicExtensionlessHeader"
  RESOURCE "fooExtensionlessResource;test.lua"
  INSTALL_NAME_DIR "@executable_path/../../../Library/Frameworks"
  DEBUG_POSTFIX -d
)
# fooBoth.h is listed as both public and private... (private wins...)
# fooNeither.h is listed as neither public nor private...

add_executable(bar bar.cxx)
target_link_libraries(bar foo)
install(TARGETS foo bar
  RUNTIME DESTINATION Applications/CMakeTestsFramework/bin
  FRAMEWORK DESTINATION Library/Frameworks
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib

  # These are ignored on the Mac... and things are automatically placed in
  # their appropriate Framework sub-folder at build time. (And then the built
  # framework is copied recursively when it is installed.)
  PRIVATE_HEADER DESTINATION share/foo-${foo_ver}/PrivateHeaders
  PUBLIC_HEADER DESTINATION include/foo-${foo_ver}
  RESOURCE DESTINATION share/foo-${foo_ver}/Resources
  # But they are required to be present so that installing a framework on other
  # other platforms will install the pieces of the framework without having to
  # duplicate install rules for the pieces of the framework.
)

# test that framework post-build commands run
add_custom_command(TARGET foo POST_BUILD COMMAND ${CMAKE_COMMAND} -E touch foo-post-build)
add_custom_target(fooCustom ALL COMMAND ${CMAKE_COMMAND} -E copy foo-post-build foo-custom)
add_dependencies(fooCustom foo)

# Make a static library and apply the framework properties to it to verify
# that everything still builds correctly. Xcode prior to version 5 does not
# support static Frameworks.
#
if(NOT XCODE OR NOT XCODE_VERSION VERSION_LESS 5)
  add_library(fooStatic STATIC
    foo.cxx
    foo.h
    foo2.h
    fooExtensionlessResource
    fooPublic.h
    fooPublicExtensionlessHeader
    fooPrivate.h
    fooPrivateExtensionlessHeader
    fooNeither.h
    fooBoth.h
    test.lua
    fooDeepPublic.h
  )
  set_target_properties(fooStatic PROPERTIES
    FRAMEWORK TRUE
    FRAMEWORK_VERSION none
  )
  add_executable(barStatic bar.cxx)
  target_link_libraries(barStatic fooStatic)
endif()

include(CPack)
