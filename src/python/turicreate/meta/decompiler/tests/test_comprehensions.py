# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Created on Nov 6, 2011

@author: sean
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
from ...decompiler.tests import Base


class ListComprehension(Base):
    def test_comp1(self):
        stmnt = "[a for b in c]"
        self.statement(stmnt)

    def test_comp2(self):
        stmnt = "[a() +1 for b in c]"
        self.statement(stmnt)

    def test_comp3(self):
        stmnt = "y = [a() +1 for b in c]"
        self.statement(stmnt)

    def test_comp_ifs(self):
        stmnt = "y = [a() +1 for b in c if asdf]"
        self.statement(stmnt)

    def test_comp_ifs1(self):
        stmnt = "y = [a() +1 for b in c if asdf if asd]"
        self.statement(stmnt)

    def test_comp_ifs2(self):
        stmnt = "y = [a() +1 for b in c if asdf if not asd]"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp1(self):
        stmnt = "[a for b in c for d in e]"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp2(self):
        stmnt = "[a() +1 for b in c for d in e]"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp3(self):
        stmnt = "y = [a() +1 for b in c for d in e]"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp_ifs(self):
        stmnt = "y = [a() +1 for b in c if asdf for d in e]"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp_ifs1(self):
        stmnt = "y = [a() +1 for b in c if asdf if asd for d in e if this]"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp_ifs2(self):
        stmnt = "y = [a() +1 for b in c for d in e if adsf]"
        self.statement(stmnt)


class SetComprehension(Base):
    def test_comp1(self):
        stmnt = "{a for b in c}"
        self.statement(stmnt)

    def test_comp2(self):
        stmnt = "{a() +1 for b in c}"
        self.statement(stmnt)

    def test_comp3(self):
        stmnt = "y = {a() +1 for b in c}"
        self.statement(stmnt)

    def test_comp_ifs(self):
        stmnt = "y = {a() +1 for b in c if asdf}"
        self.statement(stmnt)

    def test_comp_ifs1(self):
        stmnt = "y = {a() +1 for b in c if asdf if asd}"
        self.statement(stmnt)

    def test_comp_ifs2(self):
        stmnt = "y = {a() +1 for b in c if asdf if not asd}"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp1(self):
        stmnt = "{a for b in c for d in e}"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp2(self):
        stmnt = "{a() +1 for b in c for d in e}"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp3(self):
        stmnt = "y = {a() +1 for b in c for d in e}"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp_ifs(self):
        stmnt = "y = {a() +1 for b in c if asdf for d in e}"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp_ifs1(self):
        stmnt = "y = {a() +1 for b in c if asdf if asd for d in e if this}"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp_ifs2(self):
        stmnt = "y = {a() +1 for b in c for d in e if adsf}"
        self.statement(stmnt)


class DictComprehension(Base):
    def test_comp1(self):
        stmnt = "{a:q for b in c}"
        self.statement(stmnt)

    def test_comp2(self):
        stmnt = "{a() +1:q for b in c}"
        self.statement(stmnt)

    def test_comp3(self):
        stmnt = "y = {a() +1:q for b in c}"
        self.statement(stmnt)

    def test_comp_ifs(self):
        stmnt = "y = {a() +1:q for b in c if asdf}"
        self.statement(stmnt)

    def test_comp_ifs1(self):
        stmnt = "y = {a() +1:q for b in c if asdf if asd}"
        self.statement(stmnt)

    def test_comp_ifs2(self):
        stmnt = "y = {a() +1:q for b in c if asdf if not asd}"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp1(self):
        stmnt = "{a:q for b in c for d in e}"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp2(self):
        stmnt = "{a():q +1 for b in c for d in e}"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp3(self):
        stmnt = "y = {a() +1:q for b in c for d in e}"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp_ifs(self):
        stmnt = "y = {a() +1:q for b in c if asdf for d in e}"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp_ifs1(self):
        stmnt = "y = {a() +1:q for b in c if asdf if asd for d in e if this}"
        self.statement(stmnt)

    @unittest.expectedFailure
    def test_multi_comp_ifs2(self):
        stmnt = "y = {a() +1:q for b in c for d in e if adsf}"
        self.statement(stmnt)


if __name__ == "__main__":
    # import sys;sys.argv = ['', 'Test.testName']
    unittest.main()
