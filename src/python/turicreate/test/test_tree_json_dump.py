# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as tc
import unittest
import json
import random
import numpy

import os as _os


class Config:
    is_debug = False  # always set this to False in production
    level = 0

    def __init__(self):
        pass


class GBDTNode:
    NODE_TYPE_INT = 1
    NODE_TYPE_INDICATOR = 2
    NODE_TYPE_LEAF = 3
    type2typeid = {
        "indicator": NODE_TYPE_INDICATOR,
        "integer": NODE_TYPE_INT,
        "leaf": NODE_TYPE_LEAF,
    }
    typeid2type = {
        NODE_TYPE_INDICATOR: "indicator",
        NODE_TYPE_INT: "integer",
        NODE_TYPE_LEAF: "leaf",
    }

    def __init__(self, node_type, value, gid, name, left=None, right=None):
        self.node_type = node_type
        self.value = numpy.float32(value)
        self.gid = gid
        self.node_name = name
        self.left = left
        self.right = right

    @classmethod
    def load_vertex(cls, v):
        node_type = GBDTNode.type2typeid[v["type"]]
        name = v["name"] if (node_type != GBDTNode.NODE_TYPE_LEAF) else ""
        gid = v["id"]
        value = v["value"]
        return GBDTNode(node_type, value, gid, name)

    @classmethod
    def load_turicreate_json_tree(cls, jstree):
        vertices = jstree["vertices"]
        edges = jstree["edges"]
        vtup = map(lambda x: (x["id"], GBDTNode.load_vertex(x)), vertices)
        id2ver = dict(vtup)
        for e in edges:
            src = id2ver[e["src"]]
            dst = id2ver[e["dst"]]
            val = e["value"]
            if val == "yes":
                src.left = dst
            else:
                src.right = dst
        r = id2ver[0]  # root node
        return r

    def __str__(self):
        n = "%d:%s" % (self.gid, self.node_name)
        if self.node_type == GBDTNode.NODE_TYPE_INDICATOR:
            n += "="
        elif self.node_type == GBDTNode.NODE_TYPE_INT:
            n += "<"
        if self.node_type == GBDTNode.NODE_TYPE_INT:
            n += "%f" % self.value
        else:
            n += "%s" % self.value
        left = "_"
        if self.left is not None:
            if isinstance(self.left, GBDTNode):
                left = "%d" % self.left.gid
            else:
                left = self.left
        right = "_"
        if self.right is not None:
            if isinstance(self.right, GBDTNode):
                right = "%d" % self.right.gid
            else:
                right = self.right
        n += ",%s,%s" % (left, right)
        return n

    def traverse(self, level=""):
        print(level + str(self))
        this_level = level
        if self.left:
            if isinstance(self.left, GBDTNode):
                self.left.traverse(level=this_level + " ")
        if self.right:
            if isinstance(self.right, GBDTNode):
                self.right.traverse(level=this_level + " ")

    def renumbering(self, val):
        self.gid = val
        start_number = val + 1
        if self.left:
            start_number = self.left.renumbering(start_number)
        if self.right:
            start_number = self.right.renumbering(start_number)
        return start_number

    def calculate_score(self, inp):
        if Config.is_debug:
            if Config.level == 0:
                print("=====================")
            print((" " * Config.level) + str(self))
            Config.level += 1
        if self.node_type == GBDTNode.NODE_TYPE_LEAF:
            retval = self.value
        elif self.node_type == GBDTNode.NODE_TYPE_INT:
            inpval = inp[self.node_name]
            if inpval < self.value:
                retval = self.left.calculate_score(inp)
            else:
                retval = self.right.calculate_score(inp)
        elif self.node_type == GBDTNode.NODE_TYPE_INDICATOR:
            strkey, strval = self.node_name.split("=")
            inpval = str(inp[strkey])
            if inpval == strval:
                retval = self.left.calculate_score(inp)
            else:
                retval = self.right.calculate_score(inp)
        if Config.is_debug:
            Config.level -= 1
        return retval

    def get_dict(self):
        d = {
            "id": self.gid,
            "type": GBDTNode.typeid2type[self.node_type],
            "value": self.value,
        }
        if self.node_type != GBDTNode.NODE_TYPE_LEAF:
            d["name"] = self.node_name
        return d

    def get_all_vertice_helper(self, vhash):
        vhash[self.gid] = self.get_dict()
        if self.left:
            if not self.left.gid in vhash:
                self.left.get_all_vertice_helper(vhash)
        if self.right:
            if not self.right.gid in vhash:
                self.right.get_all_vertice_helper(vhash)

    def get_all_vertices(self):
        vhash = {}
        self.get_all_vertice_helper(vhash)
        vlist = vhash.values()
        return vlist

    def get_all_edge_helper(self, visited, edge_lst):
        visited.add(self.gid)
        if self.left:
            edge_lst.append({"src": self.gid, "dst": self.left.gid, "value": "yes"})
            if self.left.gid not in visited:
                self.left.get_all_edge_helper(visited, edge_lst)

        if self.right:
            edge_lst.append({"src": self.gid, "dst": self.right.gid, "value": "no"})
            if self.right.gid not in visited:
                self.right.get_all_edge_helper(visited, edge_lst)

    def get_all_edges(self):
        edge_list = []
        visited = set()
        self.get_all_edge_helper(visited, edge_list)
        return edge_list

    def to_dict(self):
        all_vertices = self.get_all_vertices()
        all_edges = self.get_all_edges()
        ret_dict = {"vertices": all_vertices, "edges": all_edges}
        return ret_dict

    def is_leaf(self):
        if self.node_type == GBDTNode.NODE_TYPE_LEAF:
            return True
        else:
            if (self.left is None) and (self.right is None):
                raise Exception("Strange tree node %s" % self.node_name)
        return False


