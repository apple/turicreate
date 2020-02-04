# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from .. import SGraph, Edge
import unittest
import time

import sys

if sys.version_info.major > 2:
    unittest.TestCase.assertItemsEqual = unittest.TestCase.assertCountEqual


def degree_count_fn(source, edge, target, edge_dir, field):
    if field is None:
        target["in_degree"] += 1
        source["out_degree"] += 1
        target["all_degree"] += 1
        source["all_degree"] += 1
    else:
        if edge_dir is "in" or edge_dir is "all":
            target[field] = target[field] + 1
        if edge_dir is "out" or edge_dir is "all":
            source[field] = source[field] + 1
    return (source, edge, target)


def exception_fn(source, edge, target):
    raise RuntimeError


def return_none_fn(source, edge, target):
    return None


def return_pair_fn(source, edge, target):
    return (source, target)


class GraphTests(unittest.TestCase):
    def test_simple_triple_apply(self):
        def identity_fun(src, edge, dst):
            return src, edge, dst

        nverts = 100
        ring_graph = SGraph().add_edges([Edge(i, 0) for i in range(1, nverts)])
        ring_graph.vertices["id"] = ring_graph.vertices["__id"]
        ring_graph.edges["src"] = ring_graph.edges["__src_id"]
        ring_graph2 = ring_graph.triple_apply(identity_fun, ["id", "src"])

        self.assertSequenceEqual(
            list(ring_graph2.vertices["id"]), list(ring_graph2.vertices["__id"])
        )
        self.assertSequenceEqual(
            list(ring_graph2.edges["src"]), list(ring_graph2.edges["__src_id"])
        )
        for i in ring_graph.edges["__dst_id"]:
            self.assertEqual(i, 0)

    def test_triple_apply(self):
        nverts = 100
        ring_graph = SGraph().add_edges([Edge(i, 0) for i in range(1, nverts)])
        vdata = ring_graph.get_vertices()
        vdata["in_degree"] = 0
        vdata["out_degree"] = 0
        vdata["all_degree"] = 0
        vdata["do_not_touch"] = 0
        ring_graph = ring_graph.add_vertices(vdata)
        ret = ring_graph.triple_apply(
            lambda source, edge, target: degree_count_fn(
                source, edge, target, "in", "in_degree"
            ),
            mutated_fields=["in_degree"],
            input_fields=["in_degree"],
        )
        self.assertItemsEqual(
            ret.get_fields(), ["__id", "__src_id", "__dst_id", "in_degree"]
        )
        ret = ring_graph.triple_apply(
            lambda source, edge, target: degree_count_fn(
                source, edge, target, "out", "out_degree"
            ),
            mutated_fields=["out_degree"],
            input_fields=["out_degree"],
        )
        self.assertItemsEqual(
            ret.get_fields(), ["__id", "__src_id", "__dst_id", "out_degree"]
        )
        ret = ring_graph.triple_apply(
            lambda source, edge, target: degree_count_fn(
                source, edge, target, "all", "all_degree"
            ),
            mutated_fields=["all_degree"],
            input_fields=["all_degree"],
        )
        self.assertItemsEqual(
            ret.get_fields(), ["__id", "__src_id", "__dst_id", "all_degree"]
        )

        ring_graph = ring_graph.triple_apply(
            lambda source, edge, target: degree_count_fn(
                source, edge, target, "all", None
            ),
            ["in_degree", "out_degree", "all_degree"],
        )
        self.assertItemsEqual(
            ring_graph.get_fields(),
            [
                "__id",
                "__src_id",
                "__dst_id",
                "in_degree",
                "out_degree",
                "all_degree",
                "do_not_touch",
            ],
        )
        vdata = ring_graph.get_vertices()
        for v in vdata:
            if v["__id"] == 0:
                self.assertEqual(v["in_degree"], nverts - 1)
                self.assertEqual(v["out_degree"], 0)
            else:
                self.assertEqual(v["in_degree"], 0)
                self.assertEqual(v["out_degree"], 1)
            self.assertEqual(v["all_degree"], (v["in_degree"] + v["out_degree"]))

        # test lambda that changes fields that are not in the mutate_fields
        ring_graph = ring_graph.triple_apply(
            lambda source, edge, target: degree_count_fn(
                source, edge, target, "all", "do_not_touch"
            ),
            mutated_fields=["in_degree"],
        )
        vdata = ring_graph.get_vertices()
        for v in vdata:
            self.assertEqual(v["do_not_touch"], 0)
            self.assertEqual(v["all_degree"], (v["in_degree"] + v["out_degree"]))

        # test change edge data
        ring_graph.edges["src_id"] = 0
        ring_graph.edges["dst_id"] = 0

        def edge_update_fn(source, edge, target):
            edge["src_id"] = source["__id"]
            edge["dst_id"] = target["__id"]
            return (source, edge, target)

        ring_graph = ring_graph.triple_apply(
            edge_update_fn, mutated_fields=["src_id", "dst_id"]
        )
        edata = ring_graph.get_edges()
        for e in edata:
            self.assertEqual(e["__src_id"], e["src_id"])
            self.assertEqual(e["__dst_id"], e["dst_id"])

        # test exception in lambda
        self.assertRaises(
            RuntimeError,
            lambda: ring_graph.triple_apply(exception_fn, mutated_fields=["in_degree"]),
        )

        # test lambda that does not return a tuple of dicts
        self.assertRaises(
            RuntimeError,
            lambda: ring_graph.triple_apply(
                return_none_fn, mutated_fields=["in_degree"]
            ),
        )
        self.assertRaises(
            RuntimeError,
            lambda: ring_graph.triple_apply(
                return_pair_fn, mutated_fields=["in_degree"]
            ),
        )

        # test api input validation
        self.assertRaises(
            TypeError,
            lambda: ring_graph.triple_apply(exception_fn, mutated_fields=None),
        )
        self.assertRaises(
            TypeError,
            lambda: ring_graph.triple_apply(
                exception_fn, mutated_fields=["in_degree"], input_fields={"a": "b"}
            ),
        )
        self.assertRaises(
            ValueError, lambda: ring_graph.triple_apply(exception_fn, mutated_fields=[])
        )
        self.assertRaises(
            ValueError,
            lambda: ring_graph.triple_apply(
                exception_fn, mutated_fields=["field_not_exist"]
            ),
        )
        self.assertRaises(
            ValueError,
            lambda: ring_graph.triple_apply(exception_fn, mutated_fields=["__id"]),
        )
