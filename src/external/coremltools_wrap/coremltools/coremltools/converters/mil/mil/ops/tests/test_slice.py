#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.testing_reqs import *

from .testing_utils import UNK_SYM, run_compare_builder

backends = testing_reqs.backends


class TestSliceByIndex:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array(list(range(24))).reshape((2, 3, 4)).astype(np.float32)
        begin_val = np.array([1, 1, 1], dtype=np.int32)
        end_val = np.array([2, 3, 3], dtype=np.int32)
        input_placeholders = {
            "x": mb.placeholder(shape=x_val.shape),
            "begin": mb.placeholder(shape=begin_val.shape, dtype=types.int32),
            "end": mb.placeholder(shape=end_val.shape, dtype=types.int32),
        }
        input_values = {"x": x_val, "begin": begin_val, "end": end_val}

        def build(x, begin, end):
            begin_c = mb.const(val=begin_val)
            end_c = mb.const(val=end_val)
            return [
                mb.slice_by_index(x=x, begin=begin, end=end),
                mb.slice_by_index(x=x, begin=begin_c, end=end_c)
            ]

        expected_output_types = [(UNK_SYM, UNK_SYM, UNK_SYM, types.fp32)] * 2
        expected_outputs = [np.array([[[17, 18], [21, 22]]], dtype=np.float32)] * 2
        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

    @ssa_fn
    def test_builder_eval(self):
        x_val = np.array(list(range(24))).reshape((2, 3, 4))
        v = [
            mb.slice_by_index(x=x_val, begin=[1, 1, 1], end=[2, 2, 2]),
            mb.slice_by_index(
                x=x_val, begin=[1, 1, 1], end=[2, 3, 4], stride=[1, 1, 2]
            ),
            mb.slice_by_index(x=x_val, begin=[-1, -3, -3], end=[-1, -1, -1]),
            mb.slice_by_index(x=x_val, begin=[0, 0, -3], end=[-1, -2, -2]),
            mb.slice_by_index(
                x=x_val, begin=[-1, -1, -1], end=[0, 1, -3], stride=[-2, -1, -3]
            ),
            mb.slice_by_index(
                x=x_val,
                begin=[1, 1, 1],
                end=[2, 3, 4],
                stride=[1, 1, 2],
                begin_mask=[True, False, True],
            ),
            mb.slice_by_index(
                x=x_val,
                begin=[1, 1, 1],
                end=[2, 3, 4],
                stride=[1, 1, 2],
                begin_mask=[True, False, True],
                end_mask=[True, True, False],
            ),
            mb.slice_by_index(
                x=x_val,
                begin=[1, 1, 1],
                end=[2, 3, 4],
                stride=[1, 1, 2],
                begin_mask=[False, False, True],
                end_mask=[True, False, False],
                squeeze_mask=[False, True, False],
            ),
            mb.slice_by_index(
                x=x_val,
                begin=[0, 0, 0],
                end=[0, 0, 0],
                stride=[1, 1, 1],
                begin_mask=[True, True, True],
                end_mask=[True, True, True],
            ),
            mb.slice_by_index(
                x=x_val,
                begin=[1, 1, 1],
                end=[2, 2, 0],
                stride=[1, 1, 1],
                squeeze_mask=[False, False, True],
            ),
            mb.slice_by_index(
                x=x_val,
                begin=[1, 0, 0],
                end=[2, 0, 0],
                stride=[1, 1, 1],
                begin_mask=[False, True, True],
                end_mask=[False, True, True],
            ),
            mb.slice_by_index(
                x=x_val,
                begin=[0, 0, 0],
                end=[0, 0, 0],
                stride=[1, 1, 1],
                begin_mask=[True, True, True],
                end_mask=[True, True, True],
            ),
            mb.slice_by_index(
                x=x_val,
                begin=[1, 0, 1],
                end=[2, 0, 2],
                stride=[1, 1, 1],
                begin_mask=[False, True, False],
                end_mask=[False, True, False],
            ),
            mb.slice_by_index(
                x=x_val,
                begin=[0, 0, 1],
                end=[0, 0, 0],
                stride=[1, 1, 1],
                begin_mask=[True, True, False],
                end_mask=[True, True, False],
                squeeze_mask=[False, False, True],
            ),
            mb.slice_by_index(
                x=x_val,
                begin=[0, 0, 0],
                end=[0, 0, 0],
                stride=[1, 1, 1],
                begin_mask=[False, False, True],
                end_mask=[False, False, True],
                squeeze_mask=[True, True, False],
            ),
            mb.slice_by_index(
                x=x_val,
                begin=[1, 0, 0],
                end=[2, 0, 0],
                stride=[1, 1, 1],
                begin_mask=[False, True, True],
                end_mask=[False, True, True],
            ),
            mb.slice_by_index(
                x=x_val,
                begin=[1, 1, 0],
                end=[2, 2, 0],
                stride=[1, 1, 1],
                begin_mask=[False, False, True],
                end_mask=[False, False, True],
            ),
            mb.slice_by_index(
                x=x_val,
                begin=[1, 0, 0],
                end=[0, 0, 0],
                stride=[1, 1, 1],
                begin_mask=[False, True, True],
                end_mask=[False, True, True],
                squeeze_mask=[True, False, False],
            ),
            mb.slice_by_index(
                x=x_val,
                begin=[0, 0, 0],
                end=[0, 0, 0],
                begin_mask=[True, True, True],
                end_mask=[True, True, True],
            ),
            mb.slice_by_index(
                x=x_val,
                begin=[0, 0, 0],
                end=[0, 0, 0],
                stride=[1, 1, -1],
                begin_mask=[True, True, True],
                end_mask=[True, True, True],
            ),
        ]
        ans = [
            x_val[1:2, 1:2, 1:2],
            x_val[1:2, 1:3, 1:4:2],
            x_val[-3:-1, -3:-1, -3:-1],
            x_val[0:-1, 0:-2, -3:-2],
            x_val[-1:0:-2, -1:1:-1, -1:-3:-3],
            x_val[:2, 1:3, :4:2],
            x_val[:, 1:, :4:2],
            x_val[1::1, 1, :3:2],
            x_val[:, :, :],
            x_val[1:2, 1:2, 1],
            x_val[1:2, ...],
            x_val[...],
            x_val[1:2, ..., 1:2],
            x_val[..., 1],
            x_val[0, 0, :],
            x_val[1:2],
            x_val[1:2, 1:2],
            x_val[1],
            x_val[:],
            x_val[..., ::-1],
        ]
        for idx in range(len(v)):
            assert is_close(ans[idx], v[idx].val)
