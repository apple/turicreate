#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from ....._deps import _HAS_TF_2

if _HAS_TF_2:
    from .ops import *  # register all TF2 ops
    from coremltools.converters.mil.frontend.tensorflow.tf_op_registry import (
        register_tf_op,
    )
