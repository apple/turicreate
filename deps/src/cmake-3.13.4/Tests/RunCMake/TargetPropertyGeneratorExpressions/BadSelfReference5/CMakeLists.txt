add_executable(BadSelfReference5 ../main.cpp)
set_property(TARGET BadSelfReference5 PROPERTY COMPILE_DEFINITIONS "$<TARGET_PROPERTY:COMPILE_DEFINITIONS>")
