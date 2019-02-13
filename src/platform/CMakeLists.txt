project(TuriPlatformDepedentcode)

subdirs(
  crash_handler
  minipsutil
  network
  parallel
  perf
  process
  shmipc
  so_utils
  timer
  # TODO - seems we can delete user_pagefault? Nothing is using it.
  #user_pagefault
  config
  )
