# prevent older policies from interfearing with this script
cmake_policy(PUSH)
cmake_policy(VERSION ${CMAKE_VERSION})

message(STATUS "=============================================================================")
message(STATUS "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)")
message(STATUS "")

if(NOT CPackComponentsForAll_BINARY_DIR)
  message(FATAL_ERROR "CPackComponentsForAll_BINARY_DIR not set")
endif()

if(NOT CPackGen)
  message(FATAL_ERROR "CPackGen not set")
endif()

message("CMAKE_CPACK_COMMAND = ${CMAKE_CPACK_COMMAND}")
if(NOT CMAKE_CPACK_COMMAND)
  message(FATAL_ERROR "CMAKE_CPACK_COMMAND not set")
endif()

if(NOT CPackComponentWay)
  message(FATAL_ERROR "CPackComponentWay not set")
endif()

set(expected_file_mask "")
# The usual default behavior is to expect a single file
# Then some specific generators (Archive, RPM, ...)
# May produce several numbers of files depending on
# CPACK_COMPONENT_xxx values
set(expected_count 1)
set(config_type $ENV{CMAKE_CONFIG_TYPE})
set(config_args )
if(config_type)
  set(config_args -C ${config_type})
endif()
set(config_verbose )

if(CPackGen MATCHES "ZIP")
    set(expected_file_mask "${CPackComponentsForAll_BINARY_DIR}/MyLib-*.zip")
    if (${CPackComponentWay} STREQUAL "default")
        set(expected_count 1)
    elseif (${CPackComponentWay} STREQUAL "OnePackPerGroup")
        set(expected_count 3)
    elseif (${CPackComponentWay} STREQUAL "IgnoreGroup")
        set(expected_count 4)
    elseif (${CPackComponentWay} STREQUAL "AllInOne")
        set(expected_count 1)
    endif ()
elseif (CPackGen MATCHES "RPM")
    set(config_verbose -D "CPACK_RPM_PACKAGE_DEBUG=1")
    set(expected_file_mask "${CPackComponentsForAll_BINARY_DIR}/MyLib-*.rpm")
    if (${CPackComponentWay} STREQUAL "default")
        set(expected_count 1)
    elseif (${CPackComponentWay} STREQUAL "OnePackPerGroup")
        set(expected_count 3)
    elseif (${CPackComponentWay} STREQUAL "IgnoreGroup")
        set(expected_count 4)
    elseif (${CPackComponentWay} STREQUAL "AllInOne")
        set(expected_count 1)
    endif ()
elseif (CPackGen MATCHES "DEB")
    set(expected_file_mask "${CPackComponentsForAll_BINARY_DIR}/mylib*_1.0.2-1_*.deb")
    if (${CPackComponentWay} STREQUAL "default")
        set(expected_count 1)
    elseif (${CPackComponentWay} STREQUAL "OnePackPerGroup")
        set(expected_count 3)
    elseif (${CPackComponentWay} STREQUAL "IgnoreGroup")
        set(expected_count 4)
    elseif (${CPackComponentWay} STREQUAL "AllInOne")
        set(expected_count 1)
    endif ()
endif()

if(CPackGen MATCHES "DragNDrop")
    set(expected_file_mask "${CPackComponentsForAll_BINARY_DIR}/MyLib-*.dmg")
    if (${CPackComponentWay} STREQUAL "default")
        set(expected_count 1)
    elseif (${CPackComponentWay} STREQUAL "OnePackPerGroup")
        set(expected_count 3)
    elseif (${CPackComponentWay} STREQUAL "IgnoreGroup")
        set(expected_count 4)
    elseif (${CPackComponentWay} STREQUAL "AllInOne")
        set(expected_count 1)
    endif ()
endif()

# clean-up previously CPack generated files
if(expected_file_mask)
  file(GLOB expected_file "${expected_file_mask}")
  if (expected_file)
    file(REMOVE ${expected_file})
  endif()
endif()

message("config_args = ${config_args}")
message("config_verbose = ${config_verbose}")
execute_process(COMMAND ${CMAKE_CPACK_COMMAND} ${config_verbose} -G ${CPackGen} ${config_args}
    RESULT_VARIABLE CPack_result
    OUTPUT_VARIABLE CPack_output
    ERROR_VARIABLE CPack_error
    WORKING_DIRECTORY ${CPackComponentsForAll_BINARY_DIR})

