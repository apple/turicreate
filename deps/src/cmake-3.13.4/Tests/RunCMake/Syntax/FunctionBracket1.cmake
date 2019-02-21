function(fun x)
  message([[${x},${ARGN},${ARGC},${ARGV},${ARGV0},${ARGV1},${ARGV2}:]]
           "${x},${ARGN},${ARGC},${ARGV},${ARGV0},${ARGV1},${ARGV2}")
endfunction(fun)
fun(a n)
fun(b n)
