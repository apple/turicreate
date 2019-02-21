# this file simulates a program that has been built with
# UndefinedBehaviorSanitizer options

message("UBSAN_OPTIONS = [$ENV{UBSAN_OPTIONS}]")
string(REGEX REPLACE ".*log_path=\'([^\']*)\'.*" "\\1" LOG_FILE "$ENV{UBSAN_OPTIONS}")
message("LOG_FILE=[${LOG_FILE}]")

# if we are not asked to simulate address sanitizer don't do it
if(NOT "$ENV{UBSAN_OPTIONS}]" MATCHES "simulate_sanitizer.1")
  return()
endif()
# clear the log file
file(REMOVE "${LOG_FILE}.2343")

# create an error like undefined behavior santizer creates;
# these names come from ubsan_diag.cc and ubsan_handlers.cc
# in llvm

file(APPEND "${LOG_FILE}.2343"
"<unknown>: runtime error: left shift of negative value -256
")
