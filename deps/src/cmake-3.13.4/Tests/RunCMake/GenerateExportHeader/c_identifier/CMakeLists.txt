set(c_identifier_lib_SRCS
  c_identifier_class.cpp
)

add_library(7c-identifier-lib++ SHARED c_identifier_class.cpp)

generate_export_header(7c-identifier-lib++)

add_executable(c_identifier_exe main.cpp)

target_link_libraries(c_identifier_exe 7c-identifier-lib++)
