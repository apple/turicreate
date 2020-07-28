#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import numpy as np

from coremltools.converters.mil.mil import types
from coremltools.converters.mil.mil import Operation, precondition, VALUE
from coremltools.converters.mil.mil.input_type import *
from coremltools.converters.mil.mil.ops.registry import SSAOpRegistry

register_op = SSAOpRegistry.register_op
