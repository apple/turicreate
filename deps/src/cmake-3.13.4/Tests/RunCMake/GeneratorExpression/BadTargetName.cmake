add_custom_target(check ALL COMMAND check
  $<TARGET_NAME:$<1:tgt>>
  VERBATIM)
