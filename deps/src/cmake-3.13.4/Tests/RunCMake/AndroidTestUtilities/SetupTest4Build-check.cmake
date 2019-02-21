include(${CMAKE_CURRENT_LIST_DIR}/check.cmake)
compare_build_to_expected(FILES
  "data/a.txt"
  )
check_for_setup_test()
