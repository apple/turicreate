add_custom_target(check ALL COMMAND check
  $<NOT>
  $<NOT:>
  $<NOT:,>
  $<NOT:0,1>
  $<NOT:01>
  $<NOT:nothing>
  VERBATIM)
