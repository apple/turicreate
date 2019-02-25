set(nothing)
separate_arguments(nothing)
if(DEFINED nothing)
  message(FATAL_ERROR "separate_arguments null-case failed: "
    "nothing=[${nothing}]")
endif()
