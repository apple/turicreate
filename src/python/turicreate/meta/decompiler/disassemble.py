# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
Created on Jul 14, 2011

@author: sean
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from dis import findlabels, findlinestarts
import types
import sys
from ..bytecodetools.disassembler_ import disassembler
import opcode

py3 = sys.version_info.major >= 3

co_ord = (lambda c:c) if py3 else ord

def disassemble(co):
    """Disassemble a code object."""
    return list(disassembler(co))

def print_code(co, lasti= -1, level=0):
    """Disassemble a code object."""
    code = co.co_code
    
    for constant in co.co_consts:
        print( '|              |' * level, end=' ')
        print( 'constant:', constant)
        
    labels = findlabels(code)
    linestarts = dict(findlinestarts(co))
    n = len(code)
    i = 0
    extended_arg = 0
    free = None
    while i < n:
        have_inner = False
        c = code[i]
        op = co_ord(c)

        if i in linestarts:
            if i > 0:
                print()
            print( '|              |' * level, end=' ')
            print( "%3d" % linestarts[i], end=' ')
        else:
            print( '|              |' * level, end=' ')
            print('   ', end=' ')

        if i == lasti: print( '-->',end=' ')
        else: print( '   ', end=' ')
        if i in labels: print( '>>', end=' ')
        else: print( '  ',end=' ')
        print(repr(i).rjust(4), end=' ')
        print(opcode.opname[op].ljust(20), end=' ')
        i = i + 1
        if op >= opcode.HAVE_ARGUMENT:
            oparg = co_ord(code[i]) + co_ord(code[i + 1]) * 256 + extended_arg
            extended_arg = 0
            i = i + 2
            if op == opcode.EXTENDED_ARG:
                extended_arg = oparg * 65536
            print( repr(oparg).rjust(5), end=' ')
            if op in opcode.hasconst:

                print( '(' + repr(co.co_consts[oparg]) + ')', end=' ')
                if type(co.co_consts[oparg]) == types.CodeType:
                    have_inner = co.co_consts[oparg]

            elif op in opcode.hasname:
                print( '(' + co.co_names[oparg] + ')',end=' ')
            elif op in opcode.hasjrel:
                print('(to ' + repr(i + oparg) + ')', end=' ')
            elif op in opcode.haslocal:
                print('(' + co.co_varnames[oparg] + ')', end=' ')
            elif op in opcode.hascompare:
                print('(' + opcode.cmp_op[oparg] + ')', end=' ')
            elif op in opcode.hasfree:
                if free is None:
                    free = co.co_cellvars + co.co_freevars
                print('(' + free[oparg] + ')', end=' ')
        print()

        if have_inner is not False:
            print_code(have_inner, level=level + 1)
