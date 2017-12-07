set(NUMBERS "")
set(COUNT 0)

while(COUNT LESS 200)
    string(APPEND NUMBERS " ${COUNT}")
    set(COUNT "2${COUNT}")

    set(NCOUNT 3)
    while(NCOUNT LESS 31)
        string(APPEND NUMBERS " ${NCOUNT}")
        string(APPEND NCOUNT "0")
    endwhile()
endwhile()

if(NOT NUMBERS STREQUAL " 0 3 30 20 3 30")
    message(SEND_ERROR "while loop nesting error, result: '${NUMBERS}'")
endif()
