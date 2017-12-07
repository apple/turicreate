# a simple CSharp only test case
project (CSharpOnly CSharp)

# C# does not make any difference between STATIC and SHARED libs
add_library(lib1 STATIC lib1.cs)
add_library(lib2 SHARED lib2.cs)

add_executable(CSharpOnly csharponly.cs)

target_link_libraries(CSharpOnly lib1 lib2)
