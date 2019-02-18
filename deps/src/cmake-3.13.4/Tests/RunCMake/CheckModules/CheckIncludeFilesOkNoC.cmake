enable_language(CXX)
include(CheckIncludeFiles)
check_include_files("cstddef;cstdlib" HAVE_CSTDLIB_H3 LANGUAGE CXX)
check_include_files("cstddef;cstdlib" HAVE_CSTDLIB_H4)
