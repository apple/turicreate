add_executable(BadSelfReference4 ../main.cpp)
set_property(TARGET BadSelfReference4 PROPERTY INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:BadSelfReference4,INCLUDE_DIRECTORIES>")
