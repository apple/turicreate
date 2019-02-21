set(FOO "BAR")

function(generate_warning)
  if("FOO" STREQUAL "BAR")
  endif()
endfunction()

generate_warning()
generate_warning()

if("FOO" STREQUAL "BAR")
endif()
