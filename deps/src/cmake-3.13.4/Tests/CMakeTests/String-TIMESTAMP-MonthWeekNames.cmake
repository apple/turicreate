string(TIMESTAMP output "%a;%b")
message("~${output}~")

list(LENGTH output output_length)

set(expected_output_length 2)

if(NOT output_length EQUAL ${expected_output_length})
    message(FATAL_ERROR "expected ${expected_output_length} entries in output "
        "with all specifiers; found ${output_length}")
endif()
