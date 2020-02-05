# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Created on Aug 5, 2011

@author: sean
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import unittest
import ast
from ...asttools.mutators.remove_trivial import remove_trivial
from ...asttools.tests import assert_ast_eq, skip_networkx
from ...asttools.visitors.graph_visitor import GraphGen


def simple_case(self, toremove, expected):

    root = ast.parse(toremove)

    remove_trivial(root)

    expected_root = ast.parse(expected)

    assert_ast_eq(self, root, expected_root)


@skip_networkx
class Test(unittest.TestCase):
    def assertRemoved(self, toremove, expected):

        root = ast.parse(toremove)

        remove_trivial(root)

        expected = ast.parse(expected)

        assert_ast_eq(self, root, expected)

    def test_single(self):
        simple_case(self, "a = 1", "a = 1")

    def test_empty(self):
        simple_case(self, "", "")

    def test_simple(self):
        simple_case(self, "a = 1; a = 2", "pass; a = 2")

    def test_multi(self):
        simple_case(self, "a = 1; a = 2; a = 3", "pass; pass; a = 3")

    def test_apart(self):
        simple_case(self, "a = 1; b = 1; a = 2", "pass; b = 1; a = 2")

    def test_if(self):
        simple_case(self, "a = 1\nif x: a = 2", "a = 1\nif x: a = 2")

    def test_if2(self):
        simple_case(self, "if x: a = 2\na = 1", "if x: a = 2\na = 1")

    def test_if_else(self):
        simple_case(
            self, "a = 1\nif x: a = 2\nelse: a = 3", "pass\nif x: a = 2\nelse: a = 3"
        )

    def test_if_else2(self):
        simple_case(
            self, "if x: a = 2\nelse: a = 3\na = 1", "if x: pass\nelse: pass\na = 1"
        )

    def test_for(self):
        simple_case(self, "a = 1\nfor x in y: a = 2", "a = 1\nfor x in y: a = 2")

    def test_for_else(self):
        simple_case(
            self,
            "a = 1\nfor x in y: a = 2\nelse: a = 3",
            "pass\nfor x in y: a = 2\nelse: a = 3",
        )

    def test_for_else_break(self):
        simple_case(
            self,
            "a = 1\nfor x in y:\n    break\n    a = 2\nelse: a = 3",
            "a = 1\nfor x in y:\n    break\n    a = 2\nelse: a = 3",
        )

    def test_for_else_conti(self):
        simple_case(
            self,
            "a = 1\nfor x in y:\n    continue\n    a = 2\nelse: a = 3",
            "a = 1\nfor x in y:\n    continue\n    a = 2\nelse: a = 3",
        )

    def test_while(self):
        simple_case(self, "a = 1\nwhile x: a = 2", "a = 1\nwhile x: a = 2")

    def test_while_else(self):
        simple_case(
            self,
            "a = 1\nwhile x: a = 2\nelse: a = 3",
            "pass\nwhile x: a = 2\nelse: a = 3",
        )


if __name__ == "__main__":
    # import sys;sys.argv = ['', 'Test.testName']
    unittest.main()
