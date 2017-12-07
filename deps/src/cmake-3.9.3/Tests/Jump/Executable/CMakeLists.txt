if(NOT LIBRARY_OUTPUT_PATH)
  link_directories(${Jump_BINARY_DIR}/Library/Static
                   ${Jump_BINARY_DIR}/Library/Shared)
endif()
add_executable(jumpExecutable jumpExecutable.cxx)
target_link_libraries(jumpExecutable jumpStatic jumpShared)
