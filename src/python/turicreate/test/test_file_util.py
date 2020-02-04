# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import os
import unittest
import tempfile
from ..util import _file_util as fu


class FileUtilTests(unittest.TestCase):
    def setUp(self):
        self.local_path = "tmp/a/b/c"
        self.s3_path = "s3://a/b/c"
        self.http_path = "http://a.b.c/d"
        self._get_env()

    def _get_env(self):
        self.run_s3_test = (
            ("FILE_UTIL_TEST_S3_BUCKET" in os.environ)
            and "AWS_ACCESS_KEY_ID" in os.environ
            and "AWS_SECRET_ACCESS_KEY" in os.environ
        )

        if self.run_s3_test:
            self.s3_test_path = os.environ["FILE_UTIL_TEST_S3_BUCKET"]
        else:
            self.s3_test_path = None

    def test_get_protocol(self):
        self.assertEqual(fu.get_protocol(self.local_path), "")
        self.assertEqual(fu.get_protocol(self.s3_path), "s3")
        self.assertEqual(fu.get_protocol(self.http_path), "http")

    def test_is_local_path(self):
        self.assertTrue(fu.is_local_path(self.local_path))
        self.assertFalse(fu.is_local_path(self.s3_path))
        self.assertFalse(fu.is_local_path(self.http_path))

    def test_expand_full_path(self):
        if not "HOME" in os.environ:
            raise RuntimeError("warning: cannot find $HOME key in environment")
        else:
            home = os.environ["HOME"]
            self.assertTrue(fu.expand_full_path("~/tmp"), os.path.join(home, "tmp"))
            self.assertTrue(
                fu.expand_full_path("tmp"), os.path.join(os.getcwd(), "tmp")
            )
