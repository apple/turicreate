# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import os
import uuid
import shutil
import sys

import pickle
from turicreate.util import _cloudpickle

import turicreate as tc
from turicreate import _gl_pickle as gl_pickle
from turicreate.util import _assert_sframe_equal as assert_sframe_equal

import os as _os


class GLPicklingTest(unittest.TestCase):
    def setUp(self):
        self.filename = str(uuid.uuid4())
        self.dir_mode = False

    def tearDown(self):
        if os.path.isdir(self.filename):
            shutil.rmtree(self.filename)
        elif os.path.exists(self.filename):
            os.remove(self.filename)

    def test_pickling_simple_types(self):

        obj_list = [
            1,
            "hello",
            5.0,
            (1, 2),
            ("i", "love", "cricket"),
            [1, 2, "hello"],
            [1.3, (1, 2), "foo"],
            ["bar", {"foo": "bar"}],
            {"cricket": "best-sport", "test": [1, 2, 3]},
            {"foo": 1.3},
        ]
        for obj in obj_list:
            pickler = gl_pickle.GLPickler(self.filename)
            pickler.dump(obj)
            pickler.close()
            unpickler = gl_pickle.GLUnpickler(self.filename)
            obj_ret = unpickler.load()
            unpickler.close()
            assert obj == obj_ret, "Failed pickling in %s (Got back %s)" % (
                obj,
                obj_ret,
            )

    def test_pickling_sarray_types(self):

        sarray_list = [
            tc.SArray([1, 2, 3]),
            tc.SArray([1.0, 2.0, 3.5]),
            tc.SArray(["foo", "bar"]),
        ]
        for obj in sarray_list:
            pickler = gl_pickle.GLPickler(self.filename)
            pickler.dump(obj)
            pickler.close()
            unpickler = gl_pickle.GLUnpickler(self.filename)
            obj_ret = unpickler.load()
            unpickler.close()
            assert list(obj) == list(obj_ret), "Failed pickling in %s (Got back %s)" % (
                obj,
                obj_ret,
            )

    def test_pickling_sframe_types(self):

        sarray_list = [
            tc.SFrame([1, 2, 3]),
            tc.SFrame([1.0, 2.0, 3.5]),
            tc.SFrame(["foo", "bar"]),
        ]
        for obj in sarray_list:
            pickler = gl_pickle.GLPickler(self.filename)
            pickler.dump(obj)
            pickler.close()
            unpickler = gl_pickle.GLUnpickler(self.filename)
            obj_ret = unpickler.load()
            unpickler.close()
            assert_sframe_equal(obj, obj_ret)

    def test_pickling_sgraph_types(self):

        sg_test_1 = tc.SGraph().add_vertices(
            [
                tc.Vertex(0, {"fluffy": 1}),
                tc.Vertex(1, {"fluffy": 1, "woof": 1}),
                tc.Vertex(2, {}),
            ]
        )

        sg_test_2 = tc.SGraph()
        sg_test_2 = sg_test_2.add_vertices([tc.Vertex(x) for x in [0, 1, 2]])
        sg_test_2 = sg_test_2.add_edges(
            [
                tc.Edge(0, 1, attr={"relationship": "dislikes"}),
                tc.Edge(1, 2, attr={"relationship": "likes"}),
                tc.Edge(1, 0, attr={"relationship": "likes"}),
            ]
        )

        sarray_list = [sg_test_1, sg_test_2]
        for obj in sarray_list:
            pickler = gl_pickle.GLPickler(self.filename)
            pickler.dump(obj)
            pickler.close()
            unpickler = gl_pickle.GLUnpickler(self.filename)
            obj_ret = unpickler.load()
            unpickler.close()
            assert_sframe_equal(obj.get_vertices(), obj_ret.get_vertices())
            assert_sframe_equal(obj.get_edges(), obj_ret.get_edges())

    def test_combination_gl_python_types(self):

        sg_test_1 = tc.SGraph().add_vertices(
            [
                tc.Vertex(1, {"fluffy": 1}),
                tc.Vertex(2, {"fluffy": 1, "woof": 1}),
                tc.Vertex(3, {}),
            ]
        )
        sarray_test_1 = tc.SArray([1, 2, 3])
        sframe_test_1 = tc.SFrame([1, 2, 3])

        obj_list = [
            [sg_test_1, sframe_test_1, sarray_test_1],
            {0: sg_test_1, 1: sframe_test_1, 2: sarray_test_1},
        ]

        for obj in obj_list:
            pickler = gl_pickle.GLPickler(self.filename)
            pickler.dump(obj)
            pickler.close()
            unpickler = gl_pickle.GLUnpickler(self.filename)
            obj_ret = unpickler.load()
            unpickler.close()
            assert_sframe_equal(obj[0].get_vertices(), obj_ret[0].get_vertices())
            assert_sframe_equal(obj[0].get_edges(), obj_ret[0].get_edges())
            assert_sframe_equal(obj[1], obj_ret[1])
            assert list(obj[2]) == list(obj_ret[2])

    def test_pickle_compatibility(self):
        obj_list = [
            1,
            "hello",
            5.0,
            (1, 2),
            ("i", "love", "cricket"),
            [1, 2, "hello"],
            [1.3, (1, 2), "foo"],
            ["bar", {"foo": "bar"}],
            {"cricket": "best-sport", "test": [1, 2, 3]},
            {"foo": 1.3},
        ]
        for obj in obj_list:
            file = open(self.filename, "wb")
            pickler = pickle.Pickler(file)
            pickler.dump(obj)
            file.close()
            unpickler = gl_pickle.GLUnpickler(self.filename)
            obj_ret = unpickler.load()
            unpickler.close()
            assert obj == obj_ret, "Failed pickling in %s (Got back %s)" % (
                obj,
                obj_ret,
            )

    def test_cloud_pickle_compatibility(self):
        obj_list = [
            1,
            "hello",
            5.0,
            (1, 2),
            ("i", "love", "cricket"),
            [1, 2, "hello"],
            [1.3, (1, 2), "foo"],
            ["bar", {"foo": "bar"}],
            {"cricket": "best-sport", "test": [1, 2, 3]},
            {"foo": 1.3},
        ]
        for obj in obj_list:
            file = open(self.filename, "wb")
            pickler = _cloudpickle.CloudPickler(file)
            pickler.dump(obj)
            file.close()
            unpickler = gl_pickle.GLUnpickler(self.filename)
            obj_ret = unpickler.load()
            unpickler.close()
            assert obj == obj_ret, "Failed pickling in %s (Got back %s)" % (
                obj,
                obj_ret,
            )

    def test_relative_path(self):
        # Arrange
        sf1 = tc.SFrame(range(10))
        relative_path = "tmp/%s" % self.filename

        # Act
        pickler = gl_pickle.GLPickler(relative_path)
        pickler.dump(sf1)
        pickler.close()
        unpickler = gl_pickle.GLUnpickler(relative_path)
        sf2 = unpickler.load()
        unpickler.close()

        # Assert
        assert_sframe_equal(sf1, sf2)

        # Clean up
        shutil.rmtree(relative_path)

    def test_save_over_previous(self):

        sarray_list = [
            tc.SFrame([1, 2, 3]),
            tc.SFrame([1.0, 2.0, 3.5]),
            tc.SFrame(["foo", "bar"]),
        ]
        for obj in sarray_list:
            pickler = gl_pickle.GLPickler(self.filename)
            pickler.dump(obj)
            pickler.close()

            unpickler = gl_pickle.GLUnpickler(self.filename)
            obj_ret = unpickler.load()
            unpickler.close()
            assert_sframe_equal(obj, obj_ret)

            pickler = gl_pickle.GLPickler(self.filename)
            pickler.dump(obj)
            pickler.close()
