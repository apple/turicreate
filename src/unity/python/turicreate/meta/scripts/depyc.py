# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
Decompile python byte encoded modules code. 
Created on Jul 19, 2011

@author: sean
'''

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from argparse import ArgumentParser, FileType

import sys
import ast

from ..asttools import print_ast, python_source
from ..bytecodetools.pyc_file import extract 
from ..decompiler.instructions import make_module
from ..decompiler.disassemble import print_code
from ..decompiler.recompile import create_pyc
import os

py3 = sys.version_info.major >= 3

def depyc(args):
    
    binary = args.input.read()
    modtime, code = extract(binary)
    
    print("Decompiling module %r compiled on %s" % (args.input.name, modtime,), file=sys.stderr)
    
    if args.output_type == 'pyc':
        if py3 and args.output is sys.stdout:
            args.output = sys.stdout.buffer
        args.output.write(binary)
        return
            
    if args.output_type == 'opcode':
        print_code(code)
        return 
    
    mod_ast = make_module(code)
    
    if args.output_type == 'ast':
        print_ast(mod_ast, file=args.output)
        return 
    
    if args.output_type == 'python':
        python_source(mod_ast, file=args.output)
        return
        
    
    raise  Exception("unknown output type %r" % args.output_type)

def src_tool(args):
    print("Analysing python module %r" % (args.input.name,), file=sys.stderr)
    
    source = args.input.read()
    mod_ast = ast.parse(source, args.input.name)
    code = compile(source, args.input.name, mode='exec', dont_inherit=True)
    
    if args.output_type == 'opcode':
        print_code(code)
        return 
    elif args.output_type == 'ast':
        print_ast(mod_ast, file=args.output)
        return 
    elif args.output_type == 'python':
        print(source.decode(), file=args.output)
    elif args.output_type == 'pyc':
        
        if py3 and args.output is sys.stdout:
            args.output = sys.stdout.buffer

        try:
            timestamp = int(os.fstat(args.input.fileno()).st_mtime)
        except AttributeError:
            timestamp = int(os.stat(args.input.name).st_mtime)
        if py3 and args.output is sys.stdout:
            args.output = sys.stdout.buffer
        create_pyc(source, cfile=args.output, timestamp=timestamp)
    else:
        raise  Exception("unknown output type %r" % args.output_type)

    return
    
def setup_parser(parser):
    parser.add_argument('input', type=FileType('rb'))
    parser.add_argument('-t', '--input-type', default='from_filename', dest='input_type')
    
    parser.add_argument('-o', '--output', default='-', type=FileType('wb'))
    
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--python', default='python', action='store_const', const='python',
                        dest='output_type')
    group.add_argument('--ast', action='store_const', const='ast',
                        dest='output_type')
    group.add_argument('--opcode', action='store_const', const='opcode',
                        dest='output_type')
    group.add_argument('--pyc', action='store_const', const='pyc',
                        dest='output_type')
    
def main():
    parser = ArgumentParser(description=__doc__)
    setup_parser(parser)
    args = parser.parse_args(sys.argv[1:])
    
    input_python = args.input.name.endswith('.py') if args.input_type == 'from_filename' else args.input_type == 'python'
    
    if input_python:
        src_tool(args)
    else:
        if py3 and args.input is sys.stdin:
            args.input = sys.stdin.buffer
        depyc(args)
        
if __name__ == '__main__':
    main()

