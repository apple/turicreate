# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Created on Aug 2, 2011

@author: sean
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import ast
from ...asttools.visitors.graph_visitor import GraphGen
from ...asttools.visitors.graph_visitor import DiGraph
from ...asttools.tests import AllTypesTested, skip_networkx

tested = AllTypesTested()


def binop_method(op):
    def test_binop(self):
        source = "c = a %s b" % (op,)
        self.assertDepends(source, {("c", "a"), ("c", "b")}, {"a", "b"}, {"c"})

    return test_binop


def unarynop_method(op):
    def test_unaryop(self):
        source = "c = %s b" % (op,)
        self.assertDepends(source, {("c", "b")}, {"b"}, {"c"})

    return test_unaryop


@skip_networkx
class Test(unittest.TestCase):
    def assertDepends(self, source, edges, undefined=None, modified=None):
        mod = ast.parse(source)

        gen = GraphGen(call_deps=True)
        gen.visit(mod)

        self.assertSetEqual(set(gen.graph.edges()), edges)

        if undefined is not None:
            self.assertSetEqual(set(gen.undefined), undefined)
        if modified is not None:
            self.assertSetEqual(set(gen.modified), modified)

        tested.update(mod)
        return gen

    def test_name(self):
        source = "a"
        self.assertDepends(source, set())

    def test_assign(self):
        source = "a = b"
        self.assertDepends(source, {("a", "b")}, {"b"}, {"a"})

    def test_assign_tuple(self):
        source = "(a, c) = b"
        self.assertDepends(source, {("a", "b"), ("c", "b")}, {"b"}, {"a", "c"})

    def test_assign_multi(self):
        source = "a = b  = c"
        self.assertDepends(source, {("a", "c"), ("b", "c")}, {"c"}, {"a", "b"})

    def test_assign_attr(self):
        source = "a.x = b"
        self.assertDepends(source, {("a", "b")}, {"b", "a"}, {"a"})

    def test_attr_assign(self):
        source = "a = b.x"
        self.assertDepends(source, {("a", "b")}, {"b"}, {"a"})

    def test_subscr(self):
        source = "a[:] = b[:]"
        self.assertDepends(source, {("a", "b")}, {"a", "b"}, {"a"})

    def test_subscr_value(self):
        source = "a = b[c]"
        self.assertDepends(source, {("a", "b"), ("a", "c")}, {"b", "c"}, {"a"})

    def test_subscr_lvalue(self):
        source = "a[c] = b"
        self.assertDepends(source, {("a", "b"), ("a", "c")}, {"a", "b", "c"}, {"a"})

    def test_subscr_attr(self):
        source = "a[:] = b[:].b"
        self.assertDepends(source, {("a", "b")}, {"a", "b"}, {"a"})

    def test_import(self):
        source = "import foo; foo.a = b"
        self.assertDepends(source, {("foo", "b")}, {"b"}, {"foo"})

    def test_import_from(self):
        source = "from bar import foo; foo.a = b"
        self.assertDepends(source, {("foo", "b")}, {"b"}, {"foo"})

    def test_import_as(self):
        source = "import bar as foo; foo.a = b"
        self.assertDepends(source, {("foo", "b")}, {"b"}, {"foo"})

    def test_import_from_as(self):
        source = "from bar import baz as foo; foo.a = b"
        self.assertDepends(source, {("foo", "b")}, {"b"}, {"foo"})

    def test_augment_assign(self):
        source = "a += b"
        self.assertDepends(source, {("a", "b"), ("a", "a")}, {"b"}, {"a"})

    test_add = binop_method("+")
    test_sub = binop_method("-")
    test_pow = binop_method("**")

    test_eq = binop_method("==")
    test_ne = binop_method("!=")

    test_rshift = binop_method(">>")
    test_lshift = binop_method("<<")

    test_mult = binop_method("*")
    test_mod = binop_method("%")
    test_div = binop_method("/")
    test_floordiv = binop_method("//")
    test_bitxor = binop_method("^")

    test_lt = binop_method("<")
    test_gt = binop_method(">")

    test_lte = binop_method("<=")
    test_gte = binop_method(">=")

    test_in = binop_method("in")
    test_not_in = binop_method("not in")
    test_is = binop_method("is")
    test_is_not = binop_method("is not")

    test_bit_or = binop_method("|")
    test_bit_and = binop_method("&")

    test_or = binop_method("or")
    test_and = binop_method("and")

    test_not = unarynop_method("not")
    test_uadd = unarynop_method("+")
    test_usub = unarynop_method("-")
    test_invert = unarynop_method("~")

    def test_call(self):
        source = "foo(a)"
        self.assertDepends(
            source, {("foo", "a"), ("a", "foo")}, {"a", "foo"},
        )

    def test_for(self):
        source = "for i in a:\n    b"
        self.assertDepends(source, {("i", "a"), ("b", "a")}, {"a", "b"}, {"i"})

    def test_for2(self):
        source = "for i in a:\n    x += b[i]"
        self.assertDepends(
            source,
            {("i", "a"), ("b", "a"), ("x", "a"), ("x", "i"), ("x", "b"), ("x", "x")},
            {"a", "b"},
            {"x", "i"},
        )

    def test_for_unpack(self):
        source = "for i, j in a:\n    x += b[i]"
        self.assertDepends(
            source,
            {
                ("i", "a"),
                ("j", "a"),
                ("b", "a"),
                ("x", "a"),
                ("x", "i"),
                ("x", "b"),
                ("x", "x"),
            },
            {"a", "b"},
            {"x", "i", "j"},
        )

    def test_dict(self):
        source = "c = {a:b}"
        self.assertDepends(source, {("c", "a"), ("c", "b")}, {"a", "b"}, {"c"})

    def test_list(self):
        source = "c = [a,b]"
        self.assertDepends(source, {("c", "a"), ("c", "b")}, {"a", "b"}, {"c"})

    def test_tuple(self):
        source = "c = (a,b)"
        self.assertDepends(source, {("c", "a"), ("c", "b")}, {"a", "b"}, {"c"})

    def test_set(self):
        source = "c = {a,b}"
        self.assertDepends(source, {("c", "a"), ("c", "b")}, {"a", "b"}, {"c"})

    def test_if(self):
        source = "if a: b"
        self.assertDepends(source, {("b", "a")}, {"a", "b"}, set())

    def test_if_else(self):
        source = "if a: b\nelse: c"
        self.assertDepends(source, {("b", "a"), ("c", "a")}, {"a", "b", "c"}, set())

    def test_if_elif_else(self):
        source = "if a: b\nelif x: c\nelse: d"
        self.assertDepends(
            source,
            {("b", "a"), ("c", "x"), ("c", "a"), ("d", "a"), ("d", "x"), ("x", "a")},
            {"a", "b", "c", "d", "x"},
            set(),
        )

    def test_if_expr(self):
        source = "d = b if a else c"
        self.assertDepends(
            source, {("d", "a"), ("d", "b"), ("d", "c")}, {"a", "b", "c"}, {"d"}
        )

    def test_assert(self):
        source = "assert a"
        self.assertDepends(source, set(), {"a"}, set())

    def test_with(self):
        source = "with a as b: c"
        self.assertDepends(source, {("b", "a"), ("c", "a")}, {"a", "c"}, {"b"})

    def test_while(self):
        source = "while a: c"
        self.assertDepends(source, {("c", "a")}, {"a", "c"})

    def test_function_def(self):
        source = """a = 1
def foo(b):
    return a + b
"""
        self.assertDepends(source, {("foo", "a")})

    def test_lambda(self):
        source = """a = 1
foo = lambda b:  a + b
"""
        self.assertDepends(source, {("foo", "a")})

    def test_list_comp(self):
        source = "a = [b for b in c]"
        self.assertDepends(source, {("a", "c")})

    def test_dict_comp(self):
        source = "a = {b:d for b,d in c}"
        self.assertDepends(source, {("a", "c")})

    def test_set_comp(self):
        source = "a = {b for b in c}"
        self.assertDepends(source, {("a", "c")})

    def test_try_except(self):
        source = """
try:
    a
except b:
    c
        """
        self.assertDepends(source, {("c", "a"), ("c", "b")})

    def test_try_except_else(self):
        source = """
try:
    a
except b:
    c
else:
    d
        """
        self.assertDepends(source, {("c", "a"), ("c", "b"), ("d", "a")})

    def test_try_finally(self):
        source = """
try:
    a
except b:
    c
finally:
    d
        """
        self.assertDepends(
            source, {("c", "a"), ("c", "b"), ("d", "a"), ("d", "b"), ("d", "c")}
        )


if __name__ == "__main__":
    # import sys;sys.argv = ['', 'Test.test_assign']
    unittest.main(exit=False)

    print(tested.tested())
