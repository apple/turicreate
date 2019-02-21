find_program(PROG
  NAMES testCom
  PATHS ${CMAKE_CURRENT_SOURCE_DIR}/Win
  NO_DEFAULT_PATH
  )
message(STATUS "PROG='${PROG}'")
