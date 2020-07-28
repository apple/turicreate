#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools._deps import _HAS_TORCH

register_torch_op = None

if _HAS_TORCH:
    from .load import load
    from .torch_op_registry import register_torch_op
