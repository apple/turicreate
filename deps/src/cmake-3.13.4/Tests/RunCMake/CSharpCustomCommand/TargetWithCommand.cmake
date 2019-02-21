enable_language(CSharp)

add_custom_target(drive ALL SOURCES dummy.cs
  COMMAND ${CMAKE_COMMAND} -E echo "Custom target with CSharp source")
