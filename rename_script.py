import re
import os
from os.path import join, normpath, relpath


expr = r'^\s*(?P<prefix>[\w_-/]*){(?P<src>[\w_-/]*) => (?P<dest>[\w_-/]*)}(?P<suffix>[\w_-/\.]*)\s*|'

for line in open('diffs.txt').readlines():

    m = re.match(expr, line)

    if m and m.group('prefix') and (m.group('suffix').endswith(".hpp") or m.group('suffix').endswith(".h")):
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

        if src_path.startswith("src/"):
            src_path = src_path[4:]

        if dest_path.startswith("src/"):
            dest_path = dest_path[4:]

        print("s|%s|%s|g" % (src_path, dest_path) )

        

        

    
