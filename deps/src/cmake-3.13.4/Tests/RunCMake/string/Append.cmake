set(out)
string(APPEND out)
if(DEFINED out)
  message(FATAL_ERROR "\"string(APPEND out)\" set out to \"${out}\"")
endif()

set(out "")
string(APPEND out)
if(NOT out STREQUAL "")
  message(FATAL_ERROR "\"string(APPEND out)\" set out to \"${out}\"")
endif()

set(out x)
string(APPEND out)
if(NOT out STREQUAL "x")
  message(FATAL_ERROR "\"string(APPEND out)\" set out to \"${out}\"")
endif()


set(out)
string(APPEND out a)
if(NOT out STREQUAL "a")
  message(FATAL_ERROR "\"string(APPEND out a)\" set out to \"${out}\"")
endif()

set(out "")
string(APPEND out a)
if(NOT out STREQUAL "a")
  message(FATAL_ERROR "\"string(APPEND out a)\" set out to \"${out}\"")
endif()

set(out x)
string(APPEND out a)
if(NOT out STREQUAL "xa")
  message(FATAL_ERROR "\"string(APPEND out a)\" set out to \"${out}\"")
endif()


set(out x)
string(APPEND out a "b")
if(NOT out STREQUAL "xab")
  message(FATAL_ERROR "\"string(APPEND out a \"b\")\" set out to \"${out}\"")
endif()

set(b)
set(out x)
string(APPEND out ${b})
if(NOT out STREQUAL "x")
  message(FATAL_ERROR "\"string(APPEND out \${b})\" set out to \"${out}\"")
endif()

set(b b)
set(out x)
string(APPEND out a "${b}" [[
${c}]])
if(NOT out STREQUAL "xab\${c}")
  message(FATAL_ERROR "\"string(APPEND out a \"\${b}\" [[\${c}]])\" set out to \"${out}\"")
endif()
