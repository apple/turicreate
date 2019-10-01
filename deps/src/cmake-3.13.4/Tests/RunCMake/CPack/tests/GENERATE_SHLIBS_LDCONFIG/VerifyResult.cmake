set(shlibs_shlibs "^libtest_lib 0\\.8 generate_shlibs_ldconfig \\(>\\= 0\\.1\\.1\\)\n$")
# NOTE: optional dot at the end of permissions regex is for SELinux enabled systems
set(shlibs_shlibs_permissions_regex "-rw-r--r--\.? .*")
set(shlibs_postinst ".*ldconfig.*")
set(shlibs_postinst_permissions_regex "-rwxr-xr-x\.? .*")
set(shlibs_postrm ".*ldconfig.*")
set(shlibs_postrm_permissions_regex "-rwxr-xr-x\.? .*")
verifyDebControl("${FOUND_FILE_1}" "shlibs" "shlibs;postinst;postrm")
