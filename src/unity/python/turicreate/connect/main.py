# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
This module contains the main logic for start, query, stop turicreate server client connection.
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from ..cython.cy_unity import UnityGlobalProxy
from ..cython.cy_server import EmbeddedServer

import decorator
import logging

""" Global variables """
__UNITY_GLOBAL_PROXY__ = None
__SERVER__ = None

# Decorator which catch the exception and output to log error.
@decorator.decorator
def __catch_and_log__(func, *args, **kargs):
    try:
        return func(*args, **kargs)
    except Exception as error:
        logging.getLogger(__name__).error(error)
        raise


@__catch_and_log__
def launch(server_log=None):
    global __UNITY_GLOBAL_PROXY__
    global __SERVER__

    server = EmbeddedServer(server_log)
    server.start()
    server.set_log_progress(True)

    # This must be imported after server.set_log_progress is called.
    from ..extensions import _publish

    __UNITY_GLOBAL_PROXY__ = UnityGlobalProxy()
    __SERVER__ = server

    _publish()


def get_server():
    return __SERVER__

def get_unity():
    """
    Returns the unity global object of the current connection.
    """
    return __UNITY_GLOBAL_PROXY__


