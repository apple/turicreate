#!/usr/bin/env python

import os
import subprocess
import StringIO

# TODO - for now, assumes you are running from the root of the repo.
# Takes no arguments and simply reports on the results found.

exit_code = 0

# Health check #1: make sure precompiled header is used instead of direct header includes.
# (For each file in the precompiled header.)

print("Health check #1: Looking for header includes that should be using PCH.")
print("Suggested fix: drop these includes from source, as the PCH already includes them.")
with open('src/pch/pch.in.hpp', 'r') as f:
    for line in f:
        line = line.strip()
        if line.startswith('#include <'):
            # We have an include line
            # Make sure it's not duplicated in any other source code
            p1 = subprocess.Popen(["git", "grep", line], stdout=subprocess.PIPE)
            p2 = subprocess.Popen(["grep", "-v", "^src\/external\/\|^src\/pch\/\|^deps\/"], stdin=p1.stdout, stdout=subprocess.PIPE)
            p3 = subprocess.Popen(["wc", "-l"], stdin=p2.stdout, stdout=subprocess.PIPE)
            output = p3.stdout.read()
            output = output.strip()
            output = int(output)
            print("Found %d instances of %s" % (output, line))
            exit_code += output

exit(exit_code)