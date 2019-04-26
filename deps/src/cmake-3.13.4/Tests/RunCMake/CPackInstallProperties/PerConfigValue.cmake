add_executable(mytest test.cpp)

foreach(CONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
  string(TOUPPER ${CONFIG} UPPER_CONFIG)
  set_property(TARGET mytest PROPERTY
    OUTPUT_NAME_${UPPER_CONFIG} bar_${CONFIG})
endforeach()

file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/runtest_info_$<CONFIG>.cmake CONTENT [[
set(CPACK_BUILD_CONFIG "$<CONFIG>")
set(EXPECTED_MYTEST_NAME "$<TARGET_FILE_NAME:mytest>")
]])

set_property(INSTALL config.cpp PROPERTY FOO $<TARGET_FILE_NAME:mytest>)
