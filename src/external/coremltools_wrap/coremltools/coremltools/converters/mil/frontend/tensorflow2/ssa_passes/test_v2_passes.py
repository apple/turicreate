#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil.mil import Builder as mb
from coremltools.converters.mil.testing_utils import (
    assert_op_count_match,
    assert_model_is_valid,
    assert_same_output_names,
)
from coremltools.converters.mil.mil.passes.pass_registry import PASS_REGISTRY
from coremltools.converters.mil.mil import types
import copy

import numpy as np

np.random.seed(1984)
validate_model = True


def test_remove_vacuous_cond():
    @mb.program(
        input_specs=[
            mb.TensorSpec(shape=(1,), dtype=types.bool),
            mb.TensorSpec(shape=(2, 3)),
        ]
    )
    def prog(a, b):
        def then_branch():
            return mb.identity(x=b)

        def else_branch():
            return mb.identity(x=b)

        pred = mb.squeeze(x=a)
        return mb.cond(pred=pred, _true_fn=then_branch, _false_fn=else_branch)

    cond_op = prog.find_ops(op_type="cond", exactly_one=True)[0]
    original_cond_op_name = cond_op.name
    assert len(cond_op.blocks[0].operations) == 1
    assert len(cond_op.blocks[1].operations) == 1
    assert cond_op.blocks[0].operations[0].op_type == "identity"
    assert cond_op.blocks[1].operations[0].op_type == "identity"

    prev_prog = copy.deepcopy(prog)
    PASS_REGISTRY["tensorflow2::remove_vacuous_cond"](prog)
    assert_same_output_names(prev_prog, prog)

    cond_op = prog.find_ops(op_type="cond")
    assert len(cond_op) == 0
    identity_op = prog.find_ops(prefix=original_cond_op_name, exactly_one=True)[0]
    assert identity_op.op_type == "identity"

    if validate_model:
        assert_model_is_valid(prog, {"a": (1,), "b": (2, 3)})
