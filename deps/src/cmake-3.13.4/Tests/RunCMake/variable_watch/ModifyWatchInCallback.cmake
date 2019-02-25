function (watch2)

endfunction ()

function (watch1)
  variable_watch(watched watch2)
  variable_watch(watched watch2)
  variable_watch(watched watch2)
  variable_watch(watched watch2)
  variable_watch(watched watch2)
  variable_watch(watched watch2)
endfunction ()

variable_watch(watched watch1)
variable_watch(watched watch2)

set(access "${watched}")
