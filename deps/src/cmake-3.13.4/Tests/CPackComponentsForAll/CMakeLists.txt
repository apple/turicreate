# CPack Example: User-selectable Installation Components
#
# In this example, we have a simple library (mylib) with an example
# application (mylibapp). We create a binary installer (a CPack Generator)
# which supports CPack components.
#
# Depending on the CPack generator and on some CPACK_xxx var values
# the generator may produce a single (NSIS, PackageMaker)
# or several package files (Archive Generators, RPM, DEB)
cmake_minimum_required(VERSION 2.8.3.20101130 FATAL_ERROR)
project(CPackComponentsForAll)

# Use GNUInstallDirs in order to enforce lib64 if needed
include(GNUInstallDirs)

# Create the mylib library
add_library(mylib mylib.cpp)

# Create the mylibapp application
add_executable(mylibapp mylibapp.cpp)
target_link_libraries(mylibapp mylib)

# Duplicate of mylibapp application
# which won't be put in any component (?mistake?)
add_executable(mylibapp2 mylibapp.cpp)
target_link_libraries(mylibapp2 mylib)

# Create installation targets. Note that we put each kind of file
# into a different component via COMPONENT. These components will
# be used to create the installation components.
install(TARGETS mylib
  ARCHIVE
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT libraries)
install(TARGETS mylibapp
  RUNTIME
  DESTINATION bin
  COMPONENT applications)

# This application does not belong to any component
# thus (as of cmake 2.8.2) it will be left "uninstalled"
# by a component-aware installer unless a
# CPACK_MONOLITHIC_INSTALL=1 is set (at cmake time).
install(TARGETS mylibapp2
  RUNTIME
  DESTINATION bin/@in@_@path@@with\\@and\\@/\@in_path\@) # test @ char in path

install(FILES mylib.h
  DESTINATION include
  COMPONENT headers)

if("${CPACK_GENERATOR}" MATCHES "RPM")
  ############## test man pages
  install(FILES mylib
    DESTINATION share/man/mylib/man3/mylib.1)
  install(FILES mylib
    DESTINATION share/man/mylib/man3/mylib.1 RENAME mylib2)

  ############## test symlinks
  # Package symbolic links
  install(DIRECTORY DESTINATION ${CMAKE_INSTALL_LIBDIR}/inside_relocatable_one/depth_two/depth_three COMPONENT libraries)
  install(DIRECTORY DESTINATION ${CMAKE_INSTALL_LIBDIR}/inside_relocatable_two/depth_two/different_relocatable/bar COMPONENT libraries)
  install(DIRECTORY DESTINATION other_relocatable/depth_two COMPONENT libraries)
  install(DIRECTORY DESTINATION non_relocatable/depth_two COMPONENT libraries)
  # test symbolic links to same dir
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink depth_three symlink_samedir_path)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/symlink_samedir_path DESTINATION ${CMAKE_INSTALL_LIBDIR}/inside_relocatable_one/depth_two COMPONENT libraries)
  # test symbolic links to same dir with current dir ./ prefix
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ./depth_three symlink_samedir_path_current_dir)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/symlink_samedir_path_current_dir DESTINATION ${CMAKE_INSTALL_LIBDIR}/inside_relocatable_one/depth_two COMPONENT libraries)
  # test symbolic links to same dir with longer relative path
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ../../../${CMAKE_INSTALL_LIBDIR}/.././${CMAKE_INSTALL_LIBDIR}/inside_relocatable_one/./depth_two/depth_three symlink_samedir_path_longer)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/symlink_samedir_path_longer DESTINATION ${CMAKE_INSTALL_LIBDIR}/inside_relocatable_one/depth_two COMPONENT libraries)
  # test symbolic links to sub dir
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ../../${CMAKE_INSTALL_LIBDIR}/inside_relocatable_one/depth_two/depth_three symlink_subdir_path)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/symlink_subdir_path DESTINATION ${CMAKE_INSTALL_LIBDIR}/inside_relocatable_one COMPONENT libraries)
  # test symbolic links to parent dir
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink .././../../../${CMAKE_INSTALL_LIBDIR}/inside_relocatable_one/./depth_two symlink_parentdir_path)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/symlink_parentdir_path DESTINATION ${CMAKE_INSTALL_LIBDIR}/inside_relocatable_one/depth_two/depth_three COMPONENT libraries)
  # test symbolic link to another relocatable path
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink .././.././../other_relocatable/./depth_two symlink_other_relocatable_path)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/symlink_other_relocatable_path DESTINATION ${CMAKE_INSTALL_LIBDIR}/inside_relocatable_two/depth_two COMPONENT libraries)
  # test symbolic link to non relocatable path
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink .././../../non_relocatable/./depth_two symlink_to_non_relocatable_path)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/symlink_to_non_relocatable_path DESTINATION ${CMAKE_INSTALL_LIBDIR}/inside_relocatable_two/depth_two COMPONENT libraries)
  # test symbolic link from non relocatable path
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink .././../${CMAKE_INSTALL_LIBDIR}/inside_relocatable_two/depth_two symlink_from_non_relocatable_path)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/symlink_from_non_relocatable_path DESTINATION non_relocatable/depth_two COMPONENT libraries)
  # test symbolic link relocatable path to its relocatable subpath
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ../../inside_relocatable_two/depth_two/different_relocatable/bar symlink_relocatable_subpath)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/symlink_relocatable_subpath DESTINATION ${CMAKE_INSTALL_LIBDIR}/inside_relocatable_one/depth_two COMPONENT libraries)
  # test symbolic link to location outside package
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ./outside_package symlink_outside_package)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/symlink_outside_package DESTINATION ${CMAKE_INSTALL_LIBDIR}/inside_relocatable_one/depth_two COMPONENT libraries)
  # test symbolic link to location outside wdr (packaging directory)
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink /outside_package_wdr symlink_outside_wdr)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/symlink_outside_wdr DESTINATION ${CMAKE_INSTALL_LIBDIR}/inside_relocatable_one/depth_two COMPONENT libraries)
endif()

