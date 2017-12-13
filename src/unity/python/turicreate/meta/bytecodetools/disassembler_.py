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

import dis
import sys
from . import Instruction

py3 = sys.version_info.major >= 3


if sys.version_info < (3, 4):
    co_ord = (lambda c:c) if py3 else ord
    def _walk_ops(code):
        """
        Yield (offset, opcode, argument number) tuples for all
        instructions in *code*.
        """
        code = getattr(code, 'co_code', b'')
        code = [co_ord(instr) for instr in code]

        n = len(code)
        i = 0
        extended_arg = 0
        while i < n:
            op = code[i]
            i += 1
            if op >= dis.HAVE_ARGUMENT:
                oparg = code[i] + code[i + 1] * 256 + extended_arg
                extended_arg = 0
                i += 2
                if op == dis.EXTENDED_ARG:
                    extended_arg = oparg * 65536
            yield i, op, oparg

else:
    def _walk_ops(code):
        """
        Yield (offset, opcode, argument number) tuples for all
        instructions in *code*.
        """
        for instr in dis.get_instructions(code):
            op = instr.opcode
            yield instr.offset, op, instr.arg


def disassembler(co, lasti= -1):
    """Disassemble a code object. 
    
    :param co: code object
    :param lasti: internal
    :yields: Instructions.
    
    """

    code = co.co_code
    labels = dis.findlabels(code)
    linestarts = dict(dis.findlinestarts(co))
    i = 0
    extended_arg = 0
    lineno = 0
    free = None
    for i, op, oparg in _walk_ops(co):
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

        instr.oparg = oparg
        extended_arg = 0
        if op == dis.EXTENDED_ARG:
            extended_arg = oparg * 65536
        instr.extended_arg = extended_arg
        if op >= dis.HAVE_ARGUMENT:
            if op in dis.hasconst:
                instr.arg = co.co_consts[oparg]
            elif op in dis.hasname:
                instr.arg = co.co_names[oparg]
            elif op in dis.hasjrel:
                instr.arg = i + oparg
            elif op in dis.haslocal:
                instr.arg = co.co_varnames[oparg]
            elif op in dis.hascompare:
                instr.arg = dis.cmp_op[oparg]
            elif op in dis.hasfree:
                if free is None:
                    free = co.co_cellvars + co.co_freevars
                instr.arg = free[oparg]

        yield instr

