enable_language(CSharp)
add_library(foo foo.cs)
add_library(foo2 foo.cs)

set(test1Reference "System")
set(test1Tag "Hello")
set(test1Value "World")

set(test2Reference "foo2")
set(test2Tag "Hallo")
set(test2Value "Welt")

target_link_libraries(foo foo2)

set_target_properties(foo PROPERTIES
  VS_DOTNET_REFERENCES "${test1Reference};Blubb"
  VS_DOTNET_REFERENCEPROP_${test1Reference}_TAG_${test1Tag} ${test1Value}
  VS_DOTNET_REFERENCEPROP_${test2Reference}_TAG_${test2Tag} ${test2Value}
  )
