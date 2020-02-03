# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
# This software may be modified and distributed under the terms
# of the BSD license. See the LICENSE file for details.
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from .._sys_util import get_config_file
from .._sys_util import setup_environment_from_config_file
from .._sys_util import write_config_file_value

import unittest
import tempfile

from os.path import join
import os
import shutil


class EnvironmentConfigTester(unittest.TestCase):
    def test_config_basic_write(self):

        test_dir = tempfile.mkdtemp()
        config_file = join(test_dir, "test_config")
        os.environ["TURI_CONFIG_FILE"] = config_file

        try:
            self.assertEqual(get_config_file(), config_file)
            write_config_file_value("TURI_FILE_TEST_VALUE", "this-is-a-test")
            setup_environment_from_config_file()
            self.assertEqual(os.environ["TURI_FILE_TEST_VALUE"], "this-is-a-test")
        finally:
            shutil.rmtree(test_dir)
            del os.environ["TURI_CONFIG_FILE"]