# CPack boilerplate for this project
set(CPACK_PACKAGE_NAME "MyLib")
set(CPACK_PACKAGE_CONTACT "None")
set(CPACK_PACKAGE_VENDOR "CMake.org")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MyLib - CPack Component Installation Example")
set(CPACK_PACKAGE_VERSION "1.0.2")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "2")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CPack Component Example")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/license.txt)

# Tell CPack all of the components to install. The "ALL"
# refers to the fact that this is the set of components that
# will be included when CPack is instructed to put everything
# into the binary installer (the default behavior).
set(CPACK_COMPONENTS_ALL applications libraries headers Unspecified)

# Set the displayed names for each of the components to install.
# These will be displayed in the list of components inside the installer.
set(CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "MyLib Application")
set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C++ Headers")

# Provide descriptions for each of the components to install.
# When the user hovers the mouse over the name of a component,
# the description will be shown in the "Description" box in the
# installer. If no descriptions are provided, the "Description"
# box will be removed.
set(CPACK_COMPONENT_APPLICATIONS_DESCRIPTION
  "An extremely useful application that makes use of MyLib")
set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION
  "Static libraries used to build programs with MyLib")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION
  "C/C++ header files for use with MyLib")

# Put the components into two different groups: "Runtime" and "Development"
set(CPACK_COMPONENT_APPLICATIONS_GROUP "Runtime")
set(CPACK_COMPONENT_LIBRARIES_GROUP "Development")
set(CPACK_COMPONENT_HEADERS_GROUP "Development")

# Expand the "Development" group by default, since we have so few components.
# Also, provide this group with a description.
set(CPACK_COMPONENT_GROUP_DEVELOPMENT_EXPANDED ON)
set(CPACK_COMPONENT_GROUP_DEVELOPMENT_DESCRIPTION
  "All of the tools you'll ever need to develop software")

# It doesn't make sense to install the headers without the libraries
# (because you could never use the headers!), so make the headers component
# depend on the libraries component.
set(CPACK_COMPONENT_HEADERS_DEPENDS libraries)

# Create two installation types with pre-selected components.
# The "Developer" installation has just the library and headers,
# while the "Full" installation has everything.
set(CPACK_ALL_INSTALL_TYPES Full Developer)
set(CPACK_INSTALL_TYPE_FULL_DISPLAY_NAME "Everything")
set(CPACK_COMPONENT_LIBRARIES_INSTALL_TYPES Developer Full)
set(CPACK_COMPONENT_HEADERS_INSTALL_TYPES Developer Full)
set(CPACK_COMPONENT_APPLICATIONS_INSTALL_TYPES Full)

# set CPACK_RPM_RELOCATION_PATHS here as GNUInstallDirs script
# can not be used in CPack scripts due to CMAKE_SIZEOF_VOID_P
# variable not being set
set(CPACK_RPM_RELOCATION_PATHS "${CMAKE_INSTALL_INCLUDEDIR}"
  "${CMAKE_INSTALL_LIBDIR}" "${CMAKE_INSTALL_BINDIR}" "other_relocatable"
  "${CMAKE_INSTALL_LIBDIR}/inside_relocatable_two/depth_two/different_relocatable")

# set CPACK_DEBIAN_FILE_NAME to use default package name format
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")

# set some tags for NuGet packages
# 1. all in one package
set(CPACK_NUGET_PACKAGE_TAGS "nuget" "unit" "test" "all-in-one")
# 2. per component packages
set(CPACK_NUGET_APPLICATIONS_PACKAGE_TAGS "nuget" "unit" "test" "applications")
set(CPACK_NUGET_LIBRARIES_PACKAGE_TAGS "nuget" "unit" "test" "libraries")
set(CPACK_NUGET_HEADERS_PACKAGE_TAGS "nuget" "unit" "test" "headers")
set(CPACK_NUGET_UNSPECIFIED_PACKAGE_TAGS "nuget" "unit" "test" "uNsP3c1FiEd")
# 3. per group packages
set(CPACK_NUGET_RUNTIME_PACKAGE_TAGS "nuget" "unit" "test" "run-time")
set(CPACK_NUGET_DEVELOPMENT_PACKAGE_TAGS "nuget" "unit" "test" "development")

# We may use the CPack specific config file in order
# to tailor CPack behavior on a CPack generator specific way
# (Behavior would be different for RPM or TGZ or DEB ...)
if (NOT ("${CPackComponentWay}" STREQUAL "default"))
  # Setup project specific CPack-time CPack Config file.
  configure_file(${CPackComponentsForAll_SOURCE_DIR}/MyLibCPackConfig-${CPackComponentWay}.cmake.in
    ${CPackComponentsForAll_BINARY_DIR}/MyLibCPackConfig-${CPackComponentWay}.cmake
    @ONLY)
  set(CPACK_PROJECT_CONFIG_FILE ${CPackComponentsForAll_BINARY_DIR}/MyLibCPackConfig-${CPackComponentWay}.cmake)
endif ()
# Include CPack to introduce the appropriate targets
include(CPack)
