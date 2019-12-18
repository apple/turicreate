#!/usr/bin/env python

import os
import sys
from os.path import join, split, normpath, exists
import subprocess
import argparse

parser = argparse.ArgumentParser(description='Fix headers.')
parser.add_argument('--header-root', dest='header_root', default='src', type=str, help="Base source of header info.")

parser.add_argument('--src-match', dest='src_match', default='src/* test/*', type=str, help="Match pattern of files to fix.")
parser.add_argument('headers', metavar='header', type=str, nargs='+', help="List of headers to fix.")

args = parser.parse_args()

header_root = args.header_root.replace("//", "/")
while header_root.endswith("/"):
    header_root=header_root[:-1]

root_path = os.getcwd()

def error_out(*messages):
    print("".join(messages), "\n")
    sys.exit(1)

if not exists(header_root + "/") :
    error_out(header_root + " is not a valid directory.")

if (not exists(join(root_path, ".git/config"))):
    error_out("Script must be run in root of repository.")

def _run_git(cmd):
    output = subprocess.check_output(cmd, shell = True)

    if type(output) is not str:
        output = output.decode("ascii")

    return [x.strip() for x in output.split("\n") if len(x.strip()) != 0]

def all_repl(filter_name, *sed_commands):
    cmd = (r"git grep --null -l '%s' %s | xargs -0 sed -i '' -e %s "
            % (filter_name, args.src_match, " -e ".join("'%s'" % s for s in sed_commands)))

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

def fix_headers(filenames):

    repls = []

    for filename in filenames:
        print("Finding %s" % filename )

        # Get the correct path of the file.
        header_files = _run_git("git ls-files %s%s" % (("%s/*/" % header_root) if not filename.startswith("%s/" % header_root) else "", filename))

        if len(header_files) == 0:
            error_out("File ", filename, " not found in repository.")

        if len(header_files) > 1:
            error_out("Multiple matches for file ", filename, " found. Please disambiguate by providing part of the path.\n"
                "Found: \n"
                + "\n".join(header_files))

        new_file = header_files[0]
        assert new_file.startswith( ("%s/" % header_root).replace("//", "/") ), new_file

        new_file = new_file[len(header_root) + 1:]

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

    fix_headers(args.headers)
