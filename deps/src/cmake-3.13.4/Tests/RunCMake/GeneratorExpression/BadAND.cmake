add_custom_target(check ALL COMMAND check
  $<AND>
  $<AND:>
  $<AND:,>
  $<AND:01>
  $<AND:nothing>
  $<AND:1,nothing>
  VERBATIM)
