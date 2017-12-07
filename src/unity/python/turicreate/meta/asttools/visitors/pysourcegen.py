# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
Created on Jul 15, 2011

@author: sean
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import _ast
from ...asttools import Visitor
from string import Formatter
import sys
from ...utils import py3op, py2op

if sys.version_info.major < 3:
    from StringIO import StringIO
else:
    from io import StringIO


class ASTFormatter(Formatter):

    def format_field(self, value, format_spec):
        if format_spec == 'node':
            gen = ExprSourceGen()
            gen.visit(value)
            return gen.dumps()
        elif value == '':
            return value
        else:
            return super(ASTFormatter, self).format_field(value, format_spec)

    def get_value(self, key, args, kwargs):
        if key == '':
            return args[0]
        elif key in kwargs:
            return kwargs[key]
        elif isinstance(key, int):
            return args[key]

        key = int(key)
        return args[key]

        raise Exception

def str_node(node):
    gen = ExprSourceGen()
    gen.visit(node)
    return gen.dumps()

def simple_string(value):
    def visitNode(self, node):
        self.print(value, **node.__dict__)
    return visitNode

class ExprSourceGen(Visitor):
    def __init__(self):
        self.out = StringIO()
        self.formatter = ASTFormatter()
        self.indent = '    '
        self.level = 0


    @property
    def indenter(self):
        return Indenter(self)

    @property
    def no_indent(self):
        return NoIndent(self)

    def dump(self, file=sys.stdout):
        self.out.seek(0)
        print(self.out.read(), file=file)

    def dumps(self):
        self.out.seek(0)
        value = self.out.read()
        return value

    def print(self, line, *args, **kwargs):
        line = self.formatter.format(line, *args, **kwargs)

        level = kwargs.get('level')
        prx = self.indent * (level if level else self.level)
        print(prx, line, sep='', end='', file=self.out)

    def print_lines(self, lines,):
        prx = self.indent * self.level
        for line in lines:
            print(prx, line, sep='', file=self.out)

    def visitName(self, node):
        self.print(node.id)
    
    @py2op
    def visitarguments(self, node):
        # ('args', 'vararg', 'kwarg', 'defaults')
        defaults = [None] * (len(node.args) - len(node.defaults))
        defaults.extend(node.defaults)

        i = 0
        args = list(node.args)
        if args:
            i += 1
            arg = args.pop(0)
            default = defaults.pop(0)
            self.visit(arg)
            if default is not None:
                self.print('={:node}', default)

        while args:
            arg = args.pop(0)
            default = defaults.pop(0)
            self.print(', ')
            self.visit(arg)
            if default is not None:
                self.print('={:node}', default)

        if node.vararg:
            self.print('{0}*{1}', ', ' if i else '', node.vararg)
        if node.kwarg:
            self.print('{0}**{1}', ', ' if i else '', node.kwarg)

    @visitarguments.py3op
    def visitarguments(self, node):
        # ('args', 'vararg', 'kwarg', 'defaults')
        defaults = [None] * (len(node.args) - len(node.defaults))
        defaults.extend(node.defaults)

        i = 0
        args = list(node.args)
        if args:
            i += 1
            arg = args.pop(0)
            default = defaults.pop(0)
            self.visit(arg)
            if default is not None:
                self.print('={:node}', default)

        while args:
            arg = args.pop(0)
            default = defaults.pop(0)
            self.print(', ')
            self.visit(arg)
            if default is not None:
                self.print('={:node}', default)

        if node.vararg:
            self.print('{0}*{1}', ', ' if i else '', node.vararg)
            if node.varargannotation:
                self.print(':{:node}', node.varargannotation)
        elif node.kwonlyargs:
            self.print('{0}*', ', ' if i else '')
        
        kwonlyargs = list(node.kwonlyargs)
        
        if kwonlyargs:
            i += 1
            kw_defaults = [None] * (len(kwonlyargs) - len(node.kw_defaults))
            kw_defaults.extend(node.kw_defaults)
            
        while kwonlyargs:
            kw_arg = kwonlyargs.pop(0)
            kw_default = kw_defaults.pop(0)
            self.print(', ')
            self.visit(kw_arg)
            if kw_default is not None:
                self.print('={:node}', kw_default)
        
        if node.kwarg:
            self.print('{0}**{1}', ', ' if i else '', node.kwarg)
            if node.varargannotation:
                self.print(':{:node}', node.kwargannotation)

    def visitNum(self, node):
        self.print(repr(node.n))

    def visitBinOp(self, node):
        self.print('({left:node} {op:node} {right:node})', left=node.left, op=node.op, right=node.right)

    def visitAdd(self, node):
        self.print('+')

    def visitalias(self, node):
        if node.asname is None:
            self.print("{0}", node.name)
        else:
            self.print("{0} as {1}", node.name, node.asname)

    def visitCall(self, node):

        self.print('{func:node}(' , func=node.func)
        i = 0

        print_comma = lambda i: self.print(", ") if i > 0 else None
        with self.no_indent:

            for arg in node.args:
                print_comma(i)
                self.print('{:node}', arg)
                i += 1

            for kw in node.keywords:
                print_comma(i)
                self.print('{:node}', kw)
                i += 1

            if node.starargs:
                print_comma(i)
                self.print('*{:node}', node.starargs)
                i += 1

            if node.kwargs:
                print_comma(i)
                self.print('**{:node}', node.kwargs)
                i += 1

            self.print(')')

    def visitkeyword(self, node):
        self.print("{0}={1:node}", node.arg, node.value)

    def visitStr(self, node):
        self.print(repr(node.s))

    def visitMod(self, node):
        self.print('%')

    def visitTuple(self, node, brace='()'):
        self.print(brace[0])

        print_comma = lambda i: self.print(", ") if i > 0 else None

        i = 0
        with self.no_indent:
            for elt in  node.elts:
                print_comma(i)
                self.print('{:node}', elt)
                i += 1
                
            if len(node.elts) == 1:
                self.print(',')

            self.print(brace[1])

    def visitCompare(self, node):
        self.print('({0:node} ', node.left)
        with self.no_indent:
            for (op, right) in zip(node.ops, node.comparators):
                self.print('{0:node} {1:node}' , op, right)
            self.print(')')

    @py2op
    def visitRaise(self, node):
        self.print('raise ')
        with self.no_indent:
            if node.type:
                self.print('{:node}' , node.type)
            if node.inst:
                self.print(', {:node}' , node.inst)
            if node.tback:
                self.print(', {:node}' , node.tback)
                
    @visitRaise.py3op
    def visitRaise(self, node):
        self.print('raise ')
        with self.no_indent:
            if node.exc:
                self.print('{:node}' , node.exc)
            if node.cause:
                self.print(' from {:node}' , node.cause)

    def visitAttribute(self, node):
        self.print('{:node}.{attr}', node.value, attr=node.attr)

    def visitDict(self, node):
        self.print('{{')

        items = zip(node.keys, node.values)

        with self.no_indent:
            i = 0
            pc = lambda : self.print(", ") if i > 0 else None

            for key, value in items:
                pc()
                self.print('{0:node}:{1:node}', key, value)
                i += 1

            self.print('}}')

    def visitSet(self, node):
        self.print('{{')

        items = node.elts

        with self.no_indent:
            i = 0
            pc = lambda : self.print(", ") if i > 0 else None

            for value in items:
                pc()
                self.print('{0:node}', value)
                i += 1

            self.print('}}')

    def visitList(self, node):
        self.print('[')

        with self.no_indent:
            i = 0
            pc = lambda : self.print(", ") if i > 0 else None

            for item in node.elts:
                pc()
                self.print('{:node}', item)
                i += 1
            self.print(']')

    def visitSubscript(self, node):

        self.print('{0:node}[{1:node}]', node.value, node.slice)

    def visitIndex(self, node):
        if isinstance(node.value, _ast.Tuple):
            with self.no_indent:
                self.visit(node.value, brace=['', ''])
        else:
            self.print('{:node}', node.value)

    def visitSlice(self, node):
        with self.no_indent:
            if node.lower is not None:
                self.print('{:node}', node.lower)
            self.print(':')
            if node.upper is not None:
                self.print('{:node}', node.upper)

            if node.step is not None:
                self.print(':')
                self.print('{:node}', node.step)

    def visitExtSlice(self, node):

        dims = list(node.dims)
        with self.no_indent:
            dim = dims.pop(0)
            self.print('{0:node}', dim)
            
            while dims:
                dim = dims.pop(0)
                self.print(', {0:node}', dim)

    def visitUnaryOp(self, node):
        self.print('({0:node}{1:node})', node.op, node.operand)

    def visitAssert(self, node):
        self.print('assert {0:node}', node.test)

        if node.msg:
            with self.no_indent:
                self.print(', {0:node}', node.msg)

    visitUSub = simple_string('-')
    visitUAdd = simple_string('+')
    visitNot = simple_string('not ')
    visitInvert = simple_string('~')

    visitAnd = simple_string('and')
    visitOr = simple_string('or')

    visitSub = simple_string('-')
    visitFloorDiv = simple_string('//')
    visitDiv = simple_string('/')
    visitMod = simple_string('%')
    visitMult = simple_string('*')
    visitPow = simple_string('**')

    visitEq = simple_string('==')
    visitNotEq = simple_string('!=')

    visitLt = simple_string('<')
    visitGt = simple_string('>')

    visitLtE = simple_string('<=')
    visitGtE = simple_string('>=')

    visitLShift = simple_string('<<')
    visitRShift = simple_string('>>')

    visitIn = simple_string('in')
    visitNotIn = simple_string('not in')

    visitIs = simple_string('is')
    visitIsNot = simple_string('is not')

    visitBitAnd = simple_string('&')
    visitBitOr = simple_string('|')
    visitBitXor = simple_string('^')

    visitEllipsis = simple_string('...')

    visitYield = simple_string('yield {value:node}')

    def visitBoolOp(self, node):

        with self.no_indent:
            values = list(node.values)
            left = values.pop(0)

            self.print('({:node} ', left)
            while values:
                left = values.pop(0)
                self.print('{0:node} {1:node})', node.op, left)

    def visitIfExp(self, node):
        self.print('{body:node} if {test:node} else {orelse:node}', **node.__dict__)

    def visitLambda(self, node):
        self.print('lambda {0:node}: {1:node}', node.args, node.body)

    def visitListComp(self, node):
        self.print('[{0:node}', node.elt)


        generators = list(node.generators)
        with self.no_indent:
            while generators:
                generator = generators.pop(0)
                self.print('{0:node}', generator)

            self.print(']')

    def visitSetComp(self, node):
        self.print('{{{0:node}', node.elt)


        generators = list(node.generators)
        with self.no_indent:
            while generators:
                generator = generators.pop(0)
                self.print('{0:node}', generator)

            self.print('}}')

    def visitDictComp(self, node):
        self.print('{{{0:node}:{1:node}', node.key, node.value)


        generators = list(node.generators)
        with self.no_indent:
            while generators:
                generator = generators.pop(0)
                self.print('{0:node}', generator)

            self.print('}}')


    def visitcomprehension(self, node):
        self.print(' for {0:node} in {1:node}', node.target, node.iter)

        ifs = list(node.ifs)
        while ifs:
            if_ = ifs.pop(0)
            self.print(" if {0:node}", if_)

    @py3op
    def visitarg(self, node):
        self.print(node.arg)
        
        if node.annotation:
            with self.no_indent:
                self.print(':{0:node}', node.annotation)
    
