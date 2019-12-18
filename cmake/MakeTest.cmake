
function (make_boost_test NAME)
  set (SOURCES ${NAME})
  set(args ${ARGN})
  make_executable(${NAME}test SOURCES ${SOURCES} ${args})
  target_link_libraries(${NAME}test PUBLIC unity_shared_for_testing boost_test ${_TC_COMMON_REQUIREMENTS})

  add_test(${NAME} ${NAME}test)
endfunction()

# Same as target_compile_options, but tests for whether the flag is known
# to the compiler before proceeding.
function (target_optional_compile_flag NAME FLAG)
  check_cxx_compiler_flag(${FLAG} __has_flag)
  if (${__has_flag})
    target_compile_options(${NAME} PRIVATE ${FLAG})
  endif()
endfunction()

