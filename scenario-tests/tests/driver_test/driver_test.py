# -*- coding: utf-8 -*-
import os
import unittest
class DriverTest(unittest.TestCase):
    def test_success(self):
        pass

    def test_skip(self):
        raise unittest.SkipTest("expected skip")

    def test_driver_passthrough(self):
        self.assertTrue(os.environ.get("TEST_ENVIRON_SET") is None)
