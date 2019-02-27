add_executable(BadSelfReference6 ../main.cpp)
set_property(TARGET BadSelfReference6 PROPERTY COMPILE_DEFINITIONS "$<TARGET_PROPERTY:BadSelfReference6,COMPILE_DEFINITIONS>")
