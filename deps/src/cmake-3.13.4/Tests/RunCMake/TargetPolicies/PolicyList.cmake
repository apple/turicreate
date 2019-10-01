
enable_language(CXX)

add_library(empty empty.cpp)
target_compile_definitions(empty
  PRIVATE
    $<$<TARGET_POLICY:NOT_A_POLICY>:SOME_DEFINE>
)
