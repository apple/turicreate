SPACES = "  "

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from .block import curr_block, Block, Function
from .input_type import *
from .operation import *
from .program import *
from .var import *

from .builder import Builder
from .ops.defs._op_reqs import register_op
