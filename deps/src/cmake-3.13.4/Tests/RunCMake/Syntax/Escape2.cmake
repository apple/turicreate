cmake_policy(SET CMP0053 NEW)

macro (escape str)
  message("${str}")
endmacro ()

escape("\\")
