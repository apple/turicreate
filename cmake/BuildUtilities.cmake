# This is an external function
# Usage:
#    make_copy_target(target
#                TARGETS [list of targets]
#                FILES [list of files (absolute path)]
#                DIRECTORIES [list of directories (absolute path)
# Example:
# make_copy_target(NAME target
#                TARGETS a b
#                FILES ${CMAKE_SOURCE_DIR}/pika/a.txt
#             )
#
# This copies all files produced by targets in TARGETS to the output binary
# directory as well as all files in FILES.
#
# TARGETS may reference an existing copy_target in which case all files copied
# by the copy target will also be copied.
#
# For instance:
# make_copy_target(NAME spark_pipe_wrapper
#                FILES ${CMAKE_CURRENT_SOURCE_DIR}/spark_pipe_wrapper.py)
#
# Then in some other location
#
# make_copy_target(NAME release_binaries
#                TARGETS spark_pipe_wrapper)
#
# Warning: The recursive reference is slightly brittle since it requires the
# referenced target to exist prior to referencing it.
# This is potentially fixable with complicated generator expressions. but... urgh.
macro (make_copy_target NAME)
  set(multi_value_args TARGETS FILES DIRECTORIES)
  CMAKE_PARSE_ARGUMENTS(copy "" "" "${multi_value_args}" ${ARGN})
  ADD_CUSTOM_TARGET(${NAME} ALL)
  set(all_target_locations)
  set(RSYNC_SOURCES ${copy_FILES} ${copy_DIRECTORIES})
  find_program(RSYNC rsync)
  if("${RSYNC}" STREQUAL "")
    # No rsync present; copy the old fashioned way with multiple cp commands
    foreach(binary ${copy_FILES})
            # Switched away from cmake -E commands because they don't work right
            # on Windows. Also Windows doesn't overwrite when copying by default
            if(WIN32)
              add_custom_command(TARGET ${NAME} POST_BUILD
                COMMENT "remove old ${binary}"
                COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/`basename ${binary}` )
            endif()
            add_custom_command(TARGET ${NAME} POST_BUILD
                    COMMENT "copy ${binary}"
                    COMMAND cp ${binary} ${CMAKE_CURRENT_BINARY_DIR})
    endforeach()

    foreach(binary ${copy_DIRECTORIES})
            # Switched away from cmake -E commands because they don't work right
            # on Windows. Also Windows doesn't overwrite when copying by default
            add_custom_command(TARGET ${NAME} POST_BUILD
                    COMMENT "remove old ${binary}"
                    COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/`basename ${binary}` )
            add_custom_command(TARGET ${NAME} POST_BUILD
                    COMMENT "copy ${binary}"
                    COMMAND cp -r ${binary} ${CMAKE_CURRENT_BINARY_DIR})
    endforeach()
  else()
    if((NOT "${copy_FILES}" STREQUAL "") OR (NOT "${copy_DIRECTORIES}" STREQUAL ""))
      add_custom_command(TARGET ${NAME} POST_BUILD
        COMMENT "copying ${NAME}"
        COMMAND ${RSYNC} -a ${RSYNC_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}
      )
    endif()
  endif()

  # Make sure the files and directories get included in all_target_locations
  foreach(binary ${copy_FILES})
          list(APPEND all_target_locations ${binary})
  endforeach()
  foreach(binary ${copy_DIRECTORIES})
          list(APPEND all_target_locations ${binary})
  endforeach()

  if (NOT "${copy_TARGETS}" STREQUAL "")
    add_dependencies(${NAME} ${copy_TARGETS})
  endif()

  foreach(target ${copy_TARGETS})
          if (TARGET ${target})
                  get_property(CUSTOM_LOCATION TARGET ${target} PROPERTY COPY_FILES)
          else()
                  set(${CUSTOM_LOCATION} "")
          endif()
          if (NOT "${CUSTOM_LOCATION}" STREQUAL "")
                  foreach(location ${CUSTOM_LOCATION})
                          add_custom_command(TARGET ${NAME} POST_BUILD
                                  COMMENT "copy ${location}"
                                  DEPENDS ${target}
                                  COMMAND ${CMAKE_COMMAND} -E
                                  copy_if_different ${location} ${CMAKE_CURRENT_BINARY_DIR})
                          list(APPEND all_target_locations ${location})
                  endforeach()
          else()
                  add_custom_command(TARGET ${NAME} POST_BUILD
                          COMMENT "copying target of ${target}"
                          DEPENDS ${target}
                          COMMAND ${CMAKE_COMMAND} -E
                          copy_if_different $<TARGET_FILE:${target}> ${CMAKE_CURRENT_BINARY_DIR})
                  list(APPEND all_target_locations $<TARGET_FILE:${target}>)
          endif()
  endforeach()
  message(STATUS "Adding Copy Target ${NAME} is copying:  ${all_target_locations}")
  set_property(TARGET ${NAME} PROPERTY COPY_FILES "${all_target_locations}")
endmacro()

