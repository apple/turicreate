set(FOO )
set(BAR "bar")
set(BAZ "baz")
set(BOO "boo")

function(_parent_scope)
    set(FOO "foo" PARENT_SCOPE)
    set(BAR "" PARENT_SCOPE)
    set(BAZ PARENT_SCOPE)
    unset(BOO PARENT_SCOPE)
endfunction()

_parent_scope()

if(NOT DEFINED FOO)
  message(FATAL_ERROR "FOO not defined")
elseif(NOT "${FOO}" STREQUAL "foo")
  message(FATAL_ERROR "FOO should be \"foo\", not \"${FOO}\"")
endif()

if(NOT DEFINED BAR)
  message(FATAL_ERROR "BAR not defined")
elseif(NOT "${BAR}" STREQUAL "")
  message(FATAL_ERROR "BAR should be an empty string, not \"${BAR}\"")
endif()

if(DEFINED BAZ)
  message(FATAL_ERROR "BAZ defined")
endif()

if(DEFINED BOO)
  message(FATAL_ERROR "BOO defined")
endif()
