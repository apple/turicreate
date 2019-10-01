message("Start")

variable_watch(TESTVAR MESSAGE)
variable_watch(TESTVAR1)

macro(testwatch var access file stack)
  message("There was a ${access} access done on the variable: ${var} in file ${file}")
  message("List file stack is: ${stack}")
  set(${var}_watched 1)
endmacro()

variable_watch(somevar testwatch)

set(TESTVAR1 "1")
set(TESTVAR "1")
set(TESTVAR1 "0")
set(TESTVAR "0")


message("Variable: ${somevar}")
if(NOT somevar_watched)
  message(SEND_ERROR "'somevar' watch failed!")
endif()
set(somevar_watched)

set(somevar "1")
message("Variable: ${somevar}")
if(NOT somevar_watched)
  message(SEND_ERROR "'somevar' watch failed!")
endif()
remove(somevar)
