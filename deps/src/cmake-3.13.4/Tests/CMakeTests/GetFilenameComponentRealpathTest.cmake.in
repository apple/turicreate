set(bindir ${CMAKE_CURRENT_BINARY_DIR})

#
# Test nonexistent REALPATH & ABSOLUTE resolution
#
get_filename_component(nonexistent1 ${bindir}/THIS_IS_A_NONEXISTENT_FILE REALPATH)
get_filename_component(nonexistent2 ${bindir}/THIS_IS_A_NONEXISTENT_FILE ABSOLUTE)
if(NOT nonexistent1 STREQUAL "${bindir}/THIS_IS_A_NONEXISTENT_FILE")
    message(FATAL_ERROR "REALPATH is not preserving nonexistent files")
endif()
if(NOT nonexistent2 STREQUAL "${bindir}/THIS_IS_A_NONEXISTENT_FILE")
    message(FATAL_ERROR "ABSOLUTE is not preserving nonexistent files")
endif()

#
# Test treatment of relative paths
#
foreach(c REALPATH ABSOLUTE)
  get_filename_component(dir "subdir/THIS_IS_A_NONEXISTENT_FILE" ${c})
  if(NOT "${dir}" STREQUAL "${bindir}/subdir/THIS_IS_A_NONEXISTENT_FILE")
    message(FATAL_ERROR
      "${c} does not handle relative paths.  Expected:\n"
      "  ${bindir}/subdir/THIS_IS_A_NONEXISTENT_FILE\n"
      "but got:\n"
      "  ${nonexistent1}\n"
      )
  endif()
endforeach()

#
# Test symbolic link resolution
#
if(UNIX)
    # file1 => file2 => file3 (real)
    file(WRITE ${bindir}/file3 "test file")

    find_program(LN NAMES "ln")
    if(LN)
        # Create symlinks using "ln -s"
        if(NOT EXISTS ${bindir}/file2)
            execute_process(COMMAND ${LN} "-s" "${bindir}/file3" "${bindir}/file2")
        endif()
        if(NOT EXISTS ${bindir}/file1)
            execute_process(COMMAND ${LN} "-s" "${bindir}/file2" "${bindir}/file1")
        endif()

        get_filename_component(file1 ${bindir}/file1 REALPATH)
        get_filename_component(file2 ${bindir}/file2 REALPATH)
        get_filename_component(file3 ${bindir}/file3 REALPATH)

        if(NOT file3 STREQUAL "${bindir}/file3")
            message(FATAL_ERROR "CMake fails resolving REALPATH file file3")
        endif()

        if(NOT file2 STREQUAL "${bindir}/file3")
            message(FATAL_ERROR "CMake fails resolving simple symlink")
        endif()

        if(NOT file1 STREQUAL "${bindir}/file3")
            message(FATAL_ERROR "CMake fails resolving double symlink")
        endif()

        # cleanup
        file(REMOVE ${bindir}/file1)
        file(REMOVE ${bindir}/file2)
        if(EXISTS file1 OR EXISTS file2)
           message(FATAL_ERROR "removal of file1 or file2 failed")
        endif()
    endif()

    file(REMOVE ${bindir}/file3)
endif()
