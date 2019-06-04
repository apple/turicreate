# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
Created on Nov 5, 2011

@author: sean
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import sys
import unittest

py2 = sys.version_info.major < 3
py3 = not py2

py2only = unittest.skipIf(not py2, "Only valid for python 2.x")

py3only = unittest.skipIf(not py3, "Only valid for python 3.x")
