# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
from turicreate._deps import __get_version as get_version
from distutils.version import StrictVersion


class VersionTest(unittest.TestCase):
    def test_min_version(self):
        MIN_VERSION = StrictVersion("1.8.1")
        self.assertEqual(get_version("1.8.1"), MIN_VERSION)
        self.assertEqual(get_version("1.8.1-dev"), MIN_VERSION)
        self.assertEqual(get_version("1.8.1rc"), MIN_VERSION)

        self.assertLess(get_version("1.8.0"), MIN_VERSION)
        self.assertLess(get_version("1.8.0-dev"), MIN_VERSION)
        self.assertLess(get_version("1.8.0rc"), MIN_VERSION)

        self.assertLess(get_version("1.6.2"), MIN_VERSION)
        self.assertLess(get_version("1.6.2-dev"), MIN_VERSION)
        self.assertLess(get_version("1.6.2rc"), MIN_VERSION)

        self.assertGreater(get_version("1.9.0"), MIN_VERSION)
        self.assertGreater(get_version("1.9.0-dev"), MIN_VERSION)
        self.assertGreater(get_version("1.9.0rc"), MIN_VERSION)

        self.assertGreater(get_version("1.8.2"), MIN_VERSION)
        self.assertGreater(get_version("1.8.2-dev"), MIN_VERSION)
        self.assertGreater(get_version("1.8.2rc"), MIN_VERSION)
