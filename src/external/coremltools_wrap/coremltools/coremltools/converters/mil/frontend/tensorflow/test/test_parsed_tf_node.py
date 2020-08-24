# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import unittest
import pytest

pytest.importorskip("tensorflow", minversion="1.14.0")
from tensorflow.core.framework import node_def_pb2 as node_def
from tensorflow.core.framework import tensor_shape_pb2 as tensor_shape
from tensorflow.core.framework import types_pb2 as types

from coremltools.converters.mil.frontend.tensorflow.parsed_tf_node import ParsedTFNode


def _mock_tf_node():
    tfnode = node_def.NodeDef()
    tfnode.name = "aNode"
    tfnode.op = "PlaceholderWithDefault"
    tfnode.input.extend(["anInput", "^aControlInput"])
    tfnode.attr["dtype"].type = types.DataType.DT_INT32
    dims = [(1, "outer"), (2, "middle"), (3, "inner")]
    for (dim_size, dim_name) in dims:
        tf_dim = tensor_shape.TensorShapeProto.Dim()
        tf_dim.size = dim_size
        tf_dim.name = dim_name
        tfnode.attr["shape"].shape.dim.append(tf_dim)
    return tfnode


class TestParsedTFNode(unittest.TestCase):
    def test_init(self):
        parsed_node = ParsedTFNode(_mock_tf_node())
        parsed_node.parse_from_attr()
        self.assertEqual("aNode", parsed_node.name)
        self.assertEqual("Placeholder", parsed_node.op)
        self.assertEqual(["anInput"], parsed_node.inputs)
        self.assertEqual(["aControlInput"], parsed_node.control_inputs)

    def test_copy(self):
        parsed_node = ParsedTFNode(_mock_tf_node())
        parsed_node.parse_from_attr()
        copy = parsed_node.copy()
        self.assertTrue(isinstance(copy, type(parsed_node)))
        props = [
            "name",
            "op",
            "datatype",
            "value",
            "inputs",
            "control_inputs",
            "outputs",
            "control_outputs",
            "attr",
            "original_node",
        ]
        for prop in props:
            self.assertEqual(
                getattr(parsed_node, prop),
                getattr(copy, prop),
                "Mismatch in property {}".format(prop),
            )
