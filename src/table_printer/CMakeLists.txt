project(table_printer)

add_library(table_printer OBJECT
  table_printer.cpp
  table_element_printers.cpp
)
add_dependencies(table_printer boost)

make_executable(table_printer_example
  SOURCES
    table_printer_examples.cpp
  REQUIRES
    unity_shared
)
