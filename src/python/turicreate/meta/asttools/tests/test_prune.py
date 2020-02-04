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

from ...asttools.tests import AllTypesTested, assert_ast_eq
import unittest
from ...asttools.mutators.prune_mutator import PruneVisitor
import ast
from ...testing import py2only

tested = AllTypesTested()


class TestExclusive(unittest.TestCase):
    def assertPruned(self, source, pruned, symbols):
        mutator = PruneVisitor(symbols=symbols, mode="exclusive")

        orig_ast = ast.parse(source)
        expected_ast = ast.parse(pruned)
        mutator.visit(orig_ast)

        assert_ast_eq(self, orig_ast, expected_ast)

        tested.update(orig_ast)

    def test_assign(self):
        source = "a = b; c = d"
        pruned = "a = b;"

        self.assertPruned(source, pruned, symbols=["c", "d"])

        pruned2 = "c = d"

        self.assertPruned(source, pruned2, symbols=["a", "b"])

        pruned = "a = b; c = d"

        self.assertPruned(source, pruned, symbols=["c"])

        pruned2 = "a = b; c = d"

        self.assertPruned(source, pruned2, symbols=["b"])

    def test_binop(self):
        source = "a + b; c + d"
        pruned = "a + b"

        self.assertPruned(source, pruned, symbols=["c", "d"])

    def test_unaryop(self):
        source = "+b; -c"
        pruned = "+b"

        self.assertPruned(source, pruned, symbols=["c"])

    def test_for(self):
        source = "for i in j: k"

        pruned = "for i in j: pass"
        self.assertPruned(source, pruned, symbols=["k"])

        pruned = ""
        self.assertPruned(source, pruned, symbols=["k", "i", "j"])

    def test_for_else(self):
        source = "for i in j:\n    k\nelse:\n    l"

        pruned = "for i in j:\n    k"
        self.assertPruned(source, pruned, symbols=["l"])

        pruned = "for i in j:\n    pass\nelse:\n    l"
        self.assertPruned(source, pruned, symbols=["i", "j", "k"])

    def test_with_as(self):
        source = "with a as b: c"

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a", "b", "c"])

        pruned = "with a as b: pass"
        self.assertPruned(source, pruned, symbols=["c"])

    def test_with(self):
        source = "with a: c"

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a", "c"])

        pruned = ""
        self.assertPruned(source, pruned, symbols=["c"])

    def test_if(self):
        source = "if a: b\nelse: c"

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a", "b", "c"])

        pruned = ""
        self.assertPruned(source, pruned, symbols=["b", "c"])

        pruned = "if a: b"
        self.assertPruned(source, pruned, symbols=["c"])

    def test_if_expr(self):
        source = "a = b if c else d"

        pruned = "a = b if c else d"
        self.assertPruned(source, pruned, symbols=["b", "c", "d"])

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a", "b", "c", "d"])

    def test_while(self):
        source = "while a: b"

        pruned = ""
        self.assertPruned(source, pruned, symbols=["b",])

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a", "b", "c", "d"])

    def test_import(self):
        source = "import a"

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a",])

        source = "import a, b"

        pruned = "import a, b"
        self.assertPruned(source, pruned, symbols=["a",])

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a", "b"])

    def test_import_from(self):
        source = "from a import b"

        pruned = "from a import b"
        self.assertPruned(source, pruned, symbols=["a",])

        pruned = ""
        self.assertPruned(source, pruned, symbols=["b",])

    def test_try(self):
        source = """
try:
    a
except b as c:
    d
"""

        pruned = """
"""
        self.assertPruned(source, pruned, symbols=["a", "b", "c", "d"])

        pruned = """
try:
    a
except b as c:
    pass
"""

        self.assertPruned(source, pruned, symbols=["d"])

        pruned = """
"""
        self.assertPruned(source, pruned, symbols=["a", "d"])

    def test_try_else(self):
        source = """
try:
    a
except b as c:
    d
else:
    e
"""

        pruned = """
try:
    pass
except:
    pass
else:
    e

"""
        self.assertPruned(source, pruned, symbols=["a",])

    def test_try_finally(self):
        source = """
try:
    a
except b as c:
    d
else:
    e
finally:
    f
"""

        pruned = """
try:
    pass
except:
    pass
else:
    e
finally:
    f

"""
        self.assertPruned(source, pruned, symbols=["a",])

        pruned = """
try:
    pass
except:
    pass
else:
    e
finally:
    pass

"""
        self.assertPruned(source, pruned, symbols=["a", "f"])

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a", "d", "e", "f"])

    @py2only
    def test_exec(self):
        source = "exec a"
        pruned = "exec a"
        self.assertPruned(source, pruned, symbols=["a"])

    def test_attr(self):
        pass


