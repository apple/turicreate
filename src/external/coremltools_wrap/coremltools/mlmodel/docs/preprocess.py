import os
import re
from itertools import izip
import inflection

def preprocess():
    "splits _sources/reference.rst into separate files"

    text = open("./_sources/reference.rst", "r").read()
    os.remove("./_sources/reference.rst")

    if not os.path.exists("./_sources/reference"):
        os.makedirs("./_sources/reference")

    def pairwise(iterable):
        "s -> (s0, s1), (s2, s3), (s4, s5), ..."

        iteration = iter(iterable)
        return izip(iteration, iteration)

    sections = map(str.strip, re.split(r"<!--\s*(.+)\s*-->", text))
    for section, content in pairwise(sections[1:]):
        if section.endswith(".proto"):
            section_name = section[:-len(".proto")]
            file_name = "./_sources/reference/{0}.rst".format(section_name)
            with open(file_name, "w") as f:
                f.truncate()
                f.write(content)
                f.close()

if __name__ == "__main__":
    preprocess()