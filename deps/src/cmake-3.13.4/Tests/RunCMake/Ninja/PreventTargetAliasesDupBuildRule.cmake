cmake_minimum_required(VERSION 3.12)
project(Test LANGUAGES C)

# fake launcher executable
set(input_launcher_executable ${CMAKE_CURRENT_BINARY_DIR}/fake_launcher_executable)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/fake_launcher_executable "")

# application and executable name
set(application_target "HelloApp")
set(application_name "Hello")
set(executable_name "Hello")

# target built in "<root>/bin"
add_executable(${application_target} hello.c)
set_target_properties(${application_target} PROPERTIES
  OUTPUT_NAME ${executable_name}
  RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}/bin
  )

# configured launcher in "<root>"
set(configured_launcher_executable "${CMAKE_CURRENT_BINARY_DIR}/${application_name}")

# create command to copy the launcher
add_custom_command(
  DEPENDS
    ${input_launcher_executable}
  OUTPUT
    ${configured_launcher_executable}
  COMMAND
    ${CMAKE_COMMAND} -E copy ${input_launcher_executable} ${configured_launcher_executable}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT
    "Configuring application launcher: ${application_name}"
  )

add_custom_target(Configure${application_name}Launcher ALL
  DEPENDS
    ${application_target}
    ${input_launcher_executable}
    ${configured_launcher_executable}
  )
