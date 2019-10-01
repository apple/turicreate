function(my_func)
  message("my_func")
endfunction()
variable_watch(a my_func)
set(a "")

variable_watch(b)
set(b "")
