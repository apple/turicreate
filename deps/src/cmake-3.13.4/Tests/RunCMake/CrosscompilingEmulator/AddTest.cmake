set(CMAKE_CROSSCOMPILING 1)
enable_testing()
add_test(NAME DoesNotUseEmulator
  COMMAND ${CMAKE_COMMAND} -E echo "Hi")

add_executable(generated_exe simple_src_exiterror.cxx)

add_test(NAME UsesEmulator
  COMMAND generated_exe)

add_test(NAME DoesNotUseEmulatorWithGenex
  COMMAND $<TARGET_FILE:generated_exe>)

add_subdirectory(AddTest)

add_test(NAME UsesEmulatorWithExecTargetFromSubdirAddedWithoutGenex
  COMMAND generated_exe_in_subdir_added_to_test_without_genex)

add_test(NAME DoesNotUseEmulatorWithExecTargetFromSubdirAddedWithGenex
  COMMAND $<TARGET_FILE:generated_exe_in_subdir_added_to_test_with_genex>)
