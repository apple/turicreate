# a target with visibility only in this directory
pkg_check_modules(FakePackage1_dir REQUIRED QUIET IMPORTED_TARGET cmakeinternalfakepackage1)

# the same with global visibility
pkg_check_modules(FakePackage1_global REQUIRED QUIET IMPORTED_TARGET GLOBAL cmakeinternalfakepackage1)
