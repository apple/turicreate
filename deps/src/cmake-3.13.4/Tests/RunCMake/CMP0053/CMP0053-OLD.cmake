cmake_policy(SET CMP0053 OLD)

function (watch_callback)
  message("called")
endfunction ()

variable_watch(test watch_callback)
message("-->${test}<--")
