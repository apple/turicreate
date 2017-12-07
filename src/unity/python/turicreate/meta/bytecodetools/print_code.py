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
from .bytecode_consumer import ByteCodeConsumer
from argparse import ArgumentParser

class ByteCodePrinter(ByteCodeConsumer):
    
    def generic_consume(self, instr):
        print(instr)

def main():
    parser = ArgumentParser()
    parser.add_argument()

if __name__ == '__main__':
    main()
