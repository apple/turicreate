# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Created on Aug 9, 2011

@author: sean
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
from ...asttools.visitors.cond_symbol_visitor import conditional_symbols
import ast
from ...testing import py2only


class Test(unittest.TestCase):
    def assertCorrect(
        self,
        source,
        lhs_conditional=None,
        lhs_stable=None,
        rhs_conditional=None,
        rhs_stable=None,
        undefined=None,
    ):

        mod = ast.parse(source)

        lhs, rhs, un = conditional_symbols(mod)

        if lhs_conditional is not None:
            self.assertEqual(lhs[0], set(lhs_conditional))

        if lhs_stable is not None:
            self.assertEqual(lhs[1], set(lhs_stable))

        if rhs_conditional is not None:
            self.assertEqual(rhs[0], set(rhs_conditional))

        if rhs_stable is not None:
            self.assertEqual(rhs[1], set(rhs_stable))

        if undefined is not None:
            self.assertEqual(un, set(undefined))

    def test_assign(self):
        self.assertCorrect(
            source="a = 1",
            lhs_conditional=[],
            lhs_stable=["a"],
            rhs_conditional=[],
            rhs_stable=[],
            undefined=[],
        )

    def test_assign2(self):
        self.assertCorrect(
            source="a = b",
            lhs_conditional=[],
            lhs_stable=["a"],
            rhs_conditional=[],
            rhs_stable=["b"],
            undefined=["b"],
        )

    def test_assign3(self):
        self.assertCorrect(
            source="a, b = b",
            lhs_conditional=[],
            lhs_stable=["a", "b"],
            rhs_conditional=[],
            rhs_stable=["b"],
            undefined=["b"],
        )

    def test_assign4(self):
        self.assertCorrect(
            source="b = 1; a = b",
            lhs_conditional=[],
            lhs_stable=["a", "b"],
            rhs_conditional=[],
            rhs_stable=["b"],
            undefined=[],
        )

    def test_assign5(self):
        self.assertCorrect(
            source="a = b; b = 1",
            lhs_conditional=[],
            lhs_stable=["a", "b"],
            rhs_conditional=[],
            rhs_stable=["b"],
            undefined=["b"],
        )

    def test_assign6(self):
        self.assertCorrect(
            source="a = a",
            lhs_conditional=[],
            lhs_stable=["a",],
            rhs_conditional=[],
            rhs_stable=["a"],
            undefined=["a"],
        )

    def test_aug_assign(self):
        self.assertCorrect(
            source="a += 1",
            lhs_conditional=[],
            lhs_stable=["a"],
            rhs_conditional=[],
            rhs_stable=["a"],
            undefined=["a"],
        )

    def test_assign_attr(self):
        self.assertCorrect(
            source="a.a = 1",
            lhs_conditional=[],
            lhs_stable=[],
            rhs_conditional=[],
            rhs_stable=["a"],
            undefined=["a"],
        )

    def test_assign_subscr(self):
        self.assertCorrect(
            source="a[b] = 1",
            lhs_conditional=[],
            lhs_stable=[],
            rhs_conditional=[],
            rhs_stable=["a", "b"],
            undefined=["a", "b"],
        )

    def test_if0(self):
        self.assertCorrect(
            source="if a: b",
            lhs_conditional=[],
            lhs_stable=[],
            rhs_conditional=["b"],
            rhs_stable=["a"],
            undefined=["a", "b"],
        )

    def test_if1(self):
        self.assertCorrect(
            source="if a: b = 1",
            lhs_conditional=["b"],
            lhs_stable=[],
            rhs_conditional=[],
            rhs_stable=["a"],
            undefined=["a"],
        )

    def test_if_else0(self):
        self.assertCorrect(
            source="if a: b\nelse: b",
            lhs_conditional=[],
            lhs_stable=[],
            rhs_conditional=[],
            rhs_stable=["a", "b"],
            undefined=["a", "b"],
        )

    def test_if_else1(self):
        self.assertCorrect(
            source="if a: b = 1\nelse: b = 1",
            lhs_conditional=[],
            lhs_stable=["b"],
            rhs_conditional=[],
            rhs_stable=["a"],
            undefined=["a"],
        )

    def test_if_elif0(self):
        self.assertCorrect(
            source="if a: b\nelif c: b",
            lhs_conditional=[],
            lhs_stable=[],
            rhs_conditional=["b", "c"],
            rhs_stable=["a"],
            undefined=["a", "b", "c"],
        )

    def test_if_elif1(self):
        self.assertCorrect(
            source="if a: b\nelif c: b\nelse: b",
            lhs_conditional=[],
            lhs_stable=[],
            rhs_conditional=["c"],
            rhs_stable=["a", "b"],
            undefined=["a", "b", "c"],
        )

    def test_if_elif2(self):
        self.assertCorrect(
            source="if a: b\nelif c: pass\nelse: b",
            lhs_conditional=[],
            lhs_stable=[],
            rhs_conditional=["b", "c"],
            rhs_stable=["a"],
            undefined=["a", "b", "c"],
        )

    def test_for(self):
        self.assertCorrect(
            source="for i in j: k = 1",
            lhs_conditional=["i", "k"],
            lhs_stable=[],
            rhs_conditional=[],
            rhs_stable=["j"],
            undefined=["j"],
        )

    def test_for_else0(self):
        self.assertCorrect(
            source="for i in j: k = 1\nelse: k = 2",
            lhs_conditional=["i"],
            lhs_stable=["k"],
            rhs_conditional=[],
            rhs_stable=["j"],
            undefined=["j"],
        )

    def test_for_else1(self):
        self.assertCorrect(
            source="for i in j: k = 1\nb = k",
            lhs_conditional=["i", "k"],
            lhs_stable=["b"],
            rhs_conditional=[],
            rhs_stable=["j", "k"],
            undefined=["j", "k"],
        )

    def test_for_break0(self):
        self.assertCorrect(
            source="for i in j:\n  break\n  k = 1",
            lhs_conditional=["i", "k"],
            lhs_stable=[],
            rhs_conditional=[],
            rhs_stable=["j"],
            undefined=["j"],
        )

    def test_for_break1(self):
        self.assertCorrect(
            source="for i in j:\n  break\n  k = 1\nelse: k = 2",
            lhs_conditional=["i", "k"],
            lhs_stable=[],
            rhs_conditional=[],
            rhs_stable=["j"],
            undefined=["j"],
        )

    def test_while(self):
        self.assertCorrect(
            source="while b: a = 1",
            lhs_conditional=["a"],
            lhs_stable=[],
            rhs_conditional=[],
            rhs_stable=["b"],
            undefined=["b"],
        )

    def test_while_else(self):
        self.assertCorrect(
            source="while b: a = 1\nelse: a = 2",
            lhs_conditional=[],
            lhs_stable=["a"],
            rhs_conditional=[],
            rhs_stable=["b"],
            undefined=["b"],
        )

    def test_while_else_break(self):
        self.assertCorrect(
            source="while b:\n  break\n  a = 1\nelse: a = 2",
            lhs_conditional=["a"],
            lhs_stable=[],
            rhs_conditional=[],
            rhs_stable=["b"],
            undefined=["b"],
        )

    def test_while_break(self):
        self.assertCorrect(
            source="while b:\n  break\n  a = 1",
            lhs_conditional=["a"],
            lhs_stable=[],
            rhs_conditional=[],
            rhs_stable=["b"],
            undefined=["b"],
        )

    def test_nested_if(self):
        self.assertCorrect(
            source="if a:\n  if b: c",
            lhs_conditional=[],
            lhs_stable=[],
            rhs_conditional=["b", "c"],
            rhs_stable=["a"],
            undefined=["a", "b", "c"],
        )

    def test_nested_if1(self):
        self.assertCorrect(
            source="if a:\n  if b: c = 1",
            lhs_conditional=["c"],
            lhs_stable=[],
            rhs_conditional=["b"],
            rhs_stable=["a"],
            undefined=["a", "b"],
        )

    def test_nested_for(self):
        self.assertCorrect(
            source="for a in b:\n  for c in a: d",
            lhs_conditional=["a", "c"],
            lhs_stable=[],
            rhs_conditional=["a", "d"],
            rhs_stable=["b"],
            undefined=["b", "d"],
        )

    def test_nested_while(self):
        self.assertCorrect(
            source="while a:\n  while c: d",
            lhs_conditional=[],
            lhs_stable=[],
            rhs_conditional=["c", "d"],
            rhs_stable=["a"],
            undefined=["a", "c", "d"],
        )

    def test_conditional_after_stable(self):
        self.assertCorrect(
            source="a = 1\nif b: a = 2",
            lhs_conditional=[],
            lhs_stable=["a"],
            rhs_conditional=[],
            rhs_stable=["b"],
            undefined=["b"],
        )

    @py2only
    def test_exec(self):
        self.assertCorrect(
            source="exec a in b, c",
            lhs_conditional=[],
            lhs_stable=[],
            rhs_conditional=[],
            rhs_stable=["a", "b", "c"],
            undefined=["a", "b", "c"],
        )

    def test_assert(self):
        self.assertCorrect(
            source="assert b, msg",
            lhs_conditional=[],
            lhs_stable=[],
            rhs_conditional=[],
            rhs_stable=["b", "msg"],
            undefined=["b", "msg"],
        )

    def test_raise(self):
        self.assertCorrect(
            source="raise b",
            lhs_conditional=[],
            lhs_stable=[],
            rhs_conditional=[],
            rhs_stable=["b"],
            undefined=["b"],
        )

    def test_try(self):
        self.assertCorrect(
            source="try: a \nexcept: b",
            lhs_conditional=[],
            lhs_stable=[],
            rhs_conditional=["a", "b"],
            rhs_stable=[],
            undefined=["a", "b"],
        )

    def test_try2(self):
        self.assertCorrect(
            source="try: a = 1  \nexcept c as d: a = 2",
            lhs_conditional=["d"],
            lhs_stable=["a"],
            rhs_conditional=["c"],
            rhs_stable=[],
            undefined=["c"],
        )

    def test_try_else(self):
        self.assertCorrect(
            source="try: a = 1  \nexcept c as d: a = 2\nelse: x = 1",
            lhs_conditional=["d", "x"],
            lhs_stable=["a"],
            rhs_conditional=["c"],
            rhs_stable=[],
            undefined=["c"],
        )

    def test_try_finally(self):
        self.assertCorrect(
            source="try: a = 1  \nexcept c as d: a = 2\nfinally: x = 1",
            lhs_conditional=["d"],
            lhs_stable=["a", "x"],
            rhs_conditional=["c"],
            rhs_stable=[],
            undefined=["c"],
        )

    def test_bug001(self):
        self.assertCorrect(
            source="if a: d\nd",
            lhs_conditional=[],
            lhs_stable=[],
            rhs_conditional=[],
            rhs_stable=["a", "d"],
            undefined=["a", "d"],
        )

    def test_bug002(self):
        self.assertCorrect(
            source="if a: d = 1\nd = 1",
            lhs_conditional=[],
            lhs_stable=["d"],
            rhs_conditional=[],
            rhs_stable=["a"],
            undefined=["a"],
        )


if __name__ == "__main__":
    # import sys;sys.argv = ['', 'Test.test_assign']
    unittest.main()
