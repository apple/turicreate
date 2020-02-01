# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

# Futuristic imports
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

# Built-in imports
import os
import signal
import time
import unittest
import uuid

# Library imports
import six

# Finally, the package under test
import turicreate as tc
from turicreate.toolkits._internal_utils import _mac_ver


class ExploreTest(unittest.TestCase):
    @unittest.skipIf(
        _mac_ver() < (10, 12), "macOS-only test; UISoup doesn't work on Linux"
    )
    @unittest.skipIf(
        _mac_ver() > (10, 13),
        "macOS 10.14 appears to have broken the UX flow to prompt for accessibility access",
    )
    @unittest.skipIf(not (six.PY2), "Python 2.7-only test; UISoup doesn't work on 3.x")
    def test_sanity_on_macOS(self):
        """
        Create a simple SFrame, containing a very unique string.
        Then, using uisoup, look for this string within a window
        and assert that it appears.
        """

        # Library imports
        from uisoup import uisoup

        # Generate some test data
        unique_str = repr(uuid.uuid4())
        sf = tc.SFrame({"a": [1, 2, 3], "b": ["hello", "world", unique_str]})

        # Run the explore view and make sure we can see our unique string
        sf.explore()
        time.sleep(2)

        window = None
        try:
            window = uisoup.get_window("Turi*Create*Visualization")
            result = window.findall(value=unique_str)
            self.assertEqual(
                len(result),
                1,
                (
                    "Expected to find exactly one element containing the unique"
                    "string %s."
                )
                % unique_str,
            )
            first = result[0]
            self.assertEqual(
                first.acc_name,
                unique_str,
                (
                    "Expected to find the unique string %s as the name of the found"
                    "element. Instead, got %s."
                )
                % (unique_str, first.acc_name),
            )

        finally:
            if window is not None:
                # Kill the explore process
                os.kill(window.proc_id, signal.SIGTERM)
