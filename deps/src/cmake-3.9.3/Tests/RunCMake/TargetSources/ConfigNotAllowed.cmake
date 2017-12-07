
add_library(somelib empty_1.cpp $<$<CONFIG:Debug>:empty_2.cpp>)
