if(NOT CPackComponentsDEB_SOURCE_DIR)
  message(FATAL_ERROR "CPackComponentsDEB_SOURCE_DIR not set")
endif()

include(${CPackComponentsDEB_SOURCE_DIR}/RunCPackVerifyResult.cmake)



# requirements

# debian now produces lower case names
set(expected_file_mask "${CPackComponentsDEB_BINARY_DIR}/mylib-*_1.0.2_*.deb")
set(expected_count 3)


set(actual_output)
run_cpack(actual_output
          CPack_output
          CPack_error
          EXPECTED_FILE_MASK "${expected_file_mask}"
          CONFIG_ARGS ${config_args}
          CONFIG_VERBOSE ${config_verbose})

message(STATUS "expected_count='${expected_count}'")
message(STATUS "expected_file_mask='${expected_file_mask}'")
message(STATUS "actual_output_files='${actual_output}'")

if(NOT actual_output)
  message(FATAL_ERROR "error: expected_files do not exist: CPackComponentsDEB test fails. (CPack_output=${CPack_output}, CPack_error=${CPack_error}")
endif()

list(LENGTH actual_output actual_count)
message(STATUS "actual_count='${actual_count}'")
if(NOT actual_count EQUAL expected_count)
  message(FATAL_ERROR "error: expected_count=${expected_count} does not match actual_count=${actual_count}: CPackComponents test fails. (CPack_output=${CPack_output}, CPack_error=${CPack_error})")
endif()


# dpkg-deb checks for the summary of the packages
find_program(DPKGDEB_EXECUTABLE dpkg-deb)
if(DPKGDEB_EXECUTABLE)
  set(dpkgdeb_output_errors_all)
  foreach(_f IN LISTS actual_output)

    # extracts the metadata from the package
    run_dpkgdeb(dpkg_output
                FILENAME ${_f}
                )

    dpkgdeb_return_specific_metaentry(dpkg_package_name
                                      DPKGDEB_OUTPUT "${dpkg_output}"
                                      METAENTRY "Package:")

    message(STATUS "package='${dpkg_package_name}'")

    if("${dpkg_package_name}" STREQUAL "mylib-applications")
      # pass
    elseif("${dpkg_package_name}" STREQUAL "mylib-headers")
      # pass
    elseif("${dpkg_package_name}" STREQUAL "mylib-libraries")
      # pass
    else()
      set(dpkgdeb_output_errors_all ${dpkgdeb_output_errors_all}
                                    "dpkg-deb: ${_f}: component name not found: ${dpkg_package_name}\n")
    endif()

  endforeach()


  if(NOT "${dpkgdeb_output_errors_all}" STREQUAL "")
    message(FATAL_ERROR "dpkg-deb checks failed:\n${dpkgdeb_output_errors_all}")
  endif()
else()
  message("dpkg-deb executable not found - skipping dpkg-deb test")
endif()
