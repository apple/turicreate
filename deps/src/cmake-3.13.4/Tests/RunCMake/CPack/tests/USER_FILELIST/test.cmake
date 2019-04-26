install(FILES CMakeLists.txt DESTINATION one     RENAME foo.txt)
install(FILES CMakeLists.txt DESTINATION one/two RENAME bar.txt)
install(FILES CMakeLists.txt DESTINATION three   RENAME baz.txt)
install(FILES CMakeLists.txt DESTINATION three   RENAME qux.txt)

# We are verifying the USER_FILELIST works by comparing what
# ends up with a %doc tag in the final rpm with what we expect
# from this USER_FILELIST.
set(CPACK_RPM_USER_FILELIST
  "%doc /usr/one/foo.txt"
  "%doc %attr(640,root,root) /usr/one/two/bar.txt"
  "%attr(600, -, root) %doc /usr/three/baz.txt"
)
