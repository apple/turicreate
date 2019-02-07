project(stack_trace)

add_library(stack_trace OBJECT
  llvm_lib.cpp
  stack_trace.cpp
)
add_dependencies(stack_trace boost)

if (${TC_HAS_LLVM})
  target_link_libraries(stack_trace PUBLIC ${TC_LLVM_LIBRARIES})

  target_link_libraries(stack_trace PUBLIC -lcurses)
  target_link_libraries(stack_trace PUBLIC -lz)
  target_link_libraries(stack_trace PUBLIC -lm)

  if (TC_PLATFORM_LINUX)
    target_link_libraries(stack_trace PUBLIC -ldl)
  endif()

  if (${TC_HAS_LIBUNWIND_STANDALONE})
    target_link_libraries(stack_trace PUBLIC ${TC_LIBUNWIND})
  else()
    target_link_libraries(stack_trace PUBLIC -lunwind)
  endif()
endif()
