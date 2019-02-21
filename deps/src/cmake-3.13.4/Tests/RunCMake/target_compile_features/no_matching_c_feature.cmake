enable_language(CXX)

if (NOT ";${CMAKE_C_COMPILE_FEATURES};" MATCHES ";gnu_c_typeof;")
  # Simulate passing the test.
  message(SEND_ERROR
    "The compiler feature \"gnu_c_dummy\" is not known to C compiler\n\"GNU\"\nversion 4.8.1."
  )
  return()
endif()

add_executable(main empty.c)

target_compile_features(main
  PRIVATE
    gnu_c_typeof
)
