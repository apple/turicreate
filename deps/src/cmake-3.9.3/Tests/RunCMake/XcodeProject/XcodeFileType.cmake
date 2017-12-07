enable_language(C)
add_executable(main main.c src-default src-explicit src-lastKnown)
set_property(SOURCE src-explicit PROPERTY XCODE_EXPLICIT_FILE_TYPE sourcecode.c.h)
set_property(SOURCE src-lastKnown PROPERTY XCODE_LAST_KNOWN_FILE_TYPE sourcecode.c.h)
