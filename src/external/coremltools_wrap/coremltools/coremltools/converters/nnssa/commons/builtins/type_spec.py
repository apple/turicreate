# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _


class Type:
    """
     - Type.name : A string with the name of the object
     - Type.tparam : For classes with template parameters, (list, dict), this 
         contains a list of Type objects of the template parameters
     - Type.python_class : The original python class implementing this type. 
                         Two Type objects compare equal
                         only on name and tparam and not python_class
    """
    __slots__ = ['name', 'tparam', 'python_class']

    def __init__(self, name, tparam=[], python_class=None):
        assert (isinstance(name, str))
        assert (isinstance(tparam, list))
        self.name = name
        self.tparam = tparam
        self.python_class = python_class

    def __hash__(self):
        return hash((self.name, tuple(self.tparam)))

    def __eq__(self, other):
        return self.name == other.name and self.tparam == other.tparam

    def __ne__(self, other):
        return not self.__eq__(other)

    def __repr__(self):
        ret = self.name
        if len(self.tparam) > 0:
            ret += '[' + ','.join(repr(x) for x in self.tparam) + ']'
        return ret

    def __str__(self):
        return self.__repr__()

    def sexp(self):
        if len(self.tparam) == 0:
            return self.name
        else:
            ret = [self.name]
            ret.append([a.sexp() if hasattr(a, 'sexp') else a for a in self.tparam])
            return ret


class FunctionType:
    """
    - FunctionType.inputs : A list of Type objects defining the types of the input
    - FunctionType.output: A Type object defining the type of the output
    - FunctionType.python\_function : The original python function implementing 
                                      this type. Two FunctionType objects compare 
                                      equal only on inputs and output and not 
                                      python_function 
    """
    __slots__ = ['inputs', 'output', 'python_function']

    def __init__(self, inputs, output, python_function=None):
        assert (isinstance(inputs, list))
        assert (isinstance(output, (FunctionType, Type)))
        self.inputs = inputs
        self.output = output
        self.python_function = python_function

    def __hash__(self):
        return hash((tuple(self.inputs), self.output))

    def __eq__(self, other):
        return self.inputs == other.inputs and self.output == other.output

    def __repr__(self):
        return '(' + ','.join(repr(x) for x in self.inputs) + ')->' + repr(self.output)

    def __str__(self):
        return self.__repr__()

    def return_sexp(self):
        return self.output.sexp()

    def inputs_sexp(self):
        return [i.sexp() for i in self.inputs]
