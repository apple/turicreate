set(file_name  ${CMAKE_CURRENT_BINARY_DIR}/NewLineStyle.txt)
file(WRITE ${file_name} "Data\n")
configure_file(${file_name} ${file_name}.out COPYONLY NEWLINE_STYLE DOS)
