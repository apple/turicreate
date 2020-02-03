# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Created on Jul 14, 2011

@author: sean
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
from ...testing import py2, py2only
from ...decompiler.tests import Base


filename = "tests.py"


class LogicJumps(Base):
    def test_logic1(self):
        "a and b or c"
        self.statement("a and b or c")

    def test_logic2(self):
        "a or (b or c)"
        self.statement("a or (b or c)")

    def test_if_expr_discard(self):

        stmnt = "a if b else c"
        self.statement(stmnt)

    @unittest.skip("I think this may be a bug in python")
    def test_if_expr_const_bug(self):

        stmnt = "0 if 1 else 2"
        self.statement(stmnt)

    def test_if_expr_assign(self):

        stmnt = "d = a if b else c"
        self.statement(stmnt)

    def test_if_expr_assignattr(self):

        stmnt = "d.a = a if b else c"
        self.statement(stmnt)

    def test_bug010(self):

        stmnt = """
def foo():
    if a:
        return 1
    else:
        return 2
        """

        equiv = """
def foo():
    if a:
        return 1
    return 2
    return None
        """

        self.statement(stmnt, equiv=equiv)

    @unittest.expectedFailure
    def test_bug011(self):

        stmnt = """
def foo():
    if a or b or c:
        return 1
    else:
        return 2
        """

        self.statement(stmnt)


class Function(Base):
    def test_function(self):
        stmnt = """
def foo():
    return None
"""
        self.statement(stmnt)

    def test_function_args(self):
        stmnt = """
def foo(a, b, c='asdf'):
    return None
"""
        self.statement(stmnt)

    def test_function_var_args(self):
        stmnt = """
def foo(a, b, *c):
    return None
"""
        self.statement(stmnt)

    def test_function_varkw_args(self):
        stmnt = """
def foo(a, b, *c, **d):
    return None
"""
        self.statement(stmnt)

    def test_function_kw_args(self):
        stmnt = """
def foo(a, b, **d):
    return None
"""
        self.statement(stmnt)

    def test_function_yield(self):
        stmnt = """
def foo(a, b):
    yield a + b
    return
"""

        self.statement(stmnt)

    def test_function_decorator(self):
        stmnt = """
@bar
def foo(a, b):
    return None
"""

        self.statement(stmnt)

    def test_function_decorator2(self):
        stmnt = """
@bar
@bar2
def foo(a, b):
    return None
"""

        self.statement(stmnt)

    def test_build_lambda(self):
        stmnt = "lambda a: a"
        self.statement(stmnt)

    def test_build_lambda1(self):
        stmnt = "func = lambda a, b: a+1"
        self.statement(stmnt)

    def test_build_lambda_var_args(self):
        stmnt = "func = lambda a, *b: a+1"
        self.statement(stmnt)

    def test_build_lambda_kw_args(self):
        stmnt = "func = lambda **b: a+1"
        self.statement(stmnt)

    def test_build_lambda_varkw_args(self):
        stmnt = "func = lambda *a, **b: a+1"
        self.statement(stmnt)


class ClassDef(Base):
    def test_build_class(self):
        stmnt = """
class Bar(object):
    'adsf'
    a = 1
"""
        self.statement(stmnt)

    def test_build_class_wfunc(self):
        stmnt = """
class Bar(object):
    'adsf'
    a = 1
    def foo(self):
        return None

"""
        self.statement(stmnt)

    def test_build_class_wdec(self):
        stmnt = """
@decorator
class Bar(object):
    'adsf'
    a = 1
    def foo(self):
        return None

"""
        self.statement(stmnt)


