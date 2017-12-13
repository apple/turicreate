# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
""""
Dummy mocking module for pandas.
When pandas is not available we will import this module as turicreate.deps.pandas,
and set HAS_pandas to false. All methods that access pandas should check the HAS_pandas
flag, therefore, attributes/class/methods in this module should never be actually used.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
