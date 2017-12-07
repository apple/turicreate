set(whitespaces_ "[\t\n\r ]*")
set(md5sums_md5sums "^.* usr/foo/CMakeLists\.txt${whitespaces_}$")
verifyDebControl("${FOUND_FILE_1}" "md5sums" "md5sums")