class ControlFlow(Base):
    def test_if(self):
        self.statement("if a: b")

    def test_if2(self):
        self.statement("if a: b or c")

    def test_if3(self):
        self.statement("if a and b: c")

    def test_if4(self):
        self.statement("if a or b: c")

    def test_if5(self):
        self.statement("if not a: c")

    def test_if6(self):
        self.statement("if not a or b: c")

    def test_elif(self):

        stmnt = """if a:
    b
elif c:
    d"""
        self.statement(stmnt)

    def test_if_else(self):

        stmnt = """if a:
    b
else:
    d"""
        self.statement(stmnt)

    def test_if_elif_else(self):

        stmnt = """if a:
    b
elif f:
    d
else:
    d"""
        self.statement(stmnt)

    def test_tryexcept1(self):
        stmnt = """
try:
    foo
except:
    bar
"""
        self.statement(stmnt)

    def test_tryexcept_else(self):
        stmnt = """
try:
    foo
except:
    bar
else:
    baz
"""
        self.statement(stmnt)

    def test_tryexcept2(self):
        stmnt = """
try:
    foo
except Exception:
    bar
else:
    baz
"""
        self.statement(stmnt)

    def test_tryexcept3(self):
        stmnt = """
try:
    foo
except Exception as error:
    bar
else:
    baz
"""
        self.statement(stmnt)

    def test_tryexcept4(self):
        stmnt = """
try:
    foo
except Exception as error:
    bar
except Baz as error:
    bar
else:
    baz
"""
        self.statement(stmnt)

    def test_while(self):
        self.statement("while b: a")

    def test_while1(self):
        self.statement("while 1: a")

    def test_while_logic(self):
        self.statement("while a or b: x")

    def test_while_logic2(self):
        self.statement("while a and b: x")

    def test_while_logic3(self):
        self.statement("while a >= r and b == c: x")

    def test_while_else(self):
        stmnt = """
while a:
    break
else:
    a
"""
        self.statement(stmnt)

    def test_for(self):
        stmnt = """
for i in  a:
    break
"""
        self.statement(stmnt)

    def test_for2(self):
        stmnt = """
for i in  a:
    b = 3
"""
        self.statement(stmnt)

    def test_for_else(self):
        stmnt = """
for i in  a:
    b = 3
else:
    b= 2
"""
        self.statement(stmnt)

    def test_for_continue(self):
        stmnt = """
for i in  a:
    b = 3
    continue
"""
        self.statement(stmnt)

    def test_for_unpack(self):
        stmnt = """
for i,j in  a:
    b = 3
"""
        self.statement(stmnt)

    def test_try_continue(self):
        stmnt = """
for x in (1,2):
        try: continue
        except: pass
"""
        self.statement(stmnt)

    def test_loop_01(self):
        stmnt = """
if c > d:
    if e > f:
        g
    h
"""

    def test_loop_bug(self):
        stmnt = """
for a in b:
    if c > d:
        if e > f:
            g
        h
"""
        self.statement(stmnt)

    def test_while_bug(self):
        stmnt = """
while a:
    q
    while b:
        w
"""
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_while_bug02(self):
        stmnt = """
while 1:
    b += y
    if b < x:
        break
"""
        self.statement(stmnt)


class Complex(Base):
    def test_if_in_for(self):
        stmnt = """
for i in j:
    if i:
        j =1
"""
        self.statement(stmnt)

    def test_if_in_for2(self):
        stmnt = """
for i in j:
    if i:
        a
    else:
        b

"""
        self.statement(stmnt)

    def test_if_in_for3(self):
        stmnt = """
for i in j:
    if i:
        break
    else:
        continue

"""
        equiv = """
for i in j:
    if i:
        break
        continue

"""
        self.statement(stmnt, equiv)

    def test_if_in_while(self):
        stmnt = """
while i in j:
    if i:
        a
    else:
        b

"""
        self.statement(stmnt)

    def test_nested_if(self):
        stmnt = """
if a:
    if b:
        c
    else:
        d
"""
        self.statement(stmnt)

    def test_nested_if2(self):
        stmnt = """
if a:
    if b:
        c
    else:
        d
else:
    b
"""
        self.statement(stmnt)

    def test_if_return(self):
        stmnt = """
def a():
    if b:
        return None
    return None
"""
        self.statement(stmnt)

    def test_if_return2(self):
        stmnt = """
def a():
    if b:
        a
    else:
        return b

    return c
"""
        self.statement(stmnt)

    def test_nested_while_bug(self):
        stmnt = """
if gid == 0:
    output[0] = initial
    while i < input.size:
        output[0] += shared[i]
"""
        self.statement(stmnt)

    def test_aug_assign_slice(self):
        stmnt = "c[idx:a:3] += b[idx:a]"
        self.statement(stmnt)

    def test_issue_4(self):
        example = """
def example(idx):
   if(idx == 2 or idx == 3):
      idx = 1
      return None
   i += 1
   return None
        """
        self.statement(example)


if __name__ == "__main__":
    # import sys;sys.argv = ['', 'Test.test_assign']
    unittest.main()
