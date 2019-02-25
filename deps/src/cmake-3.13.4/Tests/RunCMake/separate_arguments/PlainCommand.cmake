set(old_out "a b  c")
separate_arguments(old_out)
set(old_exp "a;b;;c")

if(NOT "${old_out}" STREQUAL "${old_exp}")
  message(FATAL_ERROR "separate_arguments old-style failed.  "
    "Expected\n  [${old_exp}]\nbut got\n  [${old_out}]\n")
endif()
