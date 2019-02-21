enable_language(C)
include(CheckIncludeFiles)
check_include_files("stddef.h;stdlib.h" HAVE_MISSING_ARGUMENT_H LANGUAGE)
