set(out)
string(PREPEND out)
if(DEFINED out)
  message(FATAL_ERROR "\"string(PREPEND out)\" set out to \"${out}\"")
endif()

set(out "")
string(PREPEND out)
if(NOT out STREQUAL "")
  message(FATAL_ERROR "\"string(PREPEND out)\" set out to \"${out}\"")
endif()

set(out x)
string(PREPEND out)
if(NOT out STREQUAL "x")
  message(FATAL_ERROR "\"string(PREPEND out)\" set out to \"${out}\"")
endif()


set(out)
string(PREPEND out a)
if(NOT out STREQUAL "a")
  message(FATAL_ERROR "\"string(PREPEND out a)\" set out to \"${out}\"")
endif()

set(out "")
string(PREPEND out a)
if(NOT out STREQUAL "a")
  message(FATAL_ERROR "\"string(PREPEND out a)\" set out to \"${out}\"")
endif()

set(out x)
string(PREPEND out a)
if(NOT out STREQUAL "ax")
  message(FATAL_ERROR "\"string(PREPEND out a)\" set out to \"${out}\"")
endif()


set(out x)
string(PREPEND out a "b")
if(NOT out STREQUAL "abx")
  message(FATAL_ERROR "\"string(PREPEND out a \"b\")\" set out to \"${out}\"")
endif()

set(b)
set(out x)
string(PREPEND out ${b})
if(NOT out STREQUAL "x")
  message(FATAL_ERROR "\"string(PREPEND out \${b})\" set out to \"${out}\"")
endif()

set(b b)
set(out x)
string(PREPEND out a "${b}" [[
${c}]])
if(NOT out STREQUAL "ab\${c}x")
  message(FATAL_ERROR "\"string(PREPEND out a \"\${b}\" [[\${c}]])\" set out to \"${out}\"")
endif()
