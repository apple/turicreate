# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
Provides utility context managers related
to executing cython functions
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import os


class debug_trace(object):
    """
    A context manager that suppress the cython
    stack trace in release build.
    """

    def __init__(self):
        self.show_cython_trace = 'TURI_BUILD_ROOT' in os.environ
        self.show_server_log = 'TURI_VERBOSE' in os.environ

    def __enter__(self):
        pass

    def _fetch_unity_log(self):
        try:
            from ..connect.main import get_server
            logfile = get_server().unity_log
            logcontent =  "\n======================================="
            logcontent += "\n unity server log location: " + logfile
            logcontent += "\n=======================================\n"
            logcontent += "\nLast 100 lines:\n"
            with open(logfile, 'r') as f:
                logcontent += "".join(f.readlines()[-100:])
            logcontent += "\n========= End of server log ===========\n"
            return logcontent
        except:
            return ""

    def __exit__(self, exc_type, exc_value, traceback):
        if exc_type:
            if not self.show_cython_trace:
                # To hide cython trace, we re-raise from here
                raise exc_type(exc_value)
            else:
                # To show the full trace, we do nothing and let exception propagate

                # In verbose mode we print the server log
                if self.show_server_log:
                    print(self._fetch_unity_log())
