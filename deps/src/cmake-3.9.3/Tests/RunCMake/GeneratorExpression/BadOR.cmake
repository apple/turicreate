add_custom_target(check ALL COMMAND check
  $<OR>
  $<OR:>
  $<OR:,>
  $<OR:01>
  $<OR:nothing>
  $<OR:0,nothing>
  VERBATIM)
