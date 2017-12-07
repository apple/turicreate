set(whitespaces_ "[\t\n\r ]*")

set(EXPECTED_FILES_COUNT "1")
set(EXPECTED_FILES_NAME_GENERATOR_SPECIFIC_FORMAT TRUE)
# dynamic lib extension is .so on Linux and .dylib on Mac so we will use a wildcard .* for it
set(EXPECTED_FILE_CONTENT_1 "^.*/usr/foo${whitespaces_}.*/usr/foo/libtest_lib\\..*$")