class GBDTDecoder:
    def __init__(self):
        self.tree_list = []

    def predict_one(self, record):
        score_list = [x.calculate_score(record) for x in self.tree_list]
        score = numpy.sum(score_list) + numpy.float32(0.5)
        return score

    @classmethod
    def parse_turicreate_json(cls, jslst):
        treelst = map(lambda js: GBDTNode.load_turicreate_json_tree(js), jslst)
        return treelst

    def predict(self, data):
        lst = []
        for row in data:
            pred = self.predict_one(row)
            lst.append(pred)
        return lst

    @classmethod
    def create_from_gbdt(cls, gbdt_model):
        jss = [json.loads(s) for s in gbdt_model._get("trees_json")]
        retval = GBDTDecoder()
        retval.combination_method = 0
        retval.tree_list = list(GBDTDecoder.parse_turicreate_json(jss))
        return retval

    @classmethod
    def create_from_gbdt_json(cls, gbdt_json):
        retval = GBDTDecoder()
        retval.combination_method = 0
        retval.tree_list = list(GBDTDecoder.parse_turicreate_json(gbdt_json))
        return retval

    def get_json_trees(self):
        dict_trees = map(lambda x: x.to_dict(), self.tree_list)
        js_trees = map(lambda x: json.dumps(x), dict_trees)
        # ret_json = json.dumps(js_trees)
        return js_trees

    def get(self, key_to_get):
        if key_to_get == "trees_json":
            return self.get_json_trees()
        else:
            raise Exception("Not implemented yet (get key = %s)" % key_to_get)

    def save_json(self, fpath):
        fp = open(fpath, "wt")
        strjs = json.dumps(self.get_json_trees())
        fp.write(strjs)
        fp.close()

    def save(self):
        raise Exception("Binary dump is not implemented yet. Please use save_json")


class TreeJsonDumpTest(unittest.TestCase):
    DELTA = 1e-5

    def _check_json_model_predict_consistency(self, glc_model, test_data):
        json_model = GBDTDecoder.create_from_gbdt(glc_model)
        glc_pred = list(glc_model.predict(test_data))
        json_pred = list(json_model.predict(test_data))
        max_diff = max([abs(x - y) for x, y in zip(glc_pred, json_pred)])
        self.assertAlmostEqual(max_diff, 0.0, delta=self.DELTA)

        # Test high precision json
        trees_json = glc_model._dump_to_json(with_stats=False)
        json_model = GBDTDecoder.create_from_gbdt_json(trees_json)
        glc_pred = list(glc_model.predict(test_data))
        json_pred = list(json_model.predict(test_data))
        max_diff = max([abs(x - y) for x, y in zip(glc_pred, json_pred)])
        self.assertEqual(max_diff, 0.0)

    def test_synthetic_data(self):
        random.seed(0)
        num_rows = 1000
        sf = tc.SFrame(
            {
                "num": [random.randint(0, 100) for i in range(num_rows)],
                "cat": [["a", "b"][random.randint(0, 1)] for i in range(num_rows)],
            }
        )
        coeffs = [random.random(), random.random()]

        def make_target(row):
            if row["cat"] == "a":
                return row["num"] * coeffs[0]
            else:
                return row["num"] * coeffs[1]

        sf["target"] = sf.apply(make_target)
        m = tc.boosted_trees_regression.create(
            sf,
            target="target",
            validation_set=None,
            random_seed=0,
            max_depth=10,
            max_iterations=3,
        )
        self._check_json_model_predict_consistency(m, sf)
