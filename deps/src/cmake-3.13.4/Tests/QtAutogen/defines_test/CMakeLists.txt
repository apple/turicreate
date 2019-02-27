
add_executable(defines_test defines_test.cpp)
set_target_properties(defines_test PROPERTIES AUTOMOC TRUE)
target_link_libraries(defines_test Qt4::QtGui)
