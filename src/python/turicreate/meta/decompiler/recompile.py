# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
Created on Nov 3, 2011

@author: sean
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import os
import sys
import struct
from time import time

py3 = sys.version_info.major >= 3

if py3:
    import builtins #@UnresolvedImport
else:
    import __builtin__ as builtins
     
import marshal
import imp
from py_compile import PyCompileError, wr_long

MAGIC = imp.get_magic()

def create_pyc(codestring, cfile, timestamp=None):

    if timestamp is None:
        timestamp = time()
    
    codeobject = builtins.compile(codestring, '<recompile>', 'exec')
        
    cfile.write(MAGIC)
    cfile.write(struct.pack('i', timestamp))
    marshal.dump(codeobject, cfile)
    cfile.flush()
    
