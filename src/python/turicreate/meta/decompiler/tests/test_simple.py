# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Created on Nov 9, 2011

@author: sean
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ...testing import py2only
from ...decompiler.tests import Base
import unittest


class Simple(Base):
    def test_assign(self):
        "a = b"
        self.statement("a = b")

    def test_assign2(self):
        "a = b = c"
        self.statement("a = b")

    def test_assign3(self):
        "a = b,d = c"
        self.statement("a = b")

    def test_assign4(self):
        "a.y = b,d = c"
        self.statement("a = b")

    def test_setattr(self):
        "a.b = b"
        self.statement("a.b = b")

    def test_getattr(self):
        "a = b.b"
        self.statement("a = b.b")

    def test_add(self):
        "a+b"
        self.statement("a+b")

    def test_sub(self):
        "a-b"
        self.statement("a-b")

    def test_mul(self):
        "a*b"
        self.statement("a*b")

    def test_div(self):
        "a/b"
        self.statement("a/b")

    def test_floordiv(self):
        "a//b"
        self.statement("a//b")

    def test_pow(self):
        "a**b"
        self.statement("a**b")

    def test_eq(self):
        "a==b"
        self.statement("a==b")

    def test_iadd(self):
        "a+=b"
        self.statement("a+=b")

    def test_isub(self):
        "a-=b"
        self.statement("a-=b")

    def test_binary_and(self):
        "a & b"
        self.statement("a & b")

    def test_binary_lshift(self):
        "a << b"
        self.statement("a << b")

    def test_binary_rshift(self):
        "a >> b"
        self.statement("a >> b")

    def test_binary_mod(self):
        "a % b"
        self.statement("a % b")

    def test_binary_or(self):
        "a | b"
        self.statement("a | b")

    def test_binary_xor(self):
        "a ^ b"
        self.statement("a ^ b")

    def test_build_list(self):
        "[x,y, 1, None]"
        self.statement("[x,y, 1, None]")

    def test_build_tuple(self):
        "(x,y, 1, None)"
        self.statement("(x,y, 1, None)")

    def test_build_set(self):
        "{x,y, 1, None}"
        self.statement("{x,y, 1, None}")

    def test_build_dict(self):
        "{a:x,b:y, c:1, d:None}"
        self.statement("{a:x,b:y, c:1, d:None}")

    def test_unpack_tuple(self):
        "(a,b) = c"
        self.statement("(a,b) = c")

    def test_delete_name(self):
        stmnt = "del a"
        self.statement(stmnt)

    def test_delete_attr(self):
        stmnt = "del a.a"
        self.statement(stmnt)

    @py2only
    def test_exec1(self):
        stmnt = "exec a"
        self.statement(stmnt)

    @py2only
    def test_exec2(self):
        stmnt = "exec a in b"
        self.statement(stmnt)

    @py2only
    def test_exec3(self):
        stmnt = "exec a in b,c"
        self.statement(stmnt)

    @py2only
    def test_exec4(self):
        stmnt = "exec a in {2:1}, { }"
        self.statement(stmnt)

    def test_import_star(self):

        stmnt = "from a import *"
        self.statement(stmnt)

        stmnt = "from a.v import *"
        self.statement(stmnt)

    def test_import(self):
        stmnt = "import a"
        self.statement(stmnt)

    def test_import_as(self):
        stmnt = "import a as b"
        self.statement(stmnt)

    def test_import_from(self):
        stmnt = "from c import a as b"
        self.statement(stmnt)

    def test_import_from2(self):
        stmnt = "from c import a \nimport x"
        self.statement(stmnt)

    def test_not(self):
        stmnt = "not a"
        self.statement(stmnt)

    def test_call(self):
        stmnt = "a()"
        self.statement(stmnt)

    def test_call_args(self):
        stmnt = "a(a, b)"
        self.statement(stmnt)

    def test_call_args1(self):
        stmnt = "a(a, b, c=33)"
        self.statement(stmnt)

    def test_call_varargs(self):
        stmnt = "a(*a)"
        self.statement(stmnt)

    def test_call_kwargs(self):
        stmnt = "a(a, b=0, **a)"
        self.statement(stmnt)

    def test_call_var_kwargs(self):
        stmnt = "a(a, b=0, *d, **a)"
        self.statement(stmnt)

    @py2only
    def test_print(self):
        stmnt = "print foo,"
        self.statement(stmnt)

    @py2only
    def test_printnl(self):
        stmnt = "print foo"
        self.statement(stmnt)

    @py2only
    def test_printitems(self):
        stmnt = "print foo, bar, bas,"
        self.statement(stmnt)

    @py2only
    def test_printitemsnl(self):
        stmnt = "print foo, bar, bas"
        self.statement(stmnt)

    @py2only
    def test_print_to(self):
        stmnt = "print >> stream, foo,"
        self.statement(stmnt)

    @py2only
    def test_print_to_nl(self):
        stmnt = "print >> stream, foo"
        self.statement(stmnt)

    @py2only
    def test_printitems_to(self):
        stmnt = "print >> stream, foo, bar, bas,"
        self.statement(stmnt)

    @py2only
    def test_printitems_to_nl(self):
        stmnt = "print >> stream, foo, bar, bas"
        self.statement(stmnt)

    def test_subscr(self):
        stmnt = "x[y]"
        self.statement(stmnt)

    def test_subscr_assign(self):
        stmnt = "x[y] =z"
        self.statement(stmnt)

    def test_subscr_del(self):
        stmnt = "del x[y]"
        self.statement(stmnt)

    def test_subscr0(self):
        stmnt = "x[:]"
        self.statement(stmnt)

    def test_subscr_assign0(self):
        stmnt = "x[:] =z"
        self.statement(stmnt)

    def test_subscr_del0(self):
        stmnt = "del x[:]"
        self.statement(stmnt)

    def test_subscr1(self):
        stmnt = "x[a:]"
        self.statement(stmnt)

    def test_subscr_assign1(self):
        stmnt = "x[a:] =z"
        self.statement(stmnt)

    def test_subscr_del1(self):
        stmnt = "del x[a:]"
        self.statement(stmnt)

    def test_subscr2(self):
        stmnt = "x[:a]"
        self.statement(stmnt)

    def test_subscr_assign2(self):
        stmnt = "x[:a] =z"
        self.statement(stmnt)

    def test_subscr_del2(self):
        stmnt = "del x[:a]"
        self.statement(stmnt)

    def test_subscr3(self):
        stmnt = "x[b:a]"
        self.statement(stmnt)

    def test_subscr_assign3(self):
        stmnt = "x[b:a] =z"
        self.statement(stmnt)

    def test_subscr_del3(self):
        stmnt = "del x[b:a]"
        self.statement(stmnt)

    def test_subscrX(self):
        stmnt = "x[b:a:c]"
        self.statement(stmnt)

    def test_subscr_assignX(self):
        stmnt = "x[b:a:c] =z"
        self.statement(stmnt)

    def test_subscr_delX(self):
        stmnt = "del x[b:a:c]"
        self.statement(stmnt)

    def test_subscrX2(self):
        stmnt = "x[::]"
        self.statement(stmnt)

    def test_subscr_assignX2(self):
        stmnt = "x[::] =z"
        self.statement(stmnt)

    def test_subscr_delX2(self):
        stmnt = "del x[::]"
        self.statement(stmnt)

    def test_subscr_tuple(self):
        stmnt = "x[x,a]"
        self.statement(stmnt)

    def test_subscr_tuple_set(self):
        stmnt = "x[x,a] =z"
        self.statement(stmnt)

    def test_subscr_tuple_del(self):
        stmnt = "del x[x,a]"
        self.statement(stmnt)

    def test_subscrX3(self):
        stmnt = "x[x,:a]"
        self.statement(stmnt)

    def test_subscr_assignX3(self):
        stmnt = "x[x,:a] =z"
        self.statement(stmnt)

    def test_subscr_delX3(self):
        stmnt = "del x[x,:a]"
        self.statement(stmnt)

    def test_bug_001(self):
        stmnt = "a = 1; b = 2; (a, b) = (b, a)"
        self.statement(stmnt)

    def test_bug_0021(self):
        stmnt = "(a, b, c) = (c, b, a)"
        self.statement(stmnt)

    def test_bug_002(self):

        stmnt = "x = range(6)\nx[2:4] += 'abc'"
        self.statement(stmnt)

    def test_bug_003(self):

        stmnt = "raise V"
        self.statement(stmnt)

    def test_bug_004(self):
        stmnt = "(a, b, c) = (c, b, a) = (x, y, z)"
        self.statement(stmnt)


if __name__ == "__main__":
    # import sys;sys.argv = ['', 'Test.test_assign']
    unittest.main()
