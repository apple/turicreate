# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
# All rights reserved.
# This software may be modified and distributed under the terms
# of the BSD license. See the LICENSE file for details.
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import sys
import parser
import symbol
import token
import ast
import inspect
from ..import meta

class expression_validator(ast.NodeVisitor):
    """
    This tree walk attempts to validate an expression: that the expression
    should *not* contain certain names.
    This is used for the case

        x = 10
        lambda x: fn(x+15, x)

    Really, the "x+15" expression is invalid since the expression uses an 
    lambda argument. However, it does evaluate correctly in the scope
    since "x" also exists in the function scope.

    We thus need to validate the expression before attempting to evaluate it
    so that the expression must not contain a lambda argument.

    This validator here is a lot stricter than it should since it will also 
    prevent all cases where something with the same name as the lambda argument
    is created in an inner scope. For instance:

        lambda x: fn((lambda x: x + 15)(5), x)

        lambda x: fn(([x for x in [1,2,3]], x)
    """

    def __init__(self, blocked_symbols):
        self.blocked_symbols = blocked_symbols

    def visit_Name(self, node):
        if node.id in self.blocked_symbols:
            raise RuntimeError("Blocked symbols encountered")


class attribute_reader(ast.NodeVisitor):
    """
    Things like tc.extensions._demo_add
    get parsed as 

        Attribute(value=Attribute(value=Name(id='gl', ctx=Load()), 
        attr='extensions', ctx=Load()), attr='_demo_add', ctx=Load())

    This causes problems for  

        lambda x: tc.extensions._demo_add(x, 5)
    
    We need to breakdown the attribute into the original string
    """
    def default(self, node):
        raise NotImplementedError("Cannot process token at " + 
                str(node.lineno) + ":" + str(node.col_offset))

    def visit_Name(self, node):
        return node.id

    def visit_Attribute(self, node):
        s = self.visit(node.value)
        return s + "." + node.attr


class Parameter(object):
    def __init__(self, name):
        self.name = name

    def __str__(self):
        return 'λ' + self.name

    def __repr__(self):
        return str(self)

