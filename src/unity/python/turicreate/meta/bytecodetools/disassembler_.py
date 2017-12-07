# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
Created on May 10, 2012

@author: sean
'''

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import opcode
from dis import findlabels, findlinestarts
import sys
from . import Instruction

py3 = sys.version_info.major >= 3
co_ord = (lambda c:c) if py3 else ord

def disassembler(co, lasti= -1):
    """Disassemble a code object. 
    
    :param co: code object
    :param lasti: internal
    :yields: Instructions.
    
    """

    code = co.co_code
    labels = findlabels(code)
    linestarts = dict(findlinestarts(co))
    n = len(code)
    i = 0
    extended_arg = 0
    lineno = 0
    free = None
    while i < n:
        c = code[i]
        op = co_ord(c)
    
    
        if i in linestarts:
            lineno = linestarts[i]

        instr = Instruction(i=i, op=op, lineno=lineno)
        instr.linestart = i in linestarts

        if i == lasti:
            instr.lasti = True
        else:
            instr.lasti = False

        if i in labels:
            instr.label = True
        else:
            instr.label = False

        i = i + 1
        if op >= opcode.HAVE_ARGUMENT:
            oparg = co_ord(code[i]) + co_ord(code[i + 1]) * 256 + extended_arg
            instr.oparg = oparg
            extended_arg = 0
            i = i + 2
            if op == opcode.EXTENDED_ARG:
                extended_arg = oparg * 65536
            instr.extended_arg = extended_arg
            if op in opcode.hasconst:
                instr.arg = co.co_consts[oparg]
            elif op in opcode.hasname:
                instr.arg = co.co_names[oparg]
            elif op in opcode.hasjrel:
                instr.arg = i + oparg
            elif op in opcode.haslocal:
                instr.arg = co.co_varnames[oparg]
            elif op in opcode.hascompare:
                instr.arg = opcode.cmp_op[oparg]
            elif op in opcode.hasfree:
                if free is None:
                    free = co.co_cellvars + co.co_freevars
                instr.arg = free[oparg]

        yield instr

