#!/usr/bin/env python

import os
import pprint
import subprocess
import StringIO

# TODO - for now, assumes you are running from the root of the repo.
# Takes no arguments and simply reports on the results found.

exit_code = 0

# Health check #1: make sure precompiled header is used instead of direct header includes.
# (For each file in the precompiled header.)

print("Health check #1: Looking for header includes that should be using the project-wide precompiled header.")
with open('src/pch/pch.in.hpp', 'r') as f:
    for line in f:
        line = line.strip()
        if line.startswith('#include <'):
            # We have an include line
            # Make sure it's not duplicated in any other source code
            p1 = subprocess.Popen(["git", "grep", line, "src", "test"], stdout=subprocess.PIPE)
            p2 = subprocess.Popen(["grep", "-v", "^src\/external\/\|^src\/pch\/\|^src\/unity\/toolkits\/coreml_export\/\|^.*\.mm:\|^.*\.py:\|^.*\.dox:"], stdin=p1.stdout, stdout=subprocess.PIPE)
            matching_files = p2.stdout.readlines()
            count = len(matching_files)
            if count != 0:
                print("Found %d instances of %s:" % (count, line))
                for match in matching_files:
                    match = match.strip()
                    print("    %s" % match)
                exit_code += count
    
    if exit_code != 0:
        print("")
        print("Suggested fix: remove these includes from source code, as the pre-compiled header already includes them. Make sure the first include in the file is for <pch/pch.hpp>.")

print("")
exit(exit_code)