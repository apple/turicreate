enable_language(C)

add_executable(main main.c ${CMAKE_CURRENT_BINARY_DIR}/test.manifest)

if(MSVC AND NOT MSVC_VERSION LESS 1400)
  set(EXTRA_CHECK [[
file(STRINGS "$<TARGET_FILE:main>" content REGEX "name=\"Kitware.CMake.C-Exe-Manifest-step[0-9]\"")
if(NOT "${content}" MATCHES "name=\"Kitware.CMake.C-Exe-Manifest-step${check_step}\"")
  set(RunCMake_TEST_FAILED "Binary has no manifest with name=\"Kitware.CMake.C-Exe-Manifest-step${check_step}\":\n ${content}")
endif()
]])
endif()

file(GENERATE OUTPUT check-$<LOWER_CASE:$<CONFIG>>.cmake CONTENT "
set(check_pairs
  \"$<TARGET_FILE:main>|${CMAKE_CURRENT_BINARY_DIR}/test.manifest\"
  )
${EXTRA_CHECK}
")
