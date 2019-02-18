set(CMAKE_CROSSCOMPILING 1)

# Executable: Return error code different from 0
add_executable(generated_exe_emulator_expected simple_src_exiterror.cxx)

# Executable: Return error code equal to 0
add_executable(generated_exe_emulator_unexpected generated_exe_emulator_unexpected.cxx)
# Place the executable in a predictable location.
set_property(TARGET generated_exe_emulator_unexpected PROPERTY RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_BINARY_DIR}>)

# Executable: Imported version of above.  Fake the imported target to use the above.
add_executable(generated_exe_emulator_unexpected_imported IMPORTED)
set_property(TARGET generated_exe_emulator_unexpected_imported PROPERTY IMPORTED_LOCATION
  "${CMAKE_CURRENT_BINARY_DIR}/generated_exe_emulator_unexpected${CMAKE_EXECUTABLE_SUFFIX}")
add_dependencies(generated_exe_emulator_unexpected_imported generated_exe_emulator_unexpected)

# DoesNotUseEmulator
add_custom_target(generate_output1 ALL
  ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/output1)

# DoesNotUseEmulator: The command will fail if emulator is prepended
add_custom_target(generate_output2 ALL
  ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:generated_exe_emulator_unexpected>"
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/output2
  DEPENDS generated_exe_emulator_unexpected)

# DoesNotUseEmulator: The command will fail if emulator is prepended
add_custom_target(generate_output3 ALL
  $<TARGET_FILE:generated_exe_emulator_unexpected>
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/output3
  DEPENDS generated_exe_emulator_unexpected)

# DoesNotUseEmulator: The command will fail if emulator is prepended
add_custom_target(generate_outputImp ALL
  COMMAND generated_exe_emulator_unexpected_imported
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/outputImp
  )

# UsesEmulator: The command only succeeds if the emulator is prepended
#               to the command.
add_custom_target(generate_output4 ALL
  generated_exe_emulator_expected
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/output4
  DEPENDS generated_exe_emulator_expected)
