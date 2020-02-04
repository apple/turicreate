# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from unittest import TestCase
import logging
from .. import config as tc_config


class LoggingConfigurationTests(TestCase):
    def setUp(self):
        """
        Cleanup the existing log configuration.
        """
        self.logger = logging.getLogger(tc_config._root_package_name)
        self.orig_handlers = self.logger.handlers
        self.logger.handlers = []
        self.level = self.logger.level
        self.logger.level = logging.DEBUG

        self.rt_logger = logging.getLogger()
        self.orig_root_handlers = self.rt_logger.handlers
        self.rt_logger.handlers = []
        self.root_level = self.rt_logger.level
        self.rt_logger.level = logging.CRITICAL

    def tearDown(self):
        """
        Restore the original log configuration.
        """
        self.logger.handlers = self.orig_handlers
        self.logger.level = self.level
        self.rt_logger.handlers = self.orig_root_handlers
        self.rt_logger.level = self.root_level

    def test_config(self):
        tc_config.init_logger()

        self.assertEqual(self.logger.level, logging.INFO)
        self.assertEqual(len(self.logger.handlers), 2)
        self.assertEqual(len(self.rt_logger.handlers), 0)

        self.assertEqual(self.logger.level, logging.INFO)
        self.assertEqual(self.rt_logger.level, logging.CRITICAL)
