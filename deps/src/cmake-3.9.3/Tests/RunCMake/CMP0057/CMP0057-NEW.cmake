cmake_policy(SET CMP0057 NEW)

set(MY_NON_EXISTENT_LIST)

set(MY_EMPTY_LIST "")

set(MY_LIST foo bar)

if(NOT "foo" IN_LIST MY_LIST)
  message(FATAL_ERROR "expected item 'foo' not found in list MY_LIST")
endif()

if("baz" IN_LIST MY_LIST)
  message(FATAL_ERROR "unexpected item 'baz' found in list MY_LIST")
endif()

if("foo" IN_LIST MY_NON_EXISTENT_LIST)
  message(FATAL_ERROR
    "unexpected item 'baz' found in non existent list MY_NON_EXISTENT_LIST")
endif()

if("foo" IN_LIST MY_EMPTY_LIST)
  message(FATAL_ERROR
    "unexpected item 'baz' found in empty list MY_EMPTY_LIST")
endif()

set(VAR "foo")

if(NOT VAR IN_LIST MY_LIST)
  message(FATAL_ERROR "expected item VAR not found in list MY_LIST")
endif()
