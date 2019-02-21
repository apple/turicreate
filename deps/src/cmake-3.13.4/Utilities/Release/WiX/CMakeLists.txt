add_subdirectory(CustomAction)

if(CMAKE_CONFIGURATION_TYPES)
  set(CUSTOM_ACTION_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/custom_action_dll-$<CONFIG>.wxs")
else()
  set(CUSTOM_ACTION_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/custom_action_dll.wxs")
endif()

file(GENERATE
  OUTPUT "${CUSTOM_ACTION_OUTPUT}"
  INPUT "${CMAKE_CURRENT_SOURCE_DIR}/custom_action_dll.wxs.in"
  )
