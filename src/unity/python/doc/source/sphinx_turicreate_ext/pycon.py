# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
import sys
from code import InteractiveInterpreter


def main():
    """
    Print lines of input along with output.
    """
    source_lines = (line.rstrip() for line in sys.stdin)
    console = InteractiveInterpreter()
    console.runsource('import turicreate')
    source = ''
    try:
        while True:
            source = source_lines.next()
            more = console.runsource(source)
            while more:
                next_line = source_lines.next()
                print '...', next_line
                source += '\n' + next_line
                more = console.runsource(source)
    except StopIteration:
        if more:
            print '... '
            more = console.runsource(source + '\n')



if __name__ == '__main__':
    main()


# vim: set expandtab shiftwidth=4 softtabstop=4 :
