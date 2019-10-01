# Add rules to check the generated executable works.

set(hello_output "${CMAKE_CURRENT_BINARY_DIR}/hello.output")
add_custom_command(
  OUTPUT "${hello_output}"
  COMMAND "$<TARGET_FILE:hello>" > "${hello_output}"
  DEPENDS hello
  VERBATIM
  )

if(NOT DEFINED HELLO_OUTPUT_STRING)
  set(HELLO_OUTPUT_STRING "Hello world!\n")
endif()

set(hello_output_ref "${CMAKE_CURRENT_BINARY_DIR}/hello.output.ref")
file(WRITE "${hello_output_ref}" "${HELLO_OUTPUT_STRING}")

add_custom_target(check_output ALL
  COMMAND "${CMAKE_COMMAND}" -E compare_files
          "${hello_output}" "${hello_output_ref}"
  DEPENDS "${hello_output}" "${hello_output_ref}"
  VERBATIM
  )
