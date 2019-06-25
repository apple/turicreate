# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
Turi Create offers several data structures for data analysis.  Concise
descriptions of the data structures and their methods are contained in the API
documentation, along with a small number of simple examples.
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

__all__ = ['sframe', 'sarray', 'sgraph', 'sketch', 'image']

from . import image
from . import sframe
from . import sarray
from . import sgraph
from . import sketch
