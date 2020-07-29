#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil.mil import Builder as mb
from coremltools.converters.mil.testing_utils import (
    assert_model_is_valid,
    get_op_types_in_program,
    apply_pass_and_basic_check,
)
from coremltools.converters.mil.mil.passes.pass_registry import PASS_REGISTRY
import copy
import pytest
import itertools

import numpy as np


def test_reshape_elimination():
    @mb.program(input_specs=[mb.TensorSpec(shape=(2, 4))])
    def prog(x):
        r1 = mb.reshape(x=x, shape=[1, 8])
        r2 = mb.reshape(x=r1, shape=[1, 8])
        return mb.relu(x=r1)

    prev_prog, prev_block, block = apply_pass_and_basic_check(
        prog, "common::noop_elimination"
    )
    assert get_op_types_in_program(prev_prog) == ["reshape", "reshape", "relu"]
    assert get_op_types_in_program(prog) == ["reshape", "relu"]
    assert_model_is_valid(
        prog,
        {"x": (2, 4)},
        expected_output_shapes={block.outputs[0].name: (1, 8)},
    )


def test_oneway_split_elimination():
    @mb.program(input_specs=[mb.TensorSpec(shape=(2, 4))])
    def prog(x):
        r1 = mb.split(x=x, num_splits=1, axis=-1) 
        return mb.relu(x=r1)

    prev_prog, prev_block, block = apply_pass_and_basic_check(
        prog, "common::noop_elimination"
    )
    assert get_op_types_in_program(prev_prog) == ["split", "relu"]
    assert get_op_types_in_program(prog) == ["relu"]
    assert_model_is_valid(
        prog,
        {"x": (2, 4)},
        expected_output_shapes={block.outputs[0].name: (2, 4)},
    )


def test_full_split_elimination():
    @mb.program(input_specs=[mb.TensorSpec(shape=(2, 4))])
    def prog(x):
        r1 = mb.split(x=x, split_sizes=[4], axis=-1) 
        return mb.relu(x=r1)

    prev_prog, prev_block, block = apply_pass_and_basic_check(
        prog, "common::noop_elimination"
    )
    assert get_op_types_in_program(prev_prog) == ["split", "relu"]
    assert get_op_types_in_program(prog) == ["relu"]
    assert_model_is_valid(
        prog,
        {"x": (2, 4)},
        expected_output_shapes={block.outputs[0].name: (2, 4)},
    )


def test_slicebysize_full_elimination():
    @mb.program(input_specs=[mb.TensorSpec(shape=(2, 4))])
    def prog(x):
        r1 = mb.slice_by_size(x=x, begin=[0, 0], size=[2, 4]) 
        return mb.relu(x=r1)

    prev_prog, prev_block, block = apply_pass_and_basic_check(
        prog, "common::noop_elimination"
    )
    assert get_op_types_in_program(prev_prog) == ["slice_by_size", "relu"]
    assert get_op_types_in_program(prog) == ["relu"]
    assert_model_is_valid(
        prog,
        {"x": (2, 4)},
        expected_output_shapes={block.outputs[0].name: (2, 4)},
    )


def test_slicebysize_to_end_elimination():
    @mb.program(input_specs=[mb.TensorSpec(shape=(2, 4))])
    def prog(x):
        r1 = mb.slice_by_size(x=x, begin=[0, 0], size=[-1, -1]) 
        return mb.relu(x=r1)

    prev_prog, prev_block, block = apply_pass_and_basic_check(
        prog, "common::noop_elimination"
    )
    assert get_op_types_in_program(prev_prog) == ["slice_by_size", "relu"]
    assert get_op_types_in_program(prog) == ["relu"]
    assert_model_is_valid(
        prog,
        {"x": (2, 4)},
        expected_output_shapes={block.outputs[0].name: (2, 4)},
    )


def test_slicebyindex_full_elimination():
    @mb.program(input_specs=[mb.TensorSpec(shape=(2, 4))])
    def prog(x):
        r1 = mb.slice_by_index(x=x, begin=[0, 0], end=[2, 4]) 
        return mb.relu(x=r1)

    prev_prog, prev_block, block = apply_pass_and_basic_check(
        prog, "common::noop_elimination"
    )
    assert get_op_types_in_program(prev_prog) == ["slice_by_index", "relu"]
    assert get_op_types_in_program(prog) == ["relu"]
    assert_model_is_valid(
        prog,
        {"x": (2, 4)},
        expected_output_shapes={block.outputs[0].name: (2, 4)},
    )


@pytest.mark.parametrize("begin_mask, end_mask",
                         itertools.product(itertools.product([True, False],[True, False]),
                                           itertools.product([True, False],[True, False])))
def test_slicebyindex_mask_elimination(begin_mask, end_mask):
    @mb.program(input_specs=[mb.TensorSpec(shape=(4, 4))])
    def prog(x):
        begin = [1, 1]
        end = [1, 1]
        for i in range(2):
            if not begin_mask[i]:
                begin[i] = 0
            if not end_mask[i]:
                end[i] = 4
        r1 = mb.slice_by_index(x=x, begin=begin, end=end, begin_mask=begin_mask, end_mask=end_mask) 
        return mb.relu(x=r1)

    prev_prog, prev_block, block = apply_pass_and_basic_check(
        prog, "common::noop_elimination"
    )
    assert get_op_types_in_program(prev_prog) == ["slice_by_index", "relu"]
    assert get_op_types_in_program(prog) == ["relu"]
    assert_model_is_valid(
        prog,
        {"x": (4, 4)},
        expected_output_shapes={block.outputs[0].name: (4, 4)},
    )


