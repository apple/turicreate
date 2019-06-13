# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
This module contains the interface for turicreate server, and the
implementation of a local turicreate server.
"""
from ..util._config import DEFAULT_CONFIG as default_local_conf
from .. import _sys_util

import logging
import os
import sys
from libcpp.string cimport string
from cy_cpp_utils cimport str_to_cpp, cpp_to_str
from .python_printer_callback import print_callback
from .. import _connect

cdef extern from "<unity/server/unity_server_control.hpp>" namespace "turi":
    cdef cppclass unity_server_options:
        string log_file
        string root_path
        bint daemon
        size_t log_rotation_interval
        size_t log_rotation_truncate

    void start_server(const unity_server_options& server_options)
    void stop_server()
    void set_log_progress "turi::set_log_progress"(bint enable)
    void set_log_progress_callback "turi::set_log_progress_callback" ( void (*callback)(const string&) )


class GraphLabServer(object):
    """
    Interface class for a turicreate server.
    """
    def __init__(self):
        raise NotImplementedError

    def start(self, num_tolerable_ping_failures):
        """ Starts the server. """
        raise NotImplementedError

    def stop(self):
        """ Stops the server. """
        raise NotImplementedError

    def set_log_progress(self, enable):
        """ Enable or disable log progress printing """
        raise NotImplementedError

    def try_stop(self):
        """ Try stopping the server and swallow the exception. """
        try:
            self.stop()
        except:
            e = sys.exc_info()[0]
            self.get_logger().warning(e)

    def get_logger(self):
        """ Return the logger object. """
        raise NotImplementedError

    def log_progress_enabled(self):
        """ Return True if progress is enabled else False. """
        raise NotImplementedError


cdef void print_status(const string& status_string) nogil:
    with gil:
        print_callback(cpp_to_str(status_string).rstrip())

class EmbeddedServer(GraphLabServer):
    """
    Embedded Server loads unity_server into the same process as shared library.
    """

    def __init__(self, unity_log_file):
        """
        @param unity_log_file string The path to the server logfile.
        """
        self.unity_log = unity_log_file
        self.logger = logging.getLogger(__name__)

        root_path = os.path.dirname(os.path.abspath(__file__))  # sframe/connect
        root_path = os.path.abspath(os.path.join(root_path, os.pardir))  # sframe/
        self.root_path = root_path
        self.started = False
        self._log_progress_enabled = False

        if not self.unity_log:
            self.unity_log = default_local_conf.get_unity_log()

    def __del__(self):
        self.stop()

    def start(self):

        if sys.platform == 'win32':
            self.unity_log += ".0"

        # Set up the structure used to call it with all these parameters.
        cdef unity_server_options server_opts
        server_opts.root_path             = str_to_cpp(self.root_path)
        server_opts.log_file              = str_to_cpp(self.unity_log)
        server_opts.log_rotation_interval = default_local_conf.log_rotation_interval
        server_opts.log_rotation_truncate = default_local_conf.log_rotation_truncate

        # Now, set up the environment.  TODO: move these in to actual
        # server parameters.
        server_env = _sys_util.make_unity_server_env()
        os.environ.update(server_env)
        for (k, v) in server_env.iteritems():
            os.putenv(k, v)

        # Try starting the server
        start_server(server_opts)

        self.started = True

    def stop(self):
        if self.started:
            stop_server()
            self.started = False

    def get_logger(self):
        return self.logger

    def log_progress_enabled(self):
        """ Return True if progress is enabled else False. """
        raise NotImplementedError

    def set_log_progress(self, enable):
        if enable:
            set_log_progress_callback(print_status)
            self._log_progress_enabled = True
        else:
            set_log_progress(False)
            self._log_progress_enabled = False

class QuietProgress(object):
    """
    Context manager facilitating the temporary suppression of progress logging.
    """

    def __init__(self, verbose):
        self.verbose = verbose
    def __enter__(self):
        server = _connect.main.get_server()
        self.log_progress_enabled = server.log_progress_enabled
        if not self.verbose:
            server.set_log_progress(False)

    def __exit__(self, type, value, traceback):
        _connect.main.get_server().set_log_progress(self.log_progress_enabled)
