enable_language(C)
add_library(some /${CMAKE_CURRENT_SOURCE_DIR}/someFileWithoutSpecialChars)
set_property(SOURCE /${CMAKE_CURRENT_SOURCE_DIR}/someFileWithoutSpecialChars PROPERTY LANGUAGE C)
