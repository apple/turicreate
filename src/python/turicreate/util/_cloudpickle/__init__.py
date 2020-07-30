# -*- coding: utf-8 -*-
# Copyright © 2020 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from sys import version_info

if version_info[0] == 2:   # Python 2.7
    from ._cloudpickle_py27 import dumps, CloudPickler
else:   # Python 3.X
    from ._cloudpickle_fast import dumps, CloudPickler
