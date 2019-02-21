if(RunCMake_SUBTEST_SUFFIX STREQUAL "soversion_not_zero")
  set(shlibs_shlibs "^libtest_lib 0\\.8 generate_shlibs \\(\\= 0\\.1\\.1\\)\n$")
else() # soversion_zero
  set(shlibs_shlibs "^libtest_lib 0 generate_shlibs \\(\\= 0\\.1\\.1\\)\n$")
endif()

# optional dot at the end of permissions regex is for SELinux enabled systems
set(shlibs_shlibs_permissions_regex "-rw-r--r--\.? .*")
verifyDebControl("${FOUND_FILE_1}" "shlibs" "shlibs")
