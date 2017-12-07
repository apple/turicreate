# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

##\internal
"""@package turicreate.toolkits

Internal Toolkit Calling
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from ..connect import main as glconnect

import time
import logging


class ToolkitError(RuntimeError):
    pass


def run(toolkit_name, options, verbose=True, show_progress=False):
    """
    Internal function to execute toolkit on the turicreate server.

    Parameters
    ----------
    toolkit_name : string
        The name of the toolkit.

    options : dict
        A map containing the required input for the toolkit function,
        for example: {'graph': g, 'reset_prob': 0.15}.

    verbose : bool
        If true, enable progress log from server.

    show_progress : bool
        If true, display progress plot.

    Returns
    -------
    out : dict
        The toolkit specific model parameters.

    Raises
    ------
    RuntimeError
        Raises RuntimeError if the server fail executing the toolkit.
    """
    unity = glconnect.get_unity()
    if (not verbose):
        glconnect.get_server().set_log_progress(False)
    # spawn progress threads
    try:
        start_time = time.time()
        (success, message, params) = unity.run_toolkit(toolkit_name, options)
        end_time = time.time()
    except:
        raise

    if (len(message) > 0):
        logging.getLogger(__name__).error("Toolkit error: " + message)

    track_props = {}
    track_props['success'] = success

    if success:
        track_props['runtime'] = end_time - start_time
    else:
        if (len(message) > 0):
            track_props['message'] = message

    # set the verbose level back to default
    glconnect.get_server().set_log_progress(True)

    if success:
        return params
    else:
        raise ToolkitError(str(message))
