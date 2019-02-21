file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test/first")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test/second")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test/first/one" "Hi, Mom!")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test/second/two" "Love you!")

file(GLOB CONTENT_LIST
  "${CMAKE_CURRENT_BINARY_DIR}/test/first/*"
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_BINARY_DIR}/test/second/*"
  )
