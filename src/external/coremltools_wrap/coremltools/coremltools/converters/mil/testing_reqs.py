#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import os
import itertools
import numpy as np
import pytest

from coremltools.converters.mil.mil import Builder as mb
from coremltools.converters.mil.mil.ops.registry import SSAOpRegistry

from coremltools.converters.mil.mil import types
from coremltools._deps import (
    _HAS_TF_1,
    _HAS_TF_2,
    _HAS_TORCH,
    MSG_TF1_NOT_FOUND,
    MSG_TF2_NOT_FOUND,
)
from .testing_utils import ssa_fn, is_close, random_gen, converter, _converter

backends = _converter.ConverterRegistry.backends.keys()

np.random.seed(1984)

if _HAS_TF_1 or _HAS_TF_2:
    import tensorflow as tf

    tf.compat.v1.set_random_seed(1234) if _HAS_TF_1 else tf.random.set_seed(1234)

if _HAS_TORCH:
    import torch
