# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as tc
import os
import unittest
from . import util
import shutil
import random
import sys

temp_number = random.randint(0, 2 ** 64)


class TestModel(unittest.TestCase):
    def __assert_model_equals__(self, m1, m2):
        self.assertEqual(type(m1), type(m2))
        self.assertSequenceEqual(m1._list_fields(), m2._list_fields())

    def __remove_file(self, path):
        if os.path.exists(path):
            shutil.rmtree(path)

    def setUp(self):

        self.pr_model = tc.pagerank.create(tc.SGraph())
        self.cc_model = tc.connected_components.create(tc.SGraph())

        self.__remove_file("~/tmp/tmp_model-%d" % temp_number)
        self.__remove_file("./tmp_model-%d" % temp_number)
        self.__remove_file("/tmp/tmp_model-%d" % temp_number)
        self.__remove_file("/tmp/tmp_model2-%d" % temp_number)

    def tearDown(self):
        self.__remove_file("~/tmp/tmp_model-%d" % temp_number)
        self.__remove_file("./tmp_model-%d" % temp_number)
        self.__remove_file("/tmp/tmp_model-%d" % temp_number)
        self.__remove_file("/tmp/tmp_model2-%d" % temp_number)

    @unittest.skip("failing since 08/30/2016")
    def test_basic_save_load(self):
        # save and load the pagerank model
        with util.TempDirectory() as tmp_pr_model_file:
            self.pr_model.save(tmp_pr_model_file)
            pr_model2 = tc.load_model(tmp_pr_model_file)
            self.__assert_model_equals__(self.pr_model, pr_model2)

        # save and load the connected_component model
        with util.TempDirectory() as tmp_cc_model_file:
            self.cc_model.save(tmp_cc_model_file)
            cc_model2 = tc.load_model(tmp_cc_model_file)
            self.__assert_model_equals__(self.cc_model, cc_model2)

        # handle different types of urls.
        # TODO: test hdfs and s3 urls.
        for url in [
            "./tmp_model-%d" % temp_number,
            "/tmp/tmp_model-%d" % temp_number,
            "~/tmp/tmp_model-%d" % temp_number,
        ]:

            self.pr_model.save(url)
            self.__assert_model_equals__(self.pr_model, tc.load_model(url))

    @unittest.skip("failing since 08/30/2016")
    def test_exception(self):
        # load model from empty file
        with util.TempDirectory() as tmp_empty_file:
            with self.assertRaises(IOError):
                tc.load_model(tmp_empty_file)

        # load model from non-existing file
        if os.path.exists("./tmp_model-%d" % temp_number):
            shutil.rmtree("./tmp_model-%d" % temp_number)
        with self.assertRaises(IOError):
            tc.load_model("./tmp_model-%d" % temp_number)

        # save model to invalid url
        restricted_place = None
        if sys.platform == "win32":
            restricted_place = "C:\\Windows\\System32\\config\\RegBack\\testmodel"
        else:
            restricted_place = "/root/tmp/testmodel"
        for url in ["http://test", restricted_place]:
            with self.assertRaises(IOError):
                self.pr_model.save(url)
