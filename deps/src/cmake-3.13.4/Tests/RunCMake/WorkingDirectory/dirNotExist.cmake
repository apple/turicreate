include(CTest)

add_test(NAME dirNotExist
         COMMAND ${CMAKE_COMMAND} -E touch someFile.txt
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/thisDirWillNotExist
)
