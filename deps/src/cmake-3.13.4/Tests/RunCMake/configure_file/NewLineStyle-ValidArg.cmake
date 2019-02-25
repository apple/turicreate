set(file_name  ${CMAKE_CURRENT_BINARY_DIR}/NewLineStyle.txt)

function(test_eol style in out)
    file(WRITE ${file_name} "${in}")
    configure_file(${file_name} ${file_name}.out NEWLINE_STYLE ${style})
    file(READ ${file_name}.out new HEX)
    if(NOT "${new}" STREQUAL "${out}")
        message(FATAL_ERROR "No ${style} line endings")
    endif()
endfunction()

test_eol(DOS   "a\n" "610d0a")
test_eol(WIN32 "b\n" "620d0a")
test_eol(CRLF  "c\n" "630d0a")

test_eol(UNIX  "d\n" "640a")
test_eol(LF    "e\n" "650a")
