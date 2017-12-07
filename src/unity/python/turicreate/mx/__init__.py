# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import absolute_import as _
from __future__ import print_function as _
from __future__ import division as _
_mxnet_import_success = False
try:
    import mxnet as _mx
    _mxnet_import_success = True
except:
    pass

if _mxnet_import_success:
    from ._mx_sframe_iter import SFrameIter, SFrameImageIter
