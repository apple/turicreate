
cmake_policy(SET CMP0068 OLD)
cmake_policy(SET CMP0042 NEW)

add_library(foo SHARED empty.cpp)
set_target_properties(foo PROPERTIES INSTALL_NAME_DIR "@rpath" INSTALL_RPATH "@loader_path/" BUILD_WITH_INSTALL_RPATH 1)