if (CPack_result)
  message(FATAL_ERROR "error: CPack execution went wrong!, CPack_output=${CPack_output}, CPack_error=${CPack_error}")
else ()
  message(STATUS "CPack_output=${CPack_output}")
endif()

# Now verify if the number of expected file is OK
# - using expected_file_mask and
# - expected_count
if(expected_file_mask)
  file(GLOB expected_file "${expected_file_mask}")

  message(STATUS "expected_count='${expected_count}'")
  message(STATUS "expected_file='${expected_file}'")
  message(STATUS "expected_file_mask='${expected_file_mask}'")

  if(NOT expected_file)
    message(FATAL_ERROR "error: expected_file does not exist: CPackComponentsForAll test fails. (CPack_output=${CPack_output}, CPack_error=${CPack_error}")
  endif()

  list(LENGTH expected_file actual_count)
  message(STATUS "actual_count='${actual_count}'")
  if(NOT actual_count EQUAL expected_count)
    message(FATAL_ERROR "error: expected_count=${expected_count} does not match actual_count=${actual_count}: CPackComponents test fails. (CPack_output=${CPack_output}, CPack_error=${CPack_error})")
  endif()
endif()

# Validate content
if(CPackGen MATCHES "RPM")
  find_program(RPM_EXECUTABLE rpm)
  if(NOT RPM_EXECUTABLE)
    message(FATAL_ERROR "error: missing rpm executable required by the test")
  endif()

  set(CPACK_RPM_PACKAGE_SUMMARY "default summary")
  set(CPACK_RPM_HEADERS_PACKAGE_SUMMARY "headers summary")
  set(CPACK_RPM_HEADERS_PACKAGE_DESCRIPTION "headers description")
  set(CPACK_COMPONENT_APPLICATIONS_DESCRIPTION
    "An extremely useful application that makes use of MyLib")
  set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION
    "Static libraries used to build programs with MyLib")
  set(LIB_SUFFIX "6?4?")

  # test package info
  if(${CPackComponentWay} STREQUAL "IgnoreGroup")
    # set gnu install prefixes to what they are set during rpm creation
    # CMAKE_SIZEOF_VOID_P is not set here but lib is prefix of lib64 so
    # relocation path test won't fail on OSes with lib64 library location
    include(GNUInstallDirs)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/foo/bar")

    foreach(check_file ${expected_file})
      string(REGEX MATCH ".*libraries.*" check_file_libraries_match ${check_file})
      string(REGEX MATCH ".*headers.*" check_file_headers_match ${check_file})
      string(REGEX MATCH ".*applications.*" check_file_applications_match ${check_file})
      string(REGEX MATCH ".*Unspecified.*" check_file_Unspecified_match ${check_file})

      execute_process(COMMAND ${RPM_EXECUTABLE} -pqi ${check_file}
          OUTPUT_VARIABLE check_file_content
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE)

      execute_process(COMMAND ${RPM_EXECUTABLE} -pqa ${check_file}
          RESULT_VARIABLE check_package_architecture_result
          OUTPUT_VARIABLE check_package_architecture
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE)

      execute_process(COMMAND ${RPM_EXECUTABLE} -pql ${check_file}
          OUTPUT_VARIABLE check_package_content
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE)

      set(whitespaces "[\\t\\n\\r ]*")

      if(check_file_libraries_match)
        set(check_file_match_expected_summary ".*${CPACK_RPM_PACKAGE_SUMMARY}.*")
        set(check_file_match_expected_description ".*${CPACK_COMPONENT_LIBRARIES_DESCRIPTION}.*")
        set(check_file_match_expected_relocation_path "Relocations${whitespaces}:${whitespaces}${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}${LIB_SUFFIX}${whitespaces}${CPACK_PACKAGING_INSTALL_PREFIX}/other_relocatable${whitespaces}${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}${LIB_SUFFIX}/inside_relocatable_two/depth_two/different_relocatable")
        set(check_file_match_expected_architecture "") # we don't explicitly set this value so it is different on each platform - ignore it
        set(spec_regex "*libraries*")
        set(check_content_list "^/usr/foo/bar/lib${LIB_SUFFIX}
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_one
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_one/depth_two
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_one/depth_two/depth_three
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_one/depth_two/depth_three/symlink_parentdir_path
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_one/depth_two/symlink_outside_package
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_one/depth_two/symlink_outside_wdr
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_one/depth_two/symlink_relocatable_subpath
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_one/depth_two/symlink_samedir_path
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_one/depth_two/symlink_samedir_path_current_dir
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_one/depth_two/symlink_samedir_path_longer
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_one/symlink_subdir_path
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_two
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_two/depth_two
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_two/depth_two/different_relocatable
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_two/depth_two/different_relocatable/bar
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_two/depth_two/symlink_other_relocatable_path
/usr/foo/bar/lib${LIB_SUFFIX}/inside_relocatable_two/depth_two/symlink_to_non_relocatable_path
/usr/foo/bar/lib${LIB_SUFFIX}/libmylib.a
/usr/foo/bar/non_relocatable
/usr/foo/bar/non_relocatable/depth_two
/usr/foo/bar/non_relocatable/depth_two/symlink_from_non_relocatable_path
/usr/foo/bar/other_relocatable
/usr/foo/bar/other_relocatable/depth_two(\n.*\.build-id.*)*$")
      elseif(check_file_headers_match)
        set(check_file_match_expected_summary ".*${CPACK_RPM_HEADERS_PACKAGE_SUMMARY}.*")
        set(check_file_match_expected_description ".*${CPACK_RPM_HEADERS_PACKAGE_DESCRIPTION}.*")
        set(check_file_match_expected_relocation_path "Relocations${whitespaces}:${whitespaces}${CPACK_PACKAGING_INSTALL_PREFIX}${whitespaces}${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
        set(check_file_match_expected_architecture "noarch")
        set(spec_regex "*headers*")
        set(check_content_list "^/usr/foo/bar\n/usr/foo/bar/include\n/usr/foo/bar/include/mylib.h(\n.*\.build-id.*)*$")
      elseif(check_file_applications_match)
        set(check_file_match_expected_summary ".*${CPACK_RPM_PACKAGE_SUMMARY}.*")
        set(check_file_match_expected_description ".*${CPACK_COMPONENT_APPLICATIONS_DESCRIPTION}.*")
        set(check_file_match_expected_relocation_path "Relocations${whitespaces}:${whitespaces}${CPACK_PACKAGING_INSTALL_PREFIX}${whitespaces}${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
        set(check_file_match_expected_architecture "armv7hf")
        set(spec_regex "*applications*")
        set(check_content_list "^/usr/foo/bar
/usr/foo/bar/bin
/usr/foo/bar/bin/mylibapp(\n.*\.build-id.*)*$")
      elseif(check_file_Unspecified_match)
        set(check_file_match_expected_summary ".*${CPACK_RPM_PACKAGE_SUMMARY}.*")
        set(check_file_match_expected_description ".*DESCRIPTION.*")
        set(check_file_match_expected_relocation_path "Relocations${whitespaces}:${whitespaces}${CPACK_PACKAGING_INSTALL_PREFIX}${whitespaces}${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
        set(check_file_match_expected_architecture "") # we don't explicitly set this value so it is different on each platform - ignore it
        set(spec_regex "*Unspecified*")
        set(check_content_list "^/usr/foo/bar
/usr/foo/bar/bin
/usr/foo/bar/bin/@in@_@path@@with
/usr/foo/bar/bin/@in@_@path@@with/@and
/usr/foo/bar/bin/@in@_@path@@with/@and/@
/usr/foo/bar/bin/@in@_@path@@with/@and/@/@in_path@
/usr/foo/bar/bin/@in@_@path@@with/@and/@/@in_path@/mylibapp2
/usr/foo/bar/share
/usr/foo/bar/share/man
/usr/foo/bar/share/man/mylib
/usr/foo/bar/share/man/mylib/man3
/usr/foo/bar/share/man/mylib/man3/mylib.1
/usr/foo/bar/share/man/mylib/man3/mylib.1/mylib
/usr/foo/bar/share/man/mylib/man3/mylib.1/mylib2(\n.*\.build-id.*)*$")
      else()
        message(FATAL_ERROR "error: unexpected rpm package '${check_file}'")
      endif()

      #######################
      # test package info
      #######################
      string(REGEX MATCH ${check_file_match_expected_summary} check_file_match_summary ${check_file_content})

      if(NOT check_file_match_summary)
        message(FATAL_ERROR "error: '${check_file}' rpm package summary does not match expected value - regex '${check_file_match_expected_summary}'; RPM output: '${check_file_content}'")
      endif()

      string(REGEX MATCH ${check_file_match_expected_description} check_file_match_description ${check_file_content})

      if(NOT check_file_match_description)
        message(FATAL_ERROR "error: '${check_file}' rpm package description does not match expected value - regex '${check_file_match_expected_description}'; RPM output: '${check_file_content}'")
      endif()

      string(REGEX MATCH ${check_file_match_expected_relocation_path} check_file_match_relocation_path ${check_file_content})

      if(NOT check_file_match_relocation_path)
        file(GLOB_RECURSE spec_file "${CPackComponentsForAll_BINARY_DIR}/${spec_regex}.spec")

        if(spec_file)
          file(READ ${spec_file} spec_file_content)
        endif()

        message(FATAL_ERROR "error: '${check_file}' rpm package relocation path does not match expected value - regex '${check_file_match_expected_relocation_path}'; RPM output: '${check_file_content}'; generated spec file: '${spec_file_content}'")
      endif()

      #######################
      # test package architecture
      #######################
      string(REGEX MATCH "Architecture${whitespaces}:" check_info_contains_arch ${check_file_content})
      if(check_info_contains_arch) # test for rpm versions that contain architecture in package info (e.g. 4.11.x)
        string(REGEX MATCH "Architecture${whitespaces}:${whitespaces}${check_file_match_expected_architecture}" check_file_match_architecture ${check_file_content})
        if(NOT check_file_match_architecture)
          message(FATAL_ERROR "error: '${check_file}' Architecture does not match expected value - '${check_file_match_expected_architecture}'; RPM output: '${check_file_content}'; generated spec file: '${spec_file_content}'")
        endif()
      elseif(NOT check_package_architecture_result) # test result only on platforms that support -pqa rpm query
        # test for rpm versions that do not contain architecture in package info (e.g. 4.8.x)
        string(REGEX MATCH ".*${check_file_match_expected_architecture}" check_file_match_architecture "${check_package_architecture}")
        if(NOT check_file_match_architecture)
          message(FATAL_ERROR "error: '${check_file}' Architecture does not match expected value - '${check_file_match_expected_architecture}'; RPM output: '${check_package_architecture}'; generated spec file: '${spec_file_content}'")
        endif()
      # else rpm version too old (e.g. 4.4.x) - skip test
      endif()

      #######################
      # test package content
      #######################
      string(REGEX MATCH "${check_content_list}" expected_content_list "${check_package_content}")

      if(NOT expected_content_list)
        file(GLOB_RECURSE spec_file "${CPackComponentsForAll_BINARY_DIR}/${spec_regex}.spec")

        if(spec_file)
          file(READ ${spec_file} spec_file_content)
        endif()

        message(FATAL_ERROR "error: '${check_file}' rpm package content does not match expected value - regex '${check_content_list}'; RPM output: '${check_package_content}'; generated spec file: '${spec_file_content}'")
      endif()

      # validate permissions user and group
      execute_process(COMMAND ${RPM_EXECUTABLE} -pqlv ${check_file}
          OUTPUT_VARIABLE check_file_content
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE)

      if(check_file_libraries_match)
        set(check_file_match_expected_permissions ".*-rwx------.*user.*defgrp.*")
      elseif(check_file_headers_match)
        set(check_file_match_expected_permissions ".*-rwxr--r--.*defusr.*defgrp.*")
      elseif(check_file_applications_match)
        set(check_file_match_expected_permissions ".*-rwxr--r--.*defusr.*group.*")
      elseif(check_file_Unspecified_match)
        set(check_file_match_expected_permissions ".*-rwxr--r--.*defusr.*defgrp.*")
      else()
        message(FATAL_ERROR "error: unexpected rpm package '${check_file}'")
      endif()

      string(REGEX MATCH "${check_file_match_expected_permissions}" check_file_match_permissions "${check_file_content}")

      if(NOT check_file_match_permissions)
          message(FATAL_ERROR "error: '${check_file}' rpm package permissions do not match expected value - regex '${check_file_match_expected_permissions}'")
      endif()
    endforeach()

    #######################
    # verify generated symbolic links
    #######################
    file(GLOB_RECURSE symlink_files RELATIVE "${CPackComponentsForAll_BINARY_DIR}" "${CPackComponentsForAll_BINARY_DIR}/*/symlink_*")

    foreach(check_symlink IN LISTS symlink_files)
      get_filename_component(symlink_name "${check_symlink}" NAME)
      execute_process(COMMAND ls -la "${check_symlink}"
                WORKING_DIRECTORY "${CPackComponentsForAll_BINARY_DIR}"
                OUTPUT_VARIABLE SYMLINK_POINT_
                OUTPUT_STRIP_TRAILING_WHITESPACE)

      if("${symlink_name}" STREQUAL "symlink_samedir_path"
          OR "${symlink_name}" STREQUAL "symlink_samedir_path_current_dir"
          OR "${symlink_name}" STREQUAL "symlink_samedir_path_longer")
        string(REGEX MATCH "^.*${whitespaces}->${whitespaces}depth_three$" check_symlink "${SYMLINK_POINT_}")
      elseif("${symlink_name}" STREQUAL "symlink_subdir_path")
        string(REGEX MATCH "^.*${whitespaces}->${whitespaces}depth_two/depth_three$" check_symlink "${SYMLINK_POINT_}")
      elseif("${symlink_name}" STREQUAL "symlink_parentdir_path")
        string(REGEX MATCH "^.*${whitespaces}->${whitespaces}../$" check_symlink "${SYMLINK_POINT_}")
      elseif("${symlink_name}" STREQUAL "symlink_to_non_relocatable_path")
        string(REGEX MATCH "^.*${whitespaces}->${whitespaces}${CPACK_PACKAGING_INSTALL_PREFIX}/non_relocatable/depth_two$" check_symlink "${SYMLINK_POINT_}")
      elseif("${symlink_name}" STREQUAL "symlink_outside_package")
        string(REGEX MATCH "^.*${whitespaces}->${whitespaces}outside_package$" check_symlink "${SYMLINK_POINT_}")
      elseif("${symlink_name}" STREQUAL "symlink_outside_wdr")
        string(REGEX MATCH "^.*${whitespaces}->${whitespaces}/outside_package_wdr$" check_symlink "${SYMLINK_POINT_}")
      elseif("${symlink_name}" STREQUAL "symlink_other_relocatable_path"
          OR "${symlink_name}" STREQUAL "symlink_from_non_relocatable_path"
          OR "${symlink_name}" STREQUAL "symlink_relocatable_subpath")
        # these links were not canged - post install script only - ignore them
      else()
        message(FATAL_ERROR "error: unexpected rpm symbolic link '${check_symlink}'")
      endif()

      if(NOT check_symlink)
        message(FATAL_ERROR "symlink points to unexpected location '${SYMLINK_POINT_}'")
      endif()
    endforeach()

    # verify post install symlink relocation script
    file(GLOB_RECURSE spec_file "${CPackComponentsForAll_BINARY_DIR}/*libraries*.spec")
    file(READ ${spec_file} spec_file_content)
    file(READ "${CMAKE_CURRENT_LIST_DIR}/symlink_postinstall_expected.txt" symlink_postinstall_expected)
    # prepare regex
    string(STRIP "${symlink_postinstall_expected}" symlink_postinstall_expected)
    string(REPLACE "[" "\\[" symlink_postinstall_expected "${symlink_postinstall_expected}")
    string(REPLACE "$" "\\$" symlink_postinstall_expected "${symlink_postinstall_expected}")
    string(REPLACE "lib" "lib${LIB_SUFFIX}" symlink_postinstall_expected "${symlink_postinstall_expected}")
    # compare
    string(REGEX MATCH ".*${symlink_postinstall_expected}.*" symlink_postinstall_expected_matches "${spec_file_content}")
    if(NOT symlink_postinstall_expected_matches)
      message(FATAL_ERROR "error: unexpected rpm symbolic link postinstall script! generated spec file: '${spec_file_content}'")
    endif()
  endif()
endif()

cmake_policy(POP)
