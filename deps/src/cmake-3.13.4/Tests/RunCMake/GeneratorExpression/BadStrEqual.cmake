add_custom_target(check ALL COMMAND check
  $<STREQUAL>
  $<STREQUAL:>
  $<STREQUAL:,,>
  $<STREQUAL:something,,>
  VERBATIM)
