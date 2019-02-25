enable_language(C)
add_executable(empty1 empty.c)
set_property(TARGET empty1 PROPERTY OUTPUT_NAME $<TARGET_FILE_NAME:empty1>)
