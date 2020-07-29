#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import scipy
from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.mil import get_new_symbol
from coremltools.converters.mil.testing_reqs import *

from .testing_utils import run_compare_builder

backends = testing_reqs.backends


class TestReduction:
    # All ops in this test share the same backends
    @pytest.mark.parametrize(
        "use_cpu_only, backend, mode",
        itertools.product(
            [True, False],
            backends,
            [
                "argmax",
                "argmin",
                "l1_norm",
                "l2_norm",
                "log_sum",
                "log_sum_exp",
                "max",
                "mean",
                "min",
                "prod",
                "sum",
                "sum_square",
            ],
        ),
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend, mode):
        val = np.array([[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=val.shape)}
        input_values = {"x": val}

        if mode in {"argmax", "argmin"}:
            expected_output_types = (2, types.int32)
        else:
            expected_output_types = (2, types.fp32)

        if mode == "argmax":
            build = lambda x: mb.reduce_argmax(x=x, axis=1, keep_dims=False)
            expected_outputs = np.array([2, 2], dtype=np.int32)
        elif mode == "argmin":
            build = lambda x: mb.reduce_argmin(x=x, axis=1, keep_dims=False)
            expected_outputs = np.array([0, 0], dtype=np.int32)
        elif mode == "l1_norm":
            build = lambda x: mb.reduce_l1_norm(x=x, axes=[1], keep_dims=False)
            expected_outputs = np.array([6.0, 15.0], dtype=np.float32)
        elif mode == "l2_norm":
            build = lambda x: mb.reduce_l2_norm(x=x, axes=[1], keep_dims=False)
            expected_outputs = np.array([3.74165738, 8.77496438], dtype=np.float32)
        elif mode == "log_sum":
            build = lambda x: mb.reduce_log_sum(x=x, axes=[1], keep_dims=False)
            expected_outputs = np.array([1.7917595, 2.70805025], dtype=np.float32)
        elif mode == "log_sum_exp":
            build = lambda x: mb.reduce_log_sum_exp(x=x, axes=[1], keep_dims=False)
            expected_outputs = np.array([3.40760589, 6.40760612], dtype=np.float32)
        elif mode == "max":
            build = lambda x: mb.reduce_max(x=x, axes=[1], keep_dims=False)
            expected_outputs = np.array([3.0, 6.0], dtype=np.float32)
        elif mode == "mean":
            build = lambda x: mb.reduce_mean(x=x, axes=[1], keep_dims=False)
            expected_outputs = np.array([2.0, 5.0], dtype=np.float32)
        elif mode == "min":
            build = lambda x: mb.reduce_min(x=x, axes=[1], keep_dims=False)
            expected_outputs = np.array([1.0, 4.0], dtype=np.float32)
        elif mode == "prod":
            build = lambda x: mb.reduce_prod(x=x, axes=[1], keep_dims=False)
            expected_outputs = np.array([6.0, 120.0], dtype=np.float32)
        elif mode == "sum":
            build = lambda x: mb.reduce_sum(x=x, axes=[1], keep_dims=False)
            expected_outputs = np.array([6.0, 15.0], dtype=np.float32)
        elif mode == "sum_square":
            build = lambda x: mb.reduce_sum_square(x=x, axes=[1], keep_dims=False)
            expected_outputs = np.array([14.0, 77.0], dtype=np.float32)
        else:
            raise NotImplementedError()

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

    @pytest.mark.parametrize(
        "use_cpu_only, backend, mode",
        itertools.product([True, False], ["nn_proto"], ["max", "mean"]),
    )
    def test_builder_to_backend_global_pool_2d(self, use_cpu_only, backend, mode):
        # test lowering to spatial reduction to global_pool path
        val = np.array([[[[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]]]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=val.shape)}
        input_values = {"x": val}

        expected_output_types = (1, 1, 1, 1, types.fp32)

        if mode == "max":
            build = lambda x: mb.reduce_max(x=x, axes=[2, -1], keep_dims=True)
            expected_outputs = np.array([[[[6.0]]]], dtype=np.float32)
        elif mode == "mean":
            build = lambda x: mb.reduce_mean(x=x, axes=[3, -2], keep_dims=True)
            expected_outputs = np.array([[[[3.5]]]], dtype=np.float32)

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs,
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, mode",
        itertools.product([True, False], ["nn_proto"], ["max", "mean"]),
    )
    def test_builder_to_backend_global_pool_3d(self, use_cpu_only, backend, mode):
        # test lowering to spatial reduction to global_pool path
        val = np.array([[[[[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]]]]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=val.shape)}
        input_values = {"x": val}

        expected_output_types = (1, 1, 1, 1, 1, types.fp32)

        if mode == "max":
            build = lambda x: mb.reduce_max(x=x, axes=[2, -1, 3], keep_dims=True)
            expected_outputs = np.array([[[[[6.0]]]]], dtype=np.float32)
        elif mode == "mean":
            build = lambda x: mb.reduce_mean(x=x, axes=[-3, 3, 4], keep_dims=True)
            expected_outputs = np.array([[[[[3.5]]]]], dtype=np.float32)

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs,
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        ["axis", "keep_dims"], itertools.product([1, -3], [True, False])
    )
    def test_builder_eval(self, axis, keep_dims):
        x_val = random_gen(shape=(1, 3, 4, 4), rand_min=-100.0, rand_max=100.0)

        @ssa_fn
        def test_reduce_argmax():
            res = mb.reduce_argmax(x=x_val, axis=axis, keep_dims=keep_dims).val
            ref = np.argmax(x_val, axis=axis)
            assert is_close(ref, res)

        @ssa_fn
        def test_reduce_argmin():
            res = mb.reduce_argmin(x=x_val, axis=axis, keep_dims=keep_dims).val
            ref = np.argmin(x_val, axis=axis)
            assert is_close(ref, res)

        @ssa_fn
        def test_reduce_l1_norm():
            res = mb.reduce_l1_norm(x=x_val, axes=[axis], keep_dims=keep_dims).val
            ref = np.sum(np.abs(x_val), axis=axis, keepdims=keep_dims)
            assert is_close(ref, res)

        @ssa_fn
        def test_reduce_l2_norm():
            res = mb.reduce_l2_norm(x=x_val, axes=[axis], keep_dims=keep_dims).val
            ref = np.sqrt(np.sum(np.square(x_val), axis=axis, keepdims=keep_dims))
            assert is_close(ref, res)

        @ssa_fn
        def test_reduce_log_sum():
            x_val = random_gen(shape=(1, 3, 4, 4), rand_min=0.0, rand_max=100.0)
            res = mb.reduce_log_sum(x=x_val, axes=[axis], keep_dims=keep_dims).val
            ref = np.log(np.sum(x_val, axis=axis, keepdims=keep_dims))
            assert is_close(ref, res)

        @ssa_fn
        def test_reduce_log_sum_exp():
            res = mb.reduce_log_sum_exp(x=x_val, axes=[axis], keep_dims=keep_dims).val
            ref = scipy.special.logsumexp(x_val, axis=axis, keepdims=keep_dims)
            assert is_close(ref, res)

        @ssa_fn
        def test_reduce_max():
            res = mb.reduce_max(x=x_val, axes=[axis], keep_dims=keep_dims).val
            ref = np.max(x_val, axis=axis, keepdims=keep_dims)
            assert is_close(ref, res)

        @ssa_fn
        def test_reduce_mean():
            res = mb.reduce_mean(x=x_val, axes=[axis], keep_dims=keep_dims).val
            ref = np.mean(x_val, axis=axis, keepdims=keep_dims)
            assert is_close(ref, res)

        @ssa_fn
        def test_reduce_min():
            res = mb.reduce_min(x=x_val, axes=[axis], keep_dims=keep_dims).val
            ref = np.min(x_val, axis=axis, keepdims=keep_dims)
            assert is_close(ref, res)

        @ssa_fn
        def test_reduce_prod():
            res = mb.reduce_prod(x=x_val, axes=[axis], keep_dims=keep_dims).val
            ref = np.prod(x_val, axis=axis, keepdims=keep_dims)
            assert is_close(ref, res)

        @ssa_fn
        def test_reduce_sum():
            res = mb.reduce_sum(x=x_val, axes=[axis], keep_dims=keep_dims).val
            ref = np.sum(x_val, axis=axis, keepdims=keep_dims)
            assert is_close(ref, res)

        @ssa_fn
        def test_reduce_sum_square():
            res = mb.reduce_sum_square(x=x_val, axes=[axis], keep_dims=keep_dims).val
            ref = np.sum(np.square(x_val), axis=axis, keepdims=keep_dims)
            assert is_close(ref, res)

        test_reduce_argmax()
        test_reduce_argmin()
        test_reduce_l1_norm()
        test_reduce_l2_norm()
        test_reduce_log_sum()
        test_reduce_log_sum_exp()
        test_reduce_max()
        test_reduce_mean()
        test_reduce_min()
        test_reduce_prod()
        test_reduce_sum()
        test_reduce_sum_square()

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_symbolic(self, use_cpu_only, backend):
        # TODO: variadic (rdar://59559656)

        s0 = get_new_symbol()

        val = np.array([[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=(s0, 3))}
        input_values = {"x": val}

        def build(x):
            return [
                mb.reduce_argmax(x=x, axis=1, keep_dims=True),
                mb.reduce_argmin(x=x, axis=0, keep_dims=True),
            ]

        expected_output_types = [(s0, 1, types.int32), (1, 3, types.int32)]
        expected_outputs = [
            np.array([[2], [2]], dtype=np.int32),
            np.array([[0], [0], [0]], dtype=np.int32),
        ]

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
