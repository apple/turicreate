# Set the ExternalProject GIT_TAG to desired_tag, and make sure the
# resulting checked out version is resulting_sha and rebuild.
# This check's the correct behavior of the ExternalProject UPDATE_COMMAND.
# Also verify that a fetch only occurs when fetch_expected is 1.
macro(check_a_tag desired_tag resulting_sha fetch_expected)
  message( STATUS "Checking ExternalProjectUpdate to tag: ${desired_tag}" )

  # Remove the FETCH_HEAD file, so we can check if it gets replaced with a 'git
  # fetch'.
  set( FETCH_HEAD_file ${ExternalProjectUpdate_BINARY_DIR}/CMakeExternals/Source/TutorialStep1-GIT/.git/FETCH_HEAD )
  file( REMOVE ${FETCH_HEAD_file} )

  # Configure
  execute_process(COMMAND ${CMAKE_COMMAND}
    -G ${CMAKE_GENERATOR} -T "${CMAKE_GENERATOR_TOOLSET}"
    -A "${CMAKE_GENERATOR_PLATFORM}"
    -DTEST_GIT_TAG:STRING=${desired_tag}
    ${ExternalProjectUpdate_SOURCE_DIR}
    WORKING_DIRECTORY ${ExternalProjectUpdate_BINARY_DIR}
    RESULT_VARIABLE error_code
    )
  if(error_code)
    message(FATAL_ERROR "Could not configure the project.")
  endif()

  # Build
  execute_process(COMMAND ${CMAKE_COMMAND}
    --build ${ExternalProjectUpdate_BINARY_DIR}
    RESULT_VARIABLE error_code
    )
  if(error_code)
    message(FATAL_ERROR "Could not build the project.")
  endif()

  # Check the resulting SHA
  execute_process(COMMAND ${GIT_EXECUTABLE}
    rev-list --max-count=1 HEAD
    WORKING_DIRECTORY ${ExternalProjectUpdate_BINARY_DIR}/CMakeExternals/Source/TutorialStep1-GIT
    RESULT_VARIABLE error_code
    OUTPUT_VARIABLE tag_sha
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if(error_code)
    message(FATAL_ERROR "Could not check the sha.")
  endif()

  if(NOT (${tag_sha} STREQUAL ${resulting_sha}))
    message(FATAL_ERROR "UPDATE_COMMAND produced
  ${tag_sha}
when
  ${resulting_sha}
was expected."
    )
  endif()

  if( NOT EXISTS ${FETCH_HEAD_file} AND ${fetch_expected})
    message( FATAL_ERROR "Fetch did NOT occur when it was expected.")
  endif()
  if( EXISTS ${FETCH_HEAD_file} AND NOT ${fetch_expected})
    message( FATAL_ERROR "Fetch DID occur when it was not expected.")
  endif()

  message( STATUS "Checking ExternalProjectUpdate to tag: ${desired_tag} (disconnected)" )

  # Remove the FETCH_HEAD file, so we can check if it gets replaced with a 'git
  # fetch'.
  set( FETCH_HEAD_file ${ExternalProjectUpdate_BINARY_DIR}/CMakeExternals/Source/TutorialStep2-GIT/.git/FETCH_HEAD )
  file( REMOVE ${FETCH_HEAD_file} )

  # Check initial SHA
  execute_process(COMMAND ${GIT_EXECUTABLE}
    rev-list --max-count=1 HEAD
    WORKING_DIRECTORY ${ExternalProjectUpdate_BINARY_DIR}/CMakeExternals/Source/TutorialStep2-GIT
    RESULT_VARIABLE error_code
    OUTPUT_VARIABLE initial_sha
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  # Configure
  execute_process(COMMAND ${CMAKE_COMMAND}
    -G ${CMAKE_GENERATOR} -T "${CMAKE_GENERATOR_TOOLSET}"
    -A "${CMAKE_GENERATOR_PLATFORM}"
    -DTEST_GIT_TAG:STRING=${desired_tag}
    ${ExternalProjectUpdate_SOURCE_DIR}
    WORKING_DIRECTORY ${ExternalProjectUpdate_BINARY_DIR}
    RESULT_VARIABLE error_code
    )
  if(error_code)
    message(FATAL_ERROR "Could not configure the project.")
  endif()

  # Build
  execute_process(COMMAND ${CMAKE_COMMAND}
    --build ${ExternalProjectUpdate_BINARY_DIR}
    RESULT_VARIABLE error_code
    )
  if(error_code)
    message(FATAL_ERROR "Could not build the project.")
  endif()

  if( EXISTS ${FETCH_HEAD_file} )
    message( FATAL_ERROR "Fetch occurred when it was not expected.")
  endif()

  # Check the resulting SHA
  execute_process(COMMAND ${GIT_EXECUTABLE}
    rev-list --max-count=1 HEAD
    WORKING_DIRECTORY ${ExternalProjectUpdate_BINARY_DIR}/CMakeExternals/Source/TutorialStep2-GIT
    RESULT_VARIABLE error_code
    OUTPUT_VARIABLE tag_sha
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if(error_code)
    message(FATAL_ERROR "Could not check the sha.")
  endif()

  if(NOT (${tag_sha} STREQUAL ${initial_sha}))
    message(FATAL_ERROR "Update occurred when it was not expected.")
  endif()

  # Update
  execute_process(COMMAND ${CMAKE_COMMAND}
    --build ${ExternalProjectUpdate_BINARY_DIR}
    --target TutorialStep2-GIT-update
    RESULT_VARIABLE error_code
    )
  if(error_code)
    message(FATAL_ERROR "Could not build the project.")
  endif()

  # Check the resulting SHA
  execute_process(COMMAND ${GIT_EXECUTABLE}
    rev-list --max-count=1 HEAD
    WORKING_DIRECTORY ${ExternalProjectUpdate_BINARY_DIR}/CMakeExternals/Source/TutorialStep2-GIT
    RESULT_VARIABLE error_code
    OUTPUT_VARIABLE tag_sha
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if(error_code)
    message(FATAL_ERROR "Could not check the sha.")
  endif()

  if(NOT (${tag_sha} STREQUAL ${resulting_sha}))
    message(FATAL_ERROR "UPDATE_COMMAND produced
  ${tag_sha}
when
  ${resulting_sha}
was expected."
    )
  endif()

  if( NOT EXISTS ${FETCH_HEAD_file} AND ${fetch_expected})
    message( FATAL_ERROR "Fetch did NOT occur when it was expected.")
  endif()
  if( EXISTS ${FETCH_HEAD_file} AND NOT ${fetch_expected})
    message( FATAL_ERROR "Fetch DID occur when it was not expected.")
  endif()
endmacro()

find_package(Git)
set(do_git_tests 0)
if(GIT_EXECUTABLE)
  set(do_git_tests 1)

  execute_process(
    COMMAND "${GIT_EXECUTABLE}" --version
    OUTPUT_VARIABLE ov
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  string(REGEX REPLACE "^git version (.+)$" "\\1" git_version "${ov}")
  message(STATUS "git_version='${git_version}'")

  if(git_version VERSION_LESS 1.6.5)
    message(STATUS "No ExternalProject git tests with git client less than version 1.6.5")
    set(do_git_tests 0)
  endif()
endif()

if(do_git_tests)
  check_a_tag(origin/master 5842b503ba4113976d9bb28d57b5aee1ad2736b7 1)
  check_a_tag(tag1          d1970730310fe8bc07e73f15dc570071f9f9654a 1)
  # With the Git UPDATE_COMMAND performance patch, this will not required a
  # 'git fetch'
  check_a_tag(tag1          d1970730310fe8bc07e73f15dc570071f9f9654a 0)
  check_a_tag(tag2          5842b503ba4113976d9bb28d57b5aee1ad2736b7 1)
  check_a_tag(d19707303     d1970730310fe8bc07e73f15dc570071f9f9654a 1)
  check_a_tag(d19707303     d1970730310fe8bc07e73f15dc570071f9f9654a 0)
  check_a_tag(origin/master 5842b503ba4113976d9bb28d57b5aee1ad2736b7 1)
  # This is a remote symbolic ref, so it will always trigger a 'git fetch'
  check_a_tag(origin/master 5842b503ba4113976d9bb28d57b5aee1ad2736b7 1)
endif()
