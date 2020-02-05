# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ..data_structures.sgraph import SGraph, Vertex, Edge, load_sgraph
from ..data_structures.sframe import SFrame
from . import util

import pandas as pd
from pandas.util.testing import assert_frame_equal
import numpy as np
import unittest
import tempfile
import json
import os

import sys

if sys.version_info.major > 2:
    unittest.TestCase.assertItemsEqual = unittest.TestCase.assertCountEqual


class GraphTests(unittest.TestCase):
    def setUp(self):
        self.vertices = pd.DataFrame(
            {
                "vid": ["1", "2", "3"],
                "color": ["g", None, "b"],
                "vec": [[0.1, 0.1, 0.1], [0.1, 0.1, 0.1], [0.1, 0.1, 0.1]],
            }
        )
        self.edges = pd.DataFrame(
            {
                "src_id": ["1", "2", "3"],
                "dst_id": ["2", "3", "4"],
                "weight": [0.0, None, 1.0],
            }
        )

    def test_empty_graph(self):
        g = SGraph()
        self.assertEqual(g.summary(), {"num_vertices": 0, "num_edges": 0})
        self.assertEqual(len(g.get_fields()), 3)
        self.assertTrue(g.get_vertices(format="sframe").shape, (0, 1))
        self.assertTrue(g.get_edges(format="sframe").shape, (0, 2))
        self.assertTrue(g.vertices.shape, (0, 1))
        self.assertTrue(g.edges.shape, (0, 2))
        self.assertTrue(len(g.get_vertices(format="list")) == 0)
        self.assertTrue(len(g.get_edges(format="list")) == 0)

    def test_graph_constructor(self):
        g = (
            SGraph()
            .add_vertices(self.vertices, "vid")
            .add_edges(self.edges, "src_id", "dst_id")
        )
        g2 = SGraph(g.vertices, g.edges)
        g3 = SGraph(
            g.vertices, g.edges, src_field="__dst_id", dst_field="__src_id"
        )  # flip around src and dst
        assert_frame_equal(
            g.vertices.to_dataframe().sort_values("__id").reset_index(drop=True),
            g2.vertices.to_dataframe().sort_values("__id").reset_index(drop=True),
        )
        assert_frame_equal(
            g.edges.to_dataframe()
            .sort_values(["__src_id", "__dst_id"])
            .reset_index(drop=True),
            g2.edges.to_dataframe()
            .sort_values(["__src_id", "__dst_id"])
            .reset_index(drop=True),
        )
        self.assertRaises(
            ValueError, lambda: SGraph(SFrame(self.vertices), SFrame(self.edges))
        )
        self.assertRaises(
            ValueError,
            lambda: SGraph(
                SFrame(self.vertices), SFrame(self.edges), "vid", "__src_id", "__dst_id"
            ),
        )
        self.assertRaises(
            ValueError,
            lambda: SGraph(
                SFrame(self.vertices),
                SFrame(self.edges),
                vid_field=None,
                src_field="src_id",
                dst_field="dst_id",
            ),
        )

    def test_simple_graph(self):
        for input_type in [pd.DataFrame, SFrame, list]:
            g = SGraph()
            if input_type is list:
                vertices = [
                    Vertex(x[1]["vid"], {"color": x[1]["color"], "vec": x[1]["vec"]})
                    for x in self.vertices.iterrows()
                ]
                edges = [
                    Edge(x[1]["src_id"], x[1]["dst_id"], {"weight": x[1]["weight"]})
                    for x in self.edges.iterrows()
                ]
                g = g.add_vertices(vertices)
                g = g.add_edges(edges)
            else:
                g = g.add_vertices(input_type(self.vertices), vid_field="vid")
                g = g.add_edges(
                    input_type(self.edges), src_field="src_id", dst_field="dst_id"
                )
            self.assertEqual(g.summary(), {"num_vertices": 4, "num_edges": 3})
            self.assertItemsEqual(
                g.get_fields(),
                ["__id", "__src_id", "__dst_id", "color", "vec", "weight"],
            )
            self.assertItemsEqual(
                g.get_vertices(format="dataframe").columns.values, ["color", "vec"]
            )
            self.assertItemsEqual(
                g.get_edges(format="dataframe").columns.values,
                ["__src_id", "__dst_id", "weight"],
            )
            self.assertTrue(g.get_edges(format="dataframe").shape, (3, 3))
            self.assertTrue(g.get_vertices(format="dataframe").shape, (4, 3))
            self.assertTrue(
                g.get_vertices(format="dataframe", fields={"color": "g"}).shape, (1, 2)
            )
            self.assertTrue(
                g.get_edges(format="dataframe", fields={"weight": 0.0}).shape, (1, 3)
            )

            self.assertItemsEqual(
                g.get_vertices(format="sframe").column_names(), ["__id", "color", "vec"]
            )
            self.assertItemsEqual(
                g.get_edges(format="sframe").column_names(),
                ["__src_id", "__dst_id", "weight"],
            )
            self.assertTrue(g.get_edges(format="sframe").shape, (3, 3))
            self.assertTrue(g.get_vertices(format="sframe").shape, (4, 3))
            self.assertTrue(
                g.get_vertices(format="sframe", fields={"color": "g"}).shape, (1, 2)
            )
            self.assertTrue(
                g.get_edges(format="sframe", fields={"weight": 0.0}).shape, (1, 3)
            )

            vertices = g.get_vertices(format="list")
            edges = g.get_edges(format="list")
            self.assertEqual(len(vertices), 4)
            self.assertEqual(len(edges), 3)

            # get edges is lazy
            edges = g.get_edges()
            self.assertFalse(edges.__is_materialized__())

    def test_vertex_query(self):
        df = pd.DataFrame(
            {
                "src": ["a", "c", "b", "d", "c", "e", "g", "f"],
                "dst": ["b", "b", "d", "c", "e", "g", "f", "e"],
            }
        )
        g = SGraph().add_edges(df, src_field="src", dst_field="dst")

        # basic check
        g2 = g.get_neighborhood(ids=["b"], radius=1, full_subgraph=False)
        out = g2.get_edges(format="dataframe")
        out.sort_values(by=["__src_id", "__dst_id"], axis=0, inplace=True)
        out.index = range(len(out))

        correct = pd.DataFrame.from_records(
            [("b", "d"), ("a", "b"), ("c", "b")], columns=["__src_id", "__dst_id"]
        )
        correct.sort_values(by=["__src_id", "__dst_id"], axis=0, inplace=True)
        correct.index = range(len(correct))
        assert_frame_equal(out, correct, check_dtype=False)

        # check larger radius, full subgraph, and multiple vertices
        g2 = g.get_neighborhood(ids=["a", "g"], radius=2, full_subgraph=True)
        out = g2.get_edges(format="dataframe")
        out.sort_values(by=["__src_id", "__dst_id"], axis=0, inplace=True)
        out.index = range(len(out))

        correct = pd.DataFrame.from_records(
            [
                ("a", "b"),
                ("b", "d"),
                ("c", "b"),
                ("c", "e"),
                ("d", "c"),
                ("e", "g"),
                ("f", "e"),
                ("g", "f"),
            ],
            columns=["__src_id", "__dst_id"],
        )
        correct.sort_values(by=["__src_id", "__dst_id"], axis=0, inplace=True)
        correct.index = range(len(correct))
        assert_frame_equal(out, correct, check_dtype=False)

    def test_select_query(self):
        g = SGraph()
        g = g.add_vertices(self.vertices, "vid").add_edges(
            self.edges, "src_id", "dst_id"
        )
        g2 = g.select_fields(["color", "weight"])
        self.assertSequenceEqual(
            (g2.get_fields()), ["__id", "color", "__src_id", "__dst_id", "weight"]
        )
        g2 = g.select_fields(["color"])
        self.assertSequenceEqual(
            (g2.get_fields()), ["__id", "color", "__src_id", "__dst_id"]
        )
        del g.edges["weight"]
        del g.vertices["vec"]
        g.vertices["color2"] = g.vertices["color"]
        self.assertSequenceEqual(
            (g.get_fields()), ["__id", "color", "color2", "__src_id", "__dst_id"]
        )
        g2 = g.select_fields([])
        self.assertSequenceEqual((g2.get_fields()), ["__id", "__src_id", "__dst_id"])

    def test_select_query_with_same_vertex_edge_field(self):
        vertices = SFrame({"__id": range(10)})
        edges = SFrame({"__src_id": range(10), "__dst_id": range(1, 11)})
        g = SGraph(vertices, edges)
        g.vertices["weight"] = 0
        g.vertices["v"] = 0
        g.edges["weight"] = 0
        g.edges["e"] = 0
        self.assertItemsEqual(
            g.get_fields(),
            ["v", "e", "weight", "weight", "__id", "__src_id", "__dst_id"],
        )
        g2 = g.select_fields("weight")
        self.assertItemsEqual(
            g2.get_fields(), ["weight", "weight", "__id", "__src_id", "__dst_id"]
        )

    def test_save_load(self):
        g = (
            SGraph()
            .add_vertices(self.vertices, "vid")
            .add_edges(self.edges, "src_id", "dst_id")
        )
        with util.TempDirectory() as f:
            g.save(f)
            g2 = load_sgraph(f, "binary")
            self.assertEqual(g2.summary(), {"num_vertices": 4, "num_edges": 3})
            self.assertItemsEqual(
                g2.get_fields(),
                {"__id", "__src_id", "__dst_id", "color", "vec", "weight"},
            )

        with util.TempDirectory() as f:
            g.save(f, format="csv")
            vertices = SFrame.read_csv(f + "/vertices.csv")
            edges = SFrame.read_csv(f + "/edges.csv")
            g2 = (
                SGraph()
                .add_edges(edges, "__src_id", "__dst_id")
                .add_vertices(vertices, "__id")
            )
            self.assertEqual(g2.summary(), {"num_vertices": 4, "num_edges": 3})
            self.assertItemsEqual(
                g2.get_fields(),
                {"__id", "__src_id", "__dst_id", "color", "vec", "weight"},
            )

        temp_fn = None
        # The delete=False is for Windows sake
        with tempfile.NamedTemporaryFile(suffix=".json", delete=False) as f:
            temp_fn = f.name
            g.save(f.name)
            with open(f.name, "r") as f2:
                data = f2.read()
                g2 = json.loads(data)
            self.assertTrue("vertices" in g2)
            self.assertTrue("edges" in g2)
        if os.path.exists(temp_fn):
            os.remove(temp_fn)

    def test_load_graph_from_text(self):
        toy_graph_snap = """#some comment string
                         #some more comment string
                         1\t2
                         1\t3
                         2\t3
                         2\t1
                         3\t1
                         3\t2"""

        toy_graph_tsv = """1\t2
                        1\t3
                        2\t3
                        2\t1
                        3\t1
                        3\t2"""
        toy_graph_csv = """1,2
                        1,3
                        2,3
                        2,1
                        3,1
                        3,2"""

        temp_fnames = []
        with tempfile.NamedTemporaryFile(
            mode="w", delete=False
        ) as fsnap, tempfile.NamedTemporaryFile(
            mode="w", delete=False
        ) as ftsv, tempfile.NamedTemporaryFile(
            mode="w", delete=False
        ) as fcsv:
            fsnap.write(toy_graph_snap)
            fsnap.file.flush()
            ftsv.write(toy_graph_tsv)
            ftsv.file.flush()
            fcsv.write(toy_graph_csv)
            fcsv.file.flush()
            for (fname, fmt) in zip(
                [fsnap.name, ftsv.name, fcsv.name], ["snap", "tsv", "csv"]
            ):
                g = load_sgraph(fname, fmt)
                self.assertEqual(g.summary(), {"num_vertices": 3, "num_edges": 6})
                temp_fnames.append(fname)

        for name in temp_fnames:
            if os.path.exists(name):
                os.remove(name)

    def test_robust_parse(self):
        df = pd.DataFrame(
            {
                "int": [1, 2, 3],
                "float": [1.0, 2.0, 3.0],
                "str": ["one", "two", "three"],
                "nan": [np.nan, np.nan, np.nan],
                "sparse_int": [1, 2, np.nan],
                "sparse_float": [np.nan, 2.0, 3.0],
                "sparse_str": [None, "two", None],
            }
        )
        g = SGraph().add_vertices(df)
        self.assertItemsEqual(
            g.get_fields(), df.columns.tolist() + ["__id", "__src_id", "__dst_id"]
        )

        df2 = g.get_vertices(format="dataframe")
        sf = g.get_vertices(format="sframe")
        for col in df.columns:
            # potential bug: df2 is missing the 'nan' column.
            if col != "nan":
                self.assertItemsEqual(
                    sorted(list(df2[col].dropna())), sorted(list(df[col].dropna()))
                )
                self.assertItemsEqual(
                    sorted(list(sf[col].dropna())), sorted(list(df[col].dropna()))
                )

    def test_missing_value_vids(self):
        vertices = SFrame()
        vertices["vid"] = [1, 2, 3, None]
        edges = SFrame()
        edges["src"] = [1, 2, 3, None]
        edges["dst"] = [4, 4, 4, 4]
        self.assertRaises(
            RuntimeError, lambda: SGraph().add_vertices(vertices, "vid").summary()
        )
        self.assertRaises(
            RuntimeError, lambda: SGraph().add_edges(edges, "src", "dst").summary()
        )
        self.assertRaises(
            RuntimeError, lambda: SGraph().add_edges(edges, "dst", "src").summary()
        )

    def test_gframe(self):
        g = SGraph()
        v = g.vertices
        self.assertSequenceEqual(v.column_names(), ["__id"])
        e = g.edges
        self.assertSequenceEqual(e.column_names(), ["__src_id", "__dst_id"])

        # Test vertices and edge attributes cannot be modified
        def set_vertices_empty(g):
            g.vertices = SFrame()

        def set_edges_empty(g):
            g.edges = SFrame()

        def remove_vertices(g):
            del g.vertices

        def remove_edges(g):
            del g.edges

        def remove_edge_column(gf, name):
            del gf[name]

        self.assertRaises(AttributeError, lambda: remove_vertices(g))
        self.assertRaises(AttributeError, lambda: remove_edges(g))
        self.assertRaises(AttributeError, lambda: set_vertices_empty(g))
        self.assertRaises(AttributeError, lambda: set_edges_empty(g))

        # Test gframe operations has the same effect as its sframe+graph equivalent
        g = (
            SGraph()
            .add_vertices(self.vertices, "vid")
            .add_edges(self.edges, "src_id", "dst_id")
        )
        v = g.vertices
        v["id_col"] = v["__id"]
        e = g.edges
        e["src_id_col"] = e["__src_id"]
        e["dst_id_col"] = e["__dst_id"]
        g2 = (
            SGraph()
            .add_vertices(self.vertices, "vid")
            .add_edges(self.edges, "src_id", "dst_id")
        )
        new_vdata = g2.get_vertices()
        new_vdata["id_col"] = new_vdata["__id"]
        new_edata = g2.get_edges()
        new_edata["src_id_col"] = new_edata["__src_id"]
        new_edata["dst_id_col"] = new_edata["__dst_id"]
        g2 = (
            SGraph()
            .add_vertices(new_vdata, "__id")
            .add_edges(new_edata, "__src_id", "__dst_id")
        )
        assert_frame_equal(
            g.get_vertices().to_dataframe().sort_values("__id").reset_index(drop=True),
            g2.get_vertices().to_dataframe().sort_values("__id").reset_index(drop=True),
        )
        assert_frame_equal(
            g.get_edges()
            .to_dataframe()
            .sort_values(["__src_id", "__dst_id"])
            .reset_index(drop=True),
            g2.get_edges()
            .to_dataframe()
            .sort_values(["__src_id", "__dst_id"])
            .reset_index(drop=True),
        )

        # check delete a column with exception, and edges is still in a valid state
        self.assertRaises(KeyError, lambda: remove_edge_column(g.edges, "badcolumn"))
        g.edges.head()

        # test slicing
        assert_frame_equal(g.edges[:3].to_dataframe(), g.get_edges()[:3].to_dataframe())
        assert_frame_equal(
            g.vertices[:3].to_dataframe(), g.get_vertices()[:3].to_dataframe()
        )

        # test add row number
        e_expected = g.get_edges().to_dataframe()
        v_expected = g.get_vertices().to_dataframe()
        e_expected["id"] = range(len(e_expected))
        v_expected["id"] = range(len(v_expected))

    def test_sframe_le_append_skip_row_bug_is_fixed(self):
        """
        This test is actually for SFrame lazy evaluation.
        The reason it is here is because the repro can only be done in SGraph.

        The bug appears when the SFrame has lazy_append and when passing through
        the logical filter, skip_rows is not done correctly. So the edge_sframe
        is in a bad state when not materialized.

        This unit test stays here to ensure the bug is fixed until we can find
        a more clean repro.
        """
        n = 12  # smallest n to repro the le_append bug

        # A graph with edge i -> i + 1
        g = SGraph().add_edges(
            SFrame({"src": range(n), "dst": range(1, n + 1)}), "src", "dst"
        )

        lazy_sf = g.get_edges()
        materialized_sf = g.get_edges()
        materialized_sf.materialize()
        assert_frame_equal(
            lazy_sf[lazy_sf["__dst_id"] == n].to_dataframe(),
            materialized_sf[materialized_sf["__dst_id"] == n].to_dataframe(),
        )
