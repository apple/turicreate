project(CASE4 C)

# This is not really a circular dependency.  "case4Bar" refers to a
# third-party library that happens to match the executable name, which
# is okay when the executable is not a linkable target (ENABLE_EXPORTS
# is not set).  This tests whether CMake avoids incorrectly reporting
# a circular dependency.  In practice case4Foo may be a shared
# library, but we skip that here because we do not want it to actually
# have to find the third-party library.
add_library(case4Foo STATIC foo.c)
target_link_libraries(case4Foo case4Bar)

# The executable avoids linking to a library with its own name, which
# has been a CMake-ism for a long time, so we will not get a link
# failure.  An imported target or executable with an OUTPUT_NAME set
# may be used if the user really wants to link a third-party library
# into an executable of the same name.
add_executable(case4Bar bar.c)
target_link_libraries(case4Bar case4Foo)
