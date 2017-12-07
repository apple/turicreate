
cmake_policy(SET CMP0026 OLD)

add_library(objlib OBJECT
    empty_1.cpp
)

add_executable(my_exe
    empty_2.cpp
    $<TARGET_OBJECTS:objlib>
)

get_target_property( loc my_exe LOCATION)
