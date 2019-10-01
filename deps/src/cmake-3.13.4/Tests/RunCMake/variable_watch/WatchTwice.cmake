function (watch1)
    message("From watch1")
endfunction ()

function (watch2)
    message("From watch2")
endfunction ()

variable_watch(watched watch1)
variable_watch(watched watch2)
set(access "${watched}")
