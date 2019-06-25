#!/usr/bin/env python

import os
import sys
from os.path import join, split, normpath, exists
import subprocess

root_path = os.getcwd()

def error_out(*messages):
    print("".join(messages), "\n")
    sys.exit(1)


if (not exists(join(root_path, "src/"))
        or not exists(join(root_path, "test/"))
        or not exists(join(root_path, ".git/config"))):
    error_out("Script must be run in root of repository.")

def _run_git(cmd):
    output = subprocess.check_output(cmd, shell = True)

    if type(output) is not str:
        output = output.decode("ascii")

    return [x.strip() for x in output.split("\n") if len(x.strip()) != 0]

def all_repl(filter_name, *sed_commands):
    cmd = (r"git grep --null -l '%s' src/* test/* | xargs -0 sed -i '' -e %s "
            % (filter_name, " -e ".join("'%s'" % s for s in sed_commands)))

    proc = subprocess.check_call(cmd, shell=True)

def fix_all_headers(header_name, true_header):
    h = header_name

    all_repl(header_name,
        r's|\#include \<boost|\#include_boost_|g',   #Exclude the boost headers from consideration, as some of them have intersecting names.
        r's|\#include \<[^\>]*/%s\>|#include <%s>|g' % (header_name, true_header),
        r's|\#include \<%s\>|#include <%s>|g' % (header_name, true_header),
        r's|\#include \"[^\"]*/%s\"|#include <%s>|g' % (header_name, true_header),
        r's|\#include \"[^\"]*/%s\"|#include <%s>|g' % (header_name, true_header),
        r's|cdef extern from \"\<[^\>]*%s\>\"|cdef extern from "<%s>"|g' % (header_name, true_header),
        r's|\#include_boost_|\#include \<boost|g')  # Place the boost headers back

def fix_headers(*filenames):

    repls = []

    for filename in filenames:
        print("Finding %s" % filename )

        # Get the correct path of the file.
        header_files = _run_git("git ls-files %s%s" % ("src/*/" if not filename.startswith("src/") else "", filename))

        if len(header_files) == 0:
            error_out("File ", filename, " not found in repository.")

        if len(header_files) > 1:
            error_out("Multiple matches for file ", filename, " found. Please disambiguate by providing part of the path.\n"
                "Found: \n"
                + "\n".join(header_files))

        new_file = header_files[0]
        assert new_file.startswith("src/"), new_file

        new_file = new_file[4:]

        repls.append( (filename, new_file) )


    for filename, new_file in repls:

        print("Fixing %s: True header path = \"%s\"" % (filename, new_file))

        fix_all_headers(filename, new_file)


def print_usage_and_exit():

    print("")
    print("Usage: %s <command> [args...]" % sys.argv[0])

    print("Commands: ")
    print("  --fix-headers <header_name>        Fixes all import paths of a unique header.")
    sys.exit(1)


if __name__ == "__main__":

    if len(sys.argv) == 1:
        print_usage_and_exit()

    cmd = sys.argv[1]

    if cmd == "--fix-headers":
        if len(sys.argv) == 2:
           print_usage_and_exit()

        fix_headers(*sys.argv[2:])

    else:

        print("Error: Command %s not recognized." % cmd)

        print_usage_and_exit()
