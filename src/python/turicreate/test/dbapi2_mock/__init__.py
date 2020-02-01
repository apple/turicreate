# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _


class dbapi2_mock(object):
    def __init__(self):
        # Mandated globals
        self.apilevel = "2.0 "
        self.threadsafety = 3
        self.paramstyle = "qmark"
        self.STRING = 41
        self.BINARY = 42
        self.DATETIME = 43
        self.NUMBER = 44
        self.ROWID = 45
        self.Error = Exception  # StandardError not in python 3
