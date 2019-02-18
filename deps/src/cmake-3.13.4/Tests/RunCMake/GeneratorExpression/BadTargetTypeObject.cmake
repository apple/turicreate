add_library(objlib OBJECT)
add_custom_target(check ALL COMMAND echo
  $<TARGET_FILE:objlib>
  $<TARGET_SONAME_FILE:objlib>
  $<TARGET_LINKER_FILE:objlib>
  )
