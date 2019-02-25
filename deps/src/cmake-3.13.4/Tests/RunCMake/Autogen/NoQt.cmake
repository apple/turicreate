enable_language(CXX)

add_executable(main empty.cpp)
set_property(TARGET main PROPERTY AUTOMOC 1)
set_property(TARGET main PROPERTY AUTORCC 1)
set_property(TARGET main PROPERTY AUTOUIC 1)
