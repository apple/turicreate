math(EXPR var "'2*1-1'")
if(NOT var EQUAL 1)
  message(FATAL_ERROR "Expression did not evaluate to 1")
endif()