class TestInclusive(unittest.TestCase):
    def assertPruned(self, source, pruned, symbols):
        mutator = PruneVisitor(symbols=symbols, mode="inclusive")

        orig_ast = ast.parse(source)
        expected_ast = ast.parse(pruned)
        mutator.visit(orig_ast)

        assert_ast_eq(self, orig_ast, expected_ast)

        tested.update(orig_ast)

    def test_assign(self):
        source = "a = b; c = d"
        pruned = "a = b;"

        self.assertPruned(source, pruned, symbols=["c", "d"])

        pruned2 = "c = d"

        self.assertPruned(source, pruned2, symbols=["a", "b"])

        pruned = "a = b"

        self.assertPruned(source, pruned, symbols=["c"])

        pruned2 = "c = d"

        self.assertPruned(source, pruned2, symbols=["b"])

    def test_binop(self):
        source = "a + b; c + d"
        pruned = "a + b"

        self.assertPruned(source, pruned, symbols=["c", "d"])

    def test_unaryop(self):
        source = "+b; -c"
        pruned = "+b"

        self.assertPruned(source, pruned, symbols=["c"])

    def test_for(self):
        source = "for i in j: k"

        pruned = "for i in j: pass"
        self.assertPruned(source, pruned, symbols=["k"])

        pruned = ""
        self.assertPruned(source, pruned, symbols=["k", "i", "j"])

    def test_for_else(self):
        source = "for i in j:\n    k\nelse:\n    l"

        pruned = "for i in j:\n    k"
        self.assertPruned(source, pruned, symbols=["l"])

        pruned = "for i in j:\n    pass\nelse:\n    l"
        self.assertPruned(source, pruned, symbols=["i", "j", "k"])

    def test_with_as(self):
        source = "with a as b: c"

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a", "b", "c"])

        pruned = "with a as b: pass"
        self.assertPruned(source, pruned, symbols=["c"])

    def test_with(self):
        source = "with a: c"

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a", "c"])

        pruned = ""
        self.assertPruned(source, pruned, symbols=["c"])

    def test_if(self):
        source = "if a: b\nelse: c"

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a", "b", "c"])

        pruned = ""
        self.assertPruned(source, pruned, symbols=["b", "c"])

        pruned = "if a: b"
        self.assertPruned(source, pruned, symbols=["c"])

    def test_if_expr(self):
        source = "a = b if c else d"

        pruned = ""
        self.assertPruned(source, pruned, symbols=["b", "c", "d"])

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a", "b", "c", "d"])

    def test_while(self):
        source = "while a: b"

        pruned = ""
        self.assertPruned(source, pruned, symbols=["b",])

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a", "b", "c", "d"])

    def test_import(self):
        source = "import a"

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a",])

        source = "import a, b"

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a",])

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a", "b"])

    def test_import_from(self):
        source = "from a import b"

        pruned = "from a import b"
        self.assertPruned(source, pruned, symbols=["a",])

        pruned = ""
        self.assertPruned(source, pruned, symbols=["b",])

    def test_try(self):
        source = """
try:
    a
except b as c:
    d
"""

        pruned = """
"""
        self.assertPruned(source, pruned, symbols=["a", "b", "c", "d"])

        pruned = """
try:
    a
except b as c:
    pass
"""

        self.assertPruned(source, pruned, symbols=["d"])

        pruned = """
"""
        self.assertPruned(source, pruned, symbols=["a", "d"])

    def test_try_else(self):
        source = """
try:
    a
except b as c:
    d
else:
    e
"""

        pruned = """
try:
    pass
except:
    pass
else:
    e

"""
        self.assertPruned(source, pruned, symbols=["a",])

    def test_try_finally(self):
        source = """
try:
    a
except b as c:
    d
else:
    e
finally:
    f
"""

        pruned = """
try:
    pass
except:
    pass
else:
    e
finally:
    f

"""
        self.assertPruned(source, pruned, symbols=["a",])

        pruned = """
try:
    pass
except:
    pass
else:
    e
finally:
    pass

"""
        self.assertPruned(source, pruned, symbols=["a", "f"])

        pruned = ""
        self.assertPruned(source, pruned, symbols=["a", "d", "e", "f"])

    @py2only
    def test_exec(self):
        source = "exec a"
        pruned = "exec a"
        self.assertPruned(source, pruned, symbols=["a"])

    def test_attr(self):
        pass


if __name__ == "__main__":
    # import sys;sys.argv = ['', 'Test.test_assign']
    unittest.main(exit=False)

    print(tested.tested())
