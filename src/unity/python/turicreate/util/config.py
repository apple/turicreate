# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import os
import time
import logging
import platform
import sys as _sys

class TuriConfig:

    __slots__ = ['turicreate_server', 'server_addr', 'server_bin', 'log_dir', 
                 'log_rotation_interval','log_rotation_truncate']

    def __init__(self, server_addr=None):
        if not server_addr:
            server_addr = 'default'
        self.server_addr = server_addr
        gl_root = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..'))
        self.log_rotation_interval = 86400
        self.log_rotation_truncate = 8
        if "TURI_UNITY" in os.environ:
            self.server_bin = os.environ["TURI_UNITY"]
        elif os.path.exists(os.path.join(gl_root, "unity_server")):
            self.server_bin = os.path.join(gl_root, "unity_server")
        elif os.path.exists(os.path.join(gl_root, "unity_server.exe")):
            self.server_bin = os.path.join(gl_root, "unity_server.exe")

        if "TURI_LOG_ROTATION_INTERVAL" in os.environ:
            tmp = os.environ["TURI_LOG_ROTATION_INTERVAL"]
            try:
                self.log_rotation_interval = int(tmp)
            except:
                logging.getLogger(__name__).warning("TURI_LOG_ROTATION_INTERVAL must be an integral value")

        if "TURI_LOG_ROTATION_TRUNCATE" in os.environ:
            tmp = os.environ["TURI_LOG_ROTATION_TRUNCATE"]
            try:
                self.log_rotation_truncate = int(tmp)
            except:
                logging.getLogger(__name__).warning("TURI_LOG_ROTATION_TRUNCATE must be an integral value")

        if "TURI_LOG_PATH" in os.environ:
            log_dir = os.environ["TURI_LOG_PATH"]
        else:
            if platform.system() == "Windows":
                if "TEMP" in os.environ:
                    log_dir = os.environ["TEMP"]
                else:
                    raise RuntimeError("Please set the TEMP environment variable")
            else:
                log_dir = "/tmp"

        self.log_dir = log_dir
        ts = str(int(time.time()))
        root_package_name = __import__(__name__.split('.')[0]).__name__

        # NOTE: Remember to update slots if you are adding any config parameters to this file.

    def get_unity_log(self):
        ts = str(int(time.time()))
        log_ext = '.log'
        root_package_name = __import__(__name__.split('.')[0]).__name__
        return os.path.join(self.log_dir, root_package_name + '_server_' + str(ts) + log_ext)

DEFAULT_CONFIG = TuriConfig()
