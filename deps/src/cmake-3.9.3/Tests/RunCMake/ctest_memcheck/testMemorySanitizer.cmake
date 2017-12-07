# this file simulates a program that has been built with MemorySanitizer
# options

message("MSAN_OPTIONS = [$ENV{MSAN_OPTIONS}]")
string(REGEX REPLACE ".*log_path=\'([^\']*)\'.*" "\\1" LOG_FILE "$ENV{MSAN_OPTIONS}")
message("LOG_FILE=[${LOG_FILE}]")

# if we are not asked to simulate address sanitizer don't do it
if(NOT "$ENV{MSAN_OPTIONS}]" MATCHES "simulate_sanitizer.1")
  return()
endif()
# clear the log file
file(REMOVE "${LOG_FILE}.2343")

# create an error of each type of thread santizer
# these names come from tsan_report.cc in llvm

file(APPEND "${LOG_FILE}.2343"
"=================================================================
==28423== WARNING: MemorySanitizer: use-of-uninitialized-value
    #0 0x7f4364210dd9 in main (/home/kitware/msan/msan-bin/umr+0x7bdd9)
    #1 0x7f4362d9376c in __libc_start_main /build/buildd/eglibc-2.15/csu/libc-start.c:226
    #2 0x7f4364210b0c in _start (/home/kitware/msan/msan-bin/umr+0x7bb0c)

SUMMARY: MemorySanitizer: use-of-uninitialized-value ??:0 main
Exiting
")
