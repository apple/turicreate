#!/usr/bin/env bash
# Update the version component if it looks like a date or -f is given.
if test "x$1" = "x-f"; then shift ; n='*' ; else n='\{8\}' ; fi
if test "$#" -gt 0; then echo 1>&2 "usage: CMakeVersion.bash [-f]"; exit 1; fi
sed -i -e '
s/\(^set(CMake_VERSION_PATCH\) [0-9]'"$n"'\(.*\)/\1 '"$(date +%Y%m%d)"'\2/
' "${BASH_SOURCE%/*}/CMakeVersion.cmake"
