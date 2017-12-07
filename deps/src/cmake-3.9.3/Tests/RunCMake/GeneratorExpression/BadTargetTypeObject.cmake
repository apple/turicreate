enable_language(C)
add_library(objlib OBJECT empty.c)
add_custom_target(check ALL COMMAND echo
  $<TARGET_FILE:objlib>
  $<TARGET_SONAME_FILE:objlib>
  $<TARGET_LINKER_FILE:objlib>
  )
