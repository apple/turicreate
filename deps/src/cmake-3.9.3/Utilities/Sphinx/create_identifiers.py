#!/usr/bin/env python

import sys, os

if len(sys.argv) != 2:
  sys.exit(-1)
name = sys.argv[1] + "/CMake.qhp"

f = open(name)

if not f:
  sys.exit(-1)

lines = f.read().splitlines()

if not lines:
  sys.exit(-1)

newlines = []

for line in lines:

  mapping = (("command", "command"),
             ("variable", "variable"),
             ("target property", "prop_tgt"),
             ("test property", "prop_test"),
             ("source file property", "prop_sf"),
             ("global property", "prop_gbl"),
             ("module", "module"),
             ("directory property", "prop_dir"),
             ("cache property", "prop_cache"),
             ("policy", "policy"),
             ("installed file property", "prop_inst"))

  for domain_object_string, domain_object_type in mapping:
    if "<keyword name=\"" + domain_object_string + "\"" in line:
      if not "id=\"" in line and not "#index-" in line:
        prefix = "<keyword name=\"" + domain_object_string + "\" "
        part1, part2 = line.split(prefix)
        head, tail = part2.split("#" + domain_object_type + ":")
        domain_object, rest = tail.split("\"")
        line = part1 + prefix + "id=\"" + domain_object_type + "/" + domain_object + "\" " + part2
  newlines.append(line + "\n")

f = open(name, "w")
f.writelines(newlines)
