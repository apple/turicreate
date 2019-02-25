add_custom_target(check ALL COMMAND check
  $<INSTALL_PREFIX>/include
  VERBATIM)
