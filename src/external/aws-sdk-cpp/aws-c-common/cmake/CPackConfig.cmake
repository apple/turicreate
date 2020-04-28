# Configuration for CPack packaging
# ---------------------------------

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(STATUS "Packaging is only supported on Linux")
  return()
endif()

# Check for a RedHat-based OS
if (EXISTS /etc/redhat-release)
  set(CPACK_GENERATOR RPM)
else()
  message(STATUS "Packaging currently only supported on Fedora.")
  return()
endif()

# We'll want 2 RPMS, one for runtime files and one for development files
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL Development Runtime)
set(CPACK_RPM_MAIN_COMPONENT Runtime)

# Configure package names
set(CPACK_PACKAGE_NAME ${PROJECT_NAME} CACHE STRING "")
set(CPACK_RPM_Development_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-devel")

# Configure package summaries
set(CPACK_PACKAGE_SUMMARY "Core c99 package for AWS SDK for C")
set(CPACK_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_SUMMARY})
set(CPACK_RPM_Development_PACKAGE_SUMMARY
    "Development files for ${CPACK_PACKAGE_NAME}")

# Configure package versioning/metadata
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_RELEASE "1" CACHE STRING "")
set(CPACK_PACKAGE_CONTACT "TODO <TODO>")
set(CPACK_PACKAGE_VENDOR "Amazon")
set(CPACK_RPM_PACKAGE_LICENSE "ASL 2.0")
set(CPACK_RPM_PACKAGE_URL "https://github.com/awslabs/aws-c-common")

# Configure the RPM filenames
set(CPACK_RPM_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.rpm")
set(CPACK_RPM_Development_FILE_NAME "${CPACK_RPM_Development_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.rpm")

# Make the development package depend on the runtime one
set(CPACK_RPM_Development_PACKAGE_REQUIRES
    "${CPACK_PACKAGE_NAME} = ${PROJECT_VERSION}")

# Set the changelog file
set(CPACK_RPM_CHANGELOG_FILE
    "${CMAKE_CURRENT_LIST_DIR}/rpm-scripts/changelog.txt")

# If we are building shared libraries, we need to run ldconfig. Unfortunately,
# we can't set this per-component yet, so we'll have to do it on the devel
# package too
if (BUILD_SHARED_LIBS)
  set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE
      "${CMAKE_CURRENT_LIST_DIR}/rpm-scripts/post.sh")
  set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE
      "${CMAKE_CURRENT_LIST_DIR}/rpm-scripts/postun.sh")
endif()

# By default, we'll try to claim the cmake directory under the library directory
# and the aws include directory. We have to share both of these
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    /usr/${LIBRARY_DIRECTORY}/cmake
    /usr/include/aws)

# Include CPack, which generates the package target
include(CPack)
