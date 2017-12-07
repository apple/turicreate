# this file simulates a program that has been built with address sanitizer
# options

message("ASAN_OPTIONS = [$ENV{ASAN_OPTIONS}]")
string(REGEX REPLACE ".*log_path=\'([^\']*)\'.*" "\\1" LOG_FILE "$ENV{ASAN_OPTIONS}")
message("LOG_FILE=[${LOG_FILE}]")

# if we are not asked to simulate address sanitizer don't do it
if(NOT "$ENV{ASAN_OPTIONS}]" MATCHES "simulate_sanitizer.1")
  return()
endif()
# clear the log file
file(REMOVE "${LOG_FILE}.2343")

# create an example error from address santizer

file(APPEND "${LOG_FILE}.2343"
"=================================================================
==19278== ERROR: AddressSanitizer: heap-buffer-overflow on address 0x60080000bffc at pc 0x4009f1 bp 0x7fff82de6520 sp 0x7fff82de6518
WRITE of size 4 at 0x60080000bffc thread T0
    #0 0x4009f0 (/home/kitware/msan/a.out+0x4009f0)
    #1 0x7f18b02c876c (/lib/x86_64-linux-gnu/libc-2.15.so+0x2176c)
    #2 0x400858 (/home/kitware/msan/a.out+0x400858)
0x60080000bffc is located 4 bytes to the right of 40-byte region [0x60080000bfd0,0x60080000bff8)
allocated by thread T0 here:
    #0 0x7f18b088f9ca (/usr/lib/x86_64-linux-gnu/libasan.so.0.0.0+0x119ca)
    #1 0x4009a2 (/home/kitware/msan/a.out+0x4009a2)
    #2 0x7f18b02c876c (/lib/x86_64-linux-gnu/libc-2.15.so+0x2176c)
Shadow bytes around the buggy address:
  0x0c017fff97a0: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c017fff97b0: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c017fff97c0: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c017fff97d0: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c017fff97e0: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
=>0x0c017fff97f0: fa fa fa fa fa fa fa fa fa fa 00 00 00 00 00[fa]
  0x0c017fff9800:fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c017fff9810: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c017fff9820: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c017fff9830: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x0c017fff9840: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Partially addressable: 01 02 03 04 05 06 07
  Heap left redzone:     fa
  Heap righ redzone:     fb
  Freed Heap region:     fd
  Stack left redzone:    f1
  Stack mid redzone:     f2
  Stack right redzone:   f3
  Stack partial redzone: f4
  Stack after return:    f5
  Stack use after scope: f8
  Global redzone:        f9
  Global init order:     f6
  Poisoned by user:      f7
  ASan internal:         fe
==19278== ABORTING
")
