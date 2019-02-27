project(BundleGeneratorTest)

cmake_minimum_required(VERSION 2.7)

# Build a shared library and install it in lib/
add_library(Library SHARED Library.cxx)
install(TARGETS Library DESTINATION lib)

# Build an executable and install it in bin/
add_executable(Executable Executable.cxx)
target_link_libraries(Executable Library)
install(TARGETS Executable DESTINATION bin)

# Use the bundle-generator for packaging ...
set(CPACK_GENERATOR "Bundle")
set(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/BundleIcon.icns")
set(CPACK_BUNDLE_NAME "BundleGeneratorTest")
set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
set(CPACK_BUNDLE_STARTUP_COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/StartupCommand")
set(CPACK_PACKAGE_DESCRIPTION "Project for testing OSX bundle generation")

# The custom volume icon is a copy of the normal Mac OSX volume icon, but
# on a white background. This is to differentiate it from the normal one
# so that you can verify that the custom icon is being used by doing a
# visual inspection of the mounted volume... This was added when fixing
# issue #7523...
#
set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/CustomVolumeIcon.icns")

set(CPACK_PACKAGE_NAME "BundleGeneratorTest")
set(CPACK_PACKAGE_VERSION "0.1")

include(CPack)