class lambda_closure_visitor(ast.NodeVisitor):
    """
    This implements a *very* limited decompiler. It only handles cases of
    lambda x: fn(a, b, x, ...) 
    where a,b, etc are variables captured from the surrounding scope, and there 
    may be some occurrences of x.
    No additional statements or expressions are permitted
    """
    FUNCTION = 0 # I am translating the wrapping lambda function
    INNER_CALL = 1 # I am translating the function call inside
    PARAMETER = 2 # I am just translating a function parameter 
    def __init__(self):
        # The fn
        self.closure_fn_name = ""

        # A list of captured positional arguments 
        # lambda parameters are denoted by being of type Parameter
        self.positional_args = []
        # A dictionary of captured named arguments 
        # lambda parameters are denoted by being of type Parameter
        self.named_args = {}

        # List of all the input argument names
        self.input_arg_names = []

        self.caller_globals = []

        self.state = self.FUNCTION

    def default(self, node):
        raise NotImplementedError("Cannot process token at " + 
                str(node.lineno) + ":" + str(node.col_offset))
    
    def __repr__(self):
        return str(self)

    def __str__(self):
        ret = self.closure_fn_name + "(" 
        comma = False
        for i in self.positional_args:
            if comma:
                ret = ret + ','
            ret = ret + str(i)
            comma = True

        for i in self.named_args:
            if comma:
                ret = ret + ','
            ret = ret + i + ":" + str(self.named_args[i])
            comma = True
        ret = ret + ")"
        return ret

    def translate_ast(self, ast_node):
        #print(ast.dump(ast_node))
        t = self.visit(ast_node)

    def visit_Module(self, node):
        if (self.state != self.FUNCTION):
            raise NotImplementedError("Unexpected module in position " + 
                    str(node.lineno) + ":" + str(node.col_offset))
        for line in node.body:
            self.visit(line)

    def visit_Call(self, node):
        if (self.state != self.INNER_CALL):
            raise NotImplementedError("Unexpected call in position " + 
                    str(node.lineno) + ":" + str(node.col_offset))
        self.state = self.INNER_CALL

        # this is the main closure function call
        if self.closure_fn_name != "":
            raise NotImplementedError("Cannot translate function call " + 
                    str(node.lineno) + ":" + str(node.col_offset))
        elif type(node.func) is ast.Name:
            self.closure_fn_name = node.func.id
        elif type(node.func) is ast.Attribute:
            self.closure_fn_name = attribute_reader().visit(node.func)
        else:
            raise NotImplementedError("Unexpected type of function call.")

        self.state = self.PARAMETER

        for i in range(len(node.args)):
            arg = node.args[i]
            if type(arg) is ast.Name and arg.id in self.input_arg_names:
                self.positional_args += [Parameter(arg.id)]
            else:
                try:
                    expression_validator(self.input_arg_names).visit(arg)
                    # try to evaluate the ast
                    result = eval(compile(ast.Expression(arg), '<string>', 'eval'), self.caller_globals)
                except:
                    raise NotImplementedError("Only simple expressions not using the function arguments are permitted")
                self.positional_args += [result]

        # keyword arguments next 
        keywordargs = {i.arg:i.value for i in node.keywords}
        for i in keywordargs:
            arg = keywordargs[i]
            if type(arg) is ast.Name and arg.id in self.input_arg_names:
                self.named_args[i] = Parameter(arg.id)
            else:
                try:
                    expression_validator(self.input_arg_names).visit(arg)
                    # try to evaluate the ast
                    result = eval(compile(ast.Expression(arg), '<string>', 'eval'), self.caller_globals)
                except:
                    raise NotImplementedError("Only simple expressions not using the function arguments are permitted")
                self.named_args[i] = result


            
    def visit_arguments(self, node):
        if (self.state != self.FUNCTION):
            raise NotImplementedError("Unexpected function")
        if sys.version_info.major == 2:
            self.input_arg_names = [arg.id for arg in node.args]
        else:
            self.input_arg_names = [arg.arg for arg in node.args]

    def visit_Name(self, node):
            raise NotImplementedError("Unexpected name")

    def visit_Return(self, node):
        if (self.state != self.INNER_CALL):
            raise NotImplementedError("Unexpected return") 
        return self.visit(node.value)

    def visit_Lambda(self, node):
        return self.visit_FunctionDef(node)


    def visit_FunctionDef(self, node):
        if (self.state != self.FUNCTION):
            raise NotImplementedError("Unexpected function") 

        self.visit(node.args)

        self.state = self.INNER_CALL
        if type(node.body) is list:
            next_node = node.body[0]
            # there is this annoying condition in which if there is a doc string,
            # it actually shows up in the ast as a Expr.str
            # so we need to catch that and skip it
            try:
              if type(next_node) is ast.Expr and type(next_node.value) is ast.Str:
                # this is *probably* a doc string!
                next_node = node.body[1]
            except:
                # just in case the above fails for various reasons like say...
                # there is *only* a doc string. We still fail with the 
                # appropriate error
                pass
        else:
            next_node = node.body

        if type(next_node) is ast.Call:
            self.visit(next_node)
        elif type(next_node) is ast.Return and type(next_node.value) is ast.Call:
            self.visit(next_node.value)
        else:
            raise NotImplementedError("Function must comprise of just a function call ")

    def visit_ClassDef(self, node):
        raise NotImplementedError("Classes are not implemented")

def _isalambda(v):
    return isinstance(v, type(lambda: None)) and v.__name__ == '<lambda>'


def translate(fn):
    visitor = lambda_closure_visitor()
    if sys.version_info.major == 2:
        visitor.caller_globals = fn.__globals__.copy()
        func_closure = fn.__closure__
        co_freevars = fn.__code__.co_freevars
    else:
        visitor.caller_globals = fn.__globals__.copy()
        func_closure = fn.__closure__
        co_freevars = fn.__code__.co_freevars
    # now. annoyingly enough certain captures are not here. We need to 
    # look in func_closures for it
    if func_closure:
        closure = dict(zip(co_freevars, (c.cell_contents for c in func_closure)))
        # inject closure into "caller_globals"
        for i in closure:
            visitor.caller_globals[i] = closure[i]

    ast_node = None
    try:
        if not _isalambda(fn):
            ast_node = ast.parse(inspect.getsource(fn))
    except:
        pass

    try:
        if ast_node is None:
            ast_node = meta.decompiler.decompile_func(fn)
    except:
        pass

    if ast_node is None:
        raise RuntimeError("Cannot process provided function")
    visitor.translate_ast(ast_node)
    return visitor
# if __name__ == "__main__":
#     if len(sys.argv) <= 1:
#         print("Usage:\n\t./Lua_Translator.py <FILENAME>\n")
#         exit(-1)
#     f = open(sys.argv[1] , 'r')
#     l = f.readlines()
#     f.close()
#     s = ""
#
#     for x in l:
#         s = s + x
#
#     ast_node = ast.parse(s)
#
#     f = open(sys.argv[1].rpartition(".")[0] + "_trans.lua", 'w')
#     test = translator_NodeVisitor(f)
#     test.translate_ast(ast_node)
#     f.close()
