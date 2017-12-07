enable_language(CXX)

if (NOT ";${CMAKE_CXX_COMPILE_FEATURES};" MATCHES ";gnu_cxx_typeof;"
    AND NOT ";${CMAKE_CXX_COMPILE_FEATURES};" MATCHES ";msvc_cxx_sealed;" )
  # Simulate passing the test.
  message(SEND_ERROR
    "The compiler feature \"gnu_cxx_dummy\" is not known to CXX compiler\n\"GNU\"\nversion 4.8.1."
  )
  return()
endif()

if (";${CMAKE_CXX_COMPILE_FEATURES};" MATCHES ";gnu_cxx_typeof;")
  set(feature msvc_cxx_sealed)
  if (";${CMAKE_CXX_COMPILE_FEATURES};" MATCHES ";msvc_cxx_sealed;")
    # If a compiler supports both extensions, remove one of them.
    list(REMOVE_ITEM CMAKE_CXX_COMPILE_FEATURES msvc_cxx_sealed)
  endif()
else()
  set(feature gnu_cxx_typeof)
endif()

add_executable(main empty.cpp)

target_compile_features(main
  PRIVATE
    ${feature}
)
