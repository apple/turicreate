function (check_target_property target prop)
  get_target_property(gtp_val "${target}" "${prop}")
  get_property(gp_val
    TARGET "${target}"
    PROPERTY "${prop}")

  message("get_target_property: -->${gtp_val}<--")
  message("get_property: -->${gp_val}<--")
endfunction ()

add_custom_target(tgt)
set_target_properties(tgt PROPERTIES empty "" custom value)

check_target_property(tgt empty)
check_target_property(tgt custom)
check_target_property(tgt noexist)
check_target_property(tgt SOURCE_DIR)
check_target_property(tgt BINARY_DIR)
