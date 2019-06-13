import re
import os
from os.path import join, normpath, relpath, split

# To generate, do:
#
# git diff -l 1000000000 --stat=1000000 --minimal HEAD~1 > diffs.txt


expr = r'^\s*(?P<prefix>[\w_\-/]*){(?P<src>[\w_\-/]*) => (?P<dest>[\w_\-/]*)}(?P<suffix>[\w_\-/\.]*)\s*|'

rename_script = open("rename_original.sh", 'w')

rename_script.write("#!/bin/bash -e\n")

header_switch = open("header_shift.sh", 'w')
header_switch.write("#!/bin/bash -e\n")


for line in open('diffs.txt').readlines():

    m = re.match(expr, line)

    if m and m.group('prefix'):
        # print(line.strip())
        # print(" -> | %s | %s | %s | %s |" % ( )

        pr, src, dest, suffix = m.group('prefix'), m.group('src'), m.group('dest'), m.group('suffix')


        if src.strip() == "":
           src = "."

        if dest.strip() == "":
            dest = "."

        src_path = normpath(pr + "/" + src + "/" + suffix)
        # print("pr = ", pr, ", src = ", src, ", suffix = ", suffix, " --> ", src_path)

        dest_path = normpath(pr + "/" + dest + "/" + suffix)
        # print("pr = ", pr, ", desht = ", dest, ", suffix = ", suffix, " --> ", dest_path)

        header_src_path = src_path
        header_dest_path = dest_path

        #
        rename_script.write("echo '%s -> %s'\n" % (src_path, dest_path))
        rename_script.write("mkdir -p %s\n" % split(dest_path)[0])
        rename_script.write("git mv %s %s\n" % (src_path, dest_path))


        prefixes = ["src/", "src/platform/"]

        def write_rep(sr, dr):
            for p in prefixes:
                if sr.startswith(p) and dr.startswith(p):
                    s = sr[len(p):]
                    d = dr[len(p):]

                    header_switch.write(r"sed -i '' 's|^#include \<%s|^#include <%s|g' dummy.cpp `git grep -l %s`\n" % (s, d, s))

        if src_path.endswith(".hpp") or src_path.endswith(".h"):
            header_switch.write("echo '%s -> %s'\n" % (src_path, dest_path))
            write_rep(src_path, dest_path)
