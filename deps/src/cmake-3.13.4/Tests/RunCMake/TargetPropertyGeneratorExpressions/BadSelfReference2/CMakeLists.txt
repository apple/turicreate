add_executable(BadSelfReference2 ../main.cpp)
set_property(TARGET BadSelfReference2 PROPERTY INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:INCLUDE_DIRECTORIES>")
