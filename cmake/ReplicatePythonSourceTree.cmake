# Note: when executed in the build dir, then CMAKE_CURRENT_SOURCE_DIR is the
# build dir.
file( COPY . tests DESTINATION "${CMAKE_ARGV3}"
        FILES_MATCHING PATTERN "*.py" PATTERN "*.ipynb" PATTERN "*.txt" PATTERN "*.in" PATTERN "*.sh" PATTERN "*.lua" PATTERN "LICENSE" PATTERN "*.conf" PATTERN "*.png" PATTERN "*.jpeg" PATTERN "*.jpg" PATTERN "*CMakeLists.txt" EXCLUDE PATTERN "*.cmake" EXCLUDE)

file (COPY docs DESTINATION ${CMAKE_ARGV3}
  FILES_MATCHING PATTERN "*.*" PATTERN "Makefile")
