enable_language(CSharp)
add_library(foo foo.cs)

set(props_file "${CMAKE_CURRENT_SOURCE_DIR}/my.props")

set(tagName "MyCustomTag")
set(tagValue "MyCustomValue")

set_source_files_properties(foo.cs
  PROPERTIES
  VS_CSHARP_${tagName} "${tagValue}")
