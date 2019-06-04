# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
Created on Nov 4, 2011

@author: sean
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import sys

py3 = sys.version_info.major >= 3

class Python2(object):
    @staticmethod
    def py2op(func):
        return func
    
    def __init__(self,*args, **kwargs):
        raise NotImplementedError("This function is not implemented in python 2.x")

def py3op(func):
    if py3:
        
        func.py2op = lambda _:func
        return func
    else:
        return Python2

class Python3(object):

    def __init__(self,*args, **kwargs):
        raise NotImplementedError("This function is not implemented in python 3.x")
    
    @staticmethod
    def py3op(func):
        return func

def py2op(func):
    if not py3:
        func.py3op = lambda _:func
        return func
    else:
        return Python3
