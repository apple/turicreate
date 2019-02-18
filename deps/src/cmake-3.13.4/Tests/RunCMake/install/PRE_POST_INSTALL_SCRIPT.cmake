enable_language(C)

add_executable(myexe main.c)
set_property(TARGET myexe PROPERTY PRE_INSTALL_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/preinstall.cmake")
set_property(TARGET myexe PROPERTY POST_INSTALL_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/postinstall.cmake")

install(TARGETS myexe DESTINATION bin)