def visit_expr(node):
    gen = ExprSourceGen()
    gen.visit(node)
    return gen.dumps()

class NoIndent(object):
    def __init__(self, gen):
        self.gen = gen

    def __enter__(self):
        self.level = self.gen.level
        self.gen.level = 0

    def __exit__(self, *args):
        self.gen.level = self.level

class Indenter(object):
    def __init__(self, gen):
        self.gen = gen

    def __enter__(self):
        self.gen.print('\n', level=0)
        self.gen.level += 1

    def __exit__(self, *args):
        self.gen.level -= 1

class SourceGen(ExprSourceGen):

    def __init__(self, header=''):
        super(SourceGen, self).__init__()
        print(header, file=self.out)

    def visitModule(self, node):

        children = list(self.children(node))
        if children and isinstance(children[0], _ast.Expr):
            if isinstance(children[0].value, _ast.Str):
                doc = children.pop(0).value
                self.print("'''")
                self.print_lines(doc.s.split('\n'))
                self.print_lines(["'''", '\n', '\n'])

        for node in children:
            self.visit(node)

    def visitFor(self, node):
        self.print('for {0:node} in {1:node}:', node.target, node.iter)
        with self.indenter:
            for stmnt in node.body:
                self.visit(stmnt)

        if node.orelse:
            self.print('else:')
            with self.indenter:
                for stmnt in node.orelse:
                    self.visit(stmnt)

    @py2op
    def visitFunctionDef(self, node):
        #fields = ('name', 'args', 'body', 'decorator_list')

        for decorator in node.decorator_list:
            self.print('@{decorator:node}\n', decorator=decorator)

        args = visit_expr(node.args)
        self.print('def {name}({args}):' , name=node.name, args=args)

        with self.indenter:
            for child in node.body:
                self.visit(child)
        return
    
    @visitFunctionDef.py3op
    def visitFunctionDef(self, node):

        for decorator in node.decorator_list:
            self.print('@{decorator:node}\n', decorator=decorator)

        args = visit_expr(node.args)
        self.print('def {name}({args})' , name=node.name, args=args)
        
        with self.no_indent:
            if node.returns:
                self.print(' -> {:node}:', node.returns)
            else:
                self.print(':', node.returns)
            
        with self.indenter:
            for child in node.body:
                self.visit(child)
        return

    def visitAssign(self, node):
        targets = [visit_expr(target) for target in node.targets]

        self.print('{targets} = {value:node}\n', targets=' = '.join(targets), value=node.value)

    def visitAugAssign(self, node):
        self.print('{target:node} {op:node}= {value:node}\n', **node.__dict__)

    def visitIf(self, node, indent_first=True):

        self.print('if {:node}:', node.test, level=self.level if indent_first else 0)

        with self.indenter:
            if node.body:
                for expr in node.body:
                    self.visit(expr)
            else:
                self.print('pass')
                    

        if node.orelse and len(node.orelse) == 1 and isinstance(node.orelse[0], _ast.If):
            self.print('el'); self.visit(node.orelse[0], indent_first=False)
        elif node.orelse:
            self.print('else:')
            with self.indenter:
                for expr in node.orelse:
                    self.visit(expr)
        self.print('\n')

    def visitImportFrom(self, node):
        for name in node.names:
            self.print("from {0} import {1:node}\n", node.module, name)

    def visitImport(self, node):
        for name in node.names:
            self.print("import {:node}\n", name)

    def visitPrint(self, node):
        self.print("print ")

        with self.no_indent:
            if node.dest:
                self.print(">> {:node}" , node.dest)
                if not node.values and node.nl:
                    self.print("\n")
                    return

                self.print(", ")

            i = 0
            pc = lambda : self.print(", ") if i > 0 else None
            for value in  node.values:
                pc()
                self.print("{:node}" , value)

            if not node.nl:
                self.print(",")

            self.print("\n")

    def visitExec(self, node):
        self.print('exec {0:node} in {1}, {2}\n', node.body,
                   'None' if node.globals is None else str_node(node.globals),
                   'None' if node.locals is None else str_node(node.locals))

    def visitWith(self, node):
        self.print('with {0:node}', node.context_expr)
        if node.optional_vars is not None:
            self.print(' as {0:node}', node.optional_vars, level=0)
        self.print(':', level=0)

        with self.indenter:
            if node.body:
                for expr in node.body:
                    self.visit(expr)
            else:
                self.print('pass\n')

    def visitGlobal(self, node):
        self.print('global ')
        with self.no_indent:
            names = list(node.names)
            if names:
                name = names.pop(0)
                self.print(name)
            while names:
                name = names.pop(0)
                self.print(', {0}', name)
            self.print('\n')


    def visitDelete(self, node):
        self.print('del ')

        targets = list(node.targets)

        with self.no_indent:
            target = targets.pop(0)
            self.print('{0:node}', target)
            while targets:
                target = targets.pop(0)
                self.print(', {0:node}', target)
            self.print('\n')

    def visitWhile(self, node):
        self.print('while {0:node}:', node.test)

        with self.indenter:
            if node.body:
                for expr in node.body:
                    self.visit(expr)
            else:
                self.print("pass")
                    

        if node.orelse:
            self.print('else:')
            with self.indenter:
                for expr in node.orelse:
                    self.visit(expr)
            self.print('\n')
        self.print('\n')


    def visitExpr(self, node):
        self.print('{:node}\n', node.value)

    visitBreak = simple_string('break\n')
    visitPass = simple_string('pass\n')
    visitContinue = simple_string('continue\n')

    def visitReturn(self, node):
        if node.value is not None:
            self.print('return {:node}\n', node.value)

    def visitTryExcept(self, node):
        self.print('try:')

        with self.indenter:
            if node.body:
                for stmnt in node.body:
                    self.visit(stmnt)
            else:
                self.print('pass')
                    
        for hndlr in node.handlers:
            self.visit(hndlr)

        if node.orelse:
            self.print('else:')
            with self.indenter:
                for stmnt in node.orelse:
                    self.visit(stmnt)
    @py2op
    def visitExceptHandler(self, node):

        self.print('except')

        with self.no_indent:
            if node.type:
                self.print(" {0:node}", node.type)
            if node.name:
                self.print(" as {0:node}", node.name)

            self.print(":")

        with self.indenter:
            if node.body:
                for stmnt in node.body:
                    self.visit(stmnt)
            else:
                self.print('pass')
                
    @visitExceptHandler.py3op
    def visitExceptHandler(self, node):
        self.print('except')

        with self.no_indent:
            if node.type:
                self.print(" {0:node}", node.type)
            if node.name:
                self.print(" as {0}", node.name)

            self.print(":")

        with self.indenter:
            for stmnt in node.body:
                self.visit(stmnt)
                
                
    def visitTryFinally(self, node):
        for item in node.body:
            self.visit(item)

        self.print('finally:')

        with self.indenter:
            for item in node.finalbody:
                self.visit(item)
    
    @py2op
    def visitClassDef(self, node):

        for decorator in node.decorator_list:
            self.print('@{0:node}\n', decorator)

        self.print('class {0}', node.name)

        with self.no_indent:
            self.print('(')
            bases = list(node.bases)
            if bases:
                base = bases.pop(0)
                self.print("{0:node}", base)
                while bases:
                    base = bases.pop(0)
                    self.print(", {0:node}", base)
            self.print(')')

            self.print(":")

        with self.indenter:
            if node.body:
                for stmnt in node.body:
                    self.visit(stmnt)
            else:
                self.print("pass\n\n")

    @visitClassDef.py3op
    def visitClassDef(self, node):

        for decorator in node.decorator_list:
            self.print('@{0:node}\n', decorator)

        self.print('class {0}', node.name)

        with self.no_indent:
            self.print('(')
            bases = list(node.bases)
            i = 0
            if bases:
                i += 1
                base = bases.pop(0)
                self.print("{0:node}", base)
                while bases:
                    base = bases.pop(0)
                    self.print(", {0:node}", base)
            keywords = list(node.keywords)
            
            if keywords:
                if i: self.print(', ')
                i += 1
                keyword = keywords.pop(0)
                self.print("{0:node}", keyword)
                while keywords:
                    base = keywords.pop(0)
                    self.print(", {0:node}", keyword)
            
            if node.starargs:
                if i: self.print(', ')
                i += 1
                self.print("*{0:node}", node.starargs)

            if node.kwargs:
                if i: self.print(', ')
                i += 1
                self.print("*{0:node}", node.kwargs)
                
            self.print(')')

            self.print(":")

        with self.indenter:
            if node.body:
                for stmnt in node.body:
                    self.visit(stmnt)
            else:
                self.print("pass\n\n")

def python_source(ast, file=sys.stdout):
    '''
    Generate executable python source code from an ast node.
      
    :param ast: ast node
    :param file: file to write output to.
    '''
    gen = SourceGen()
    gen.visit(ast)
    gen.dump(file)

def dump_python_source(ast):
    '''
    :return: a string containing executable python source code from an ast node.
      
    :param ast: ast node
    :param file: file to write output to.
    '''
    gen = SourceGen()
    gen.visit(ast)
    return gen.dumps()



