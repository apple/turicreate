# this file simulates a program that has been built with ThreadSanitizer
# options

message("TSAN_OPTIONS = [$ENV{TSAN_OPTIONS}]")
string(REGEX REPLACE ".*log_path=\'([^\']*)\'.*" "\\1" LOG_FILE "$ENV{TSAN_OPTIONS}")
message("LOG_FILE=[${LOG_FILE}]")

set(error_types
 "data race"
 "data race on vptr (ctor/dtor vs virtual call)"
 "heap-use-after-free"
 "thread leak"
 "destroy of a locked mutex"
  "double lock of a mutex"
  "unlock of an unlocked mutex (or by a wrong thread)"
  "read lock of a write locked mutex"
  "read unlock of a write locked mutex"
  "signal-unsafe call inside of a signal"
  "signal handler spoils errno"
  "lock-order-inversion (potential deadlock)"
 )

# clear the log file
file(REMOVE "${LOG_FILE}.2343")

# create an error of each type of thread santizer
# these names come from tsan_report.cc in llvm
foreach(error_type ${error_types} )

  file(APPEND "${LOG_FILE}.2343"
"==================
WARNING: ThreadSanitizer: ${error_type} (pid=27978)
  Write of size 4 at 0x7fe017ce906c by thread T1:
    #0 Thread1 ??:0 (exe+0x000000000bb0)
    #1 <null> <null>:0 (libtsan.so.0+0x00000001b279)

  Previous write of size 4 at 0x7fe017ce906c by main thread:
    #0 main ??:0 (exe+0x000000000c3c)

  Thread T1 (tid=27979, running) created by main thread at:
    #0 <null> <null>:0 (libtsan.so.0+0x00000001ed7b)
    #1 main ??:0 (exe+0x000000000c2c)

SUMMARY: ThreadSanitizer: ${error_type} ??:0 Thread1
==================
")
endforeach()
