foreach (_lang IN ITEMS C CXX)
  add_test(NAME FindThreads.${_lang}-only COMMAND ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/FindThreads/${_lang}-only"
    "${CMake_BINARY_DIR}/Tests/FindThreads/${_lang}-only"
    ${build_generator_args}
    --build-project FindThreads_${_lang}-only
    --build-options ${build_options}
    --test-command ${CMAKE_CTEST_COMMAND} -V
    )
endforeach ()
