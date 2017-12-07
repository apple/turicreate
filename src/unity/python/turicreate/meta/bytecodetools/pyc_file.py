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

from imp import get_magic
import time
import struct
import marshal

def extract(binary):
    '''
    Extract a code object from a binary pyc file.
    
    :param binary: a sequence of bytes from a pyc file.
    '''
    if len(binary) <= 8:
        raise Exception("Binary pyc must be greater than 8 bytes (got %i)" % len(binary))
    
    magic = binary[:4]
    MAGIC = get_magic()
    
    if magic != MAGIC:
        raise Exception("Python version mismatch (%r != %r) Is this a pyc file?" % (magic, MAGIC))
    
    modtime = time.asctime(time.localtime(struct.unpack('i', binary[4:8])[0]))

    code = marshal.loads(binary[8:])
    
    return modtime, code

