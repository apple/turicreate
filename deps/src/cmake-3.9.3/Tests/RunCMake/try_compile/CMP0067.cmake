enable_language(C)

set(CMAKE_POLICY_WARNING_CMP0067 ON)
message("before try_compile with CMP0067 WARN-enabled but no variables")
try_compile(RESULT ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src.c
  )
message("after try_compile with CMP0067 WARN-enabled but no variables")
set(CMAKE_POLICY_WARNING_CMP0067 OFF)

#-----------------------------------------------------------------------------

set(CMAKE_C_STANDARD 90)

message("before try_compile with CMP0067 WARN-default")
try_compile(RESULT ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src.c
  )
message("after try_compile with CMP0067 WARN-default")

set(CMAKE_POLICY_WARNING_CMP0067 ON)
message("before try_compile with CMP0067 WARN-enabled")
try_compile(RESULT ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src.c
  )
message("after try_compile with CMP0067 WARN-enabled")

cmake_policy(SET CMP0067 OLD)
message("before try_compile with CMP0067 OLD")
try_compile(RESULT ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src.c
  )
message("after try_compile with CMP0067 OLD")

cmake_policy(SET CMP0066 NEW)
message("before try_compile with CMP0067 NEW")
try_compile(RESULT ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src.c
  )
message("after try_compile with CMP0067 NEW")
