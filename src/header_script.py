import os
from os.path import join, split, relpath, normpath, abspath, exists
import re


root_dir = abspath(os.getcwd())

copyright = """
/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
"""


def do_subdir(d, put_in_root):

    d = relpath(abspath(d), root_dir)

    base, name = split(d)

    macro_name = "TURI_%s_HPP_" % re.sub("[^a-zA-Z]", "_", d).upper()
    forward_macro_name = "TURI_%s_FORWARD_HPP_" % re.sub("[^a-zA-Z]", "_", d).upper()

    if put_in_root:
        out_file = join(d, name + ".hpp")
        forward_out_file = join(d, name + "_forward.hpp")
    else:
        out_file = join(split(d)[0], name + ".hpp")
        forward_out_file = join(split(d)[0], name + "_forward.hpp")


    data = []

    data.append("#ifndef %s" % macro_name)
    data.append("#define %s" % macro_name)
    data.append("")

    for dirpath, dirnames, filenames in os.walk(d):

        for f in filenames:
            if f.endswith(".hpp") or f.endswith(".h"):
                data.append("#include <%s/%s>" % (dirpath, f))


    data.append("")
    data.append("#endif\n")

    if not exists(out_file):
        open(out_file, "w").write("\n".join(data))

        print("Wrote out file %s." % out_file)
    else:
        print("Skipping file %s." % out_file)

    if not exists(forward_out_file):
        open(forward_out_file, "w").write(
"""
#ifndef %s
#define %s

namespace turi {

    // Forward declarations of classes


}

#endif
""" % (forward_macro_name, forward_macro_name))


if __name__ == "__main__":

    def is_header_dir(d):
        if d in ["core", "python", "core/ml", "core/generics", "external"]:
            return False, None

        if re.match("^external/[a-z_]+$", d):
            return True, False

        if re.match("^core/[a-z_]+$", d):
            return True, True

        if re.match("^core/ml/[a-z_]+$", d):
            return True, False

        if re.match("^[a-z_]+$", d):
            return True, True

        return False, None


    paths = (relpath(d, root_dir) for d, dl, fl in os.walk(root_dir))

    for p in paths:
        t, use_root_dir = is_header_dir(p)
        if t:
            print(p)
            do_subdir(p, use_root_dir)
