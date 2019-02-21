set(source_unsorted
        c/B.h
        a/c.h
        B/a.h
        )

## Test with default options
set(expected
        B/a.h
        a/c.h
        c/B.h
        )
set(list ${source_unsorted})
list(SORT list)
if (NOT expected STREQUAL list)
    message(FATAL_ERROR "wrong sort result with command list(SORT list CASE SENSITIVE ORDER ASCENDING COMPARE STRING)")
endif ()


## Test CASE INSENSITIVE ORDER ASCENDING COMPARE STRING
set(expected
        a/c.h
        B/a.h
        c/B.h
        )
set(list ${source_unsorted})
list(SORT list CASE INSENSITIVE ORDER ASCENDING COMPARE STRING)
if (NOT expected STREQUAL list)
    message(FATAL_ERROR "wrong sort result with command list(SORT list CASE INSENSITIVE ORDER ASCENDING COMPARE STRING)")
endif ()

## Test CASE INSENSITIVE ORDER DESCENDING COMPARE STRING
set(expected
        c/B.h
        B/a.h
        a/c.h
        )
set(list ${source_unsorted})
list(SORT list CASE INSENSITIVE ORDER DESCENDING COMPARE STRING)
if (NOT expected STREQUAL list)
    message(FATAL_ERROR "wrong sort result with command list(SORT list CASE INSENSITIVE ORDER DESCENDING COMPARE STRING)")
endif ()

## Test CASE SENSITIVE ORDER ASCENDING COMPARE STRING
set(expected
        B/a.h
        a/c.h
        c/B.h
        )
set(list ${source_unsorted})
list(SORT list CASE SENSITIVE ORDER ASCENDING COMPARE STRING)
if (NOT expected STREQUAL list)
    message(FATAL_ERROR "wrong sort result with command list(SORT list CASE SENSITIVE ORDER ASCENDING COMPARE STRING)")
endif ()

## Test CASE SENSITIVE ORDER DESCENDING COMPARE STRING
set(expected
        c/B.h
        a/c.h
        B/a.h
        )
set(list ${source_unsorted})
list(SORT list CASE SENSITIVE ORDER DESCENDING COMPARE STRING)
if (NOT expected STREQUAL list)
    message(FATAL_ERROR "wrong sort result with command list(SORT list CASE SENSITIVE ORDER DESCENDING COMPARE STRING)")
endif ()

## Test CASE INSENSITIVE ORDER ASCENDING COMPARE FILE_BASENAME
set(expected
        B/a.h
        c/B.h
        a/c.h
        )
set(list ${source_unsorted})
list(SORT list CASE INSENSITIVE ORDER ASCENDING COMPARE FILE_BASENAME)
if (NOT expected STREQUAL list)
    message(FATAL_ERROR "wrong sort result with command list(SORT list CASE INSENSITIVE ORDER ASCENDING COMPARE FILE_BASENAME)")
endif ()

## Test CASE INSENSITIVE ORDER DESCENDING COMPARE FILE_BASENAME
set(expected
        a/c.h
        c/B.h
        B/a.h
        )
set(list ${source_unsorted})
list(SORT list CASE INSENSITIVE ORDER DESCENDING COMPARE FILE_BASENAME)
if (NOT expected STREQUAL list)
    message(FATAL_ERROR "wrong sort result with command list(SORT list CASE INSENSITIVE ORDER DESCENDING COMPARE FILE_BASENAME)")
endif ()

## Test CASE SENSITIVE ORDER ASCENDING COMPARE FILE_BASENAME
set(expected
        c/B.h
        B/a.h
        a/c.h
        )
set(list ${source_unsorted})
list(SORT list CASE SENSITIVE ORDER ASCENDING COMPARE FILE_BASENAME)
if (NOT expected STREQUAL list)
    message(FATAL_ERROR "wrong sort result with command list(SORT list CASE SENSITIVE ORDER ASCENDING COMPARE FILE_BASENAME)")
endif ()

## Test CASE SENSITIVE ORDER DESCENDING COMPARE FILE_BASENAME
set(expected
        a/c.h
        B/a.h
        c/B.h
        )
set(list ${source_unsorted})
list(SORT list CASE SENSITIVE ORDER DESCENDING COMPARE FILE_BASENAME)
if (NOT expected STREQUAL list)
    message(FATAL_ERROR "wrong sort result with command list(SORT list CASE SENSITIVE ORDER DESCENDING COMPARE FILE_BASENAME)")
endif ()
