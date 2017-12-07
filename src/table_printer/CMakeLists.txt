project(table_printer)

make_library(table_printer
  SOURCES
    table_printer.cpp
    table_element_printers.cpp
  REQUIRES
    sframe logger 
    EXTERNAL_VISIBILITY
  )

make_executable(table_printer_example
  SOURCES table_printer_examples.cpp
  REQUIRES table_printer random)
