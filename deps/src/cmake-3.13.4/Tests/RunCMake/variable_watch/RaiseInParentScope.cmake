
function(watch variable access value)
  message("${variable} ${access} ${value}")
endfunction ()

# --------------

variable_watch(var watch)
set(var "a")

function(f)
  set(var "b" PARENT_SCOPE)
endfunction(f)

f()
