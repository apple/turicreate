# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import sys
try:
    import IPython
    from IPython.core.interactiveshell import InteractiveShell
    have_ipython = True
except ImportError:
    have_ipython = False

def print_callback(val):
    """
    Internal function.
    This function is called via a call back returning from IPC to Cython
    to Python. It tries to perform incremental printing to IPython Notebook or
    Jupyter Notebook and when all else fails, just prints locally.
    """
    success = False
    try:
        # for reasons I cannot fathom, regular printing, even directly
        # to io.stdout does not work.
        # I have to intrude rather deep into IPython to make it behave
        if have_ipython:
            if InteractiveShell.initialized():
                IPython.display.publish_display_data({'text/plain':val,'text/html':'<pre>' + val + '</pre>'})
                success = True
    except:
        pass

    if not success:
        print(val)
        sys.stdout.flush()
