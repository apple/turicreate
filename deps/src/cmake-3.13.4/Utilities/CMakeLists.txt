# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

subdirs(Doxygen)

if(CMAKE_DOC_TARBALL)
  # Undocumented option to extract and install pre-built documentation.
  # This is intended for use during packaging of CMake itself.
  if(CMAKE_DOC_TARBALL MATCHES "/([^/]+)\\.tar\\.gz$")
    set(dir "${CMAKE_MATCH_1}")
  else()
    message(FATAL_ERROR "CMAKE_DOC_TARBALL must end in .tar.gz")
  endif()
  add_custom_command(
    OUTPUT ${dir}.stamp
    COMMAND cmake -E remove_directory ${dir}
    COMMAND cmake -E tar xf ${CMAKE_DOC_TARBALL}
    COMMAND cmake -E touch ${dir}.stamp
    DEPENDS ${CMAKE_DOC_TARBALL}
    )
  add_custom_target(documentation ALL DEPENDS ${dir}.stamp)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${dir}/
          DESTINATION . USE_SOURCE_PERMISSIONS)
else()
  # Normal documentation build.
  add_subdirectory(Sphinx)
endif()

if(WIX_CUSTOM_ACTION_ENABLED)
  add_subdirectory(Release/WiX)
endif()

# Make sure generated files use the same clang-tidy checks (none).
configure_file(.clang-tidy .clang-tidy COPYONLY)
