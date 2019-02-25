enable_language(CSharp)

add_executable(CSharpCustomCommand dummy.cs)

add_custom_command(OUTPUT ${generatedFileName}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${inputFileName} ${generatedFileName}
  MAIN_DEPENDENCY ${inputFileName}
  COMMENT "${commandComment}")

target_sources(CSharpCustomCommand PRIVATE
  ${inputFileName}
  ${generatedFileName})
