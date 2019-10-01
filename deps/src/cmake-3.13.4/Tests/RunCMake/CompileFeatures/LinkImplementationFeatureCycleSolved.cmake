
add_library(empty1 empty.cpp)

add_library(empty2 INTERFACE)
add_library(empty3 INTERFACE)
target_compile_features(empty3 INTERFACE cxx_std_11)

target_link_libraries(empty1
  $<$<COMPILE_FEATURES:cxx_nullptr>:empty2>
  empty3
)
# This, or populating the COMPILE_FEATURES property with a feature in the
# same standard as cxx_nullptr, solves the cycle above.
set_property(TARGET empty1 PROPERTY CXX_STANDARD 11)
