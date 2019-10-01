macro(mac x)
  message([[${x},${ARGN},${ARGC},${ARGV},${ARGV0},${ARGV1},${ARGV2}:]]
           "${x},${ARGN},${ARGC},${ARGV},${ARGV0},${ARGV1},${ARGV2}")
endmacro(mac)
mac(a n)
mac(b n)
