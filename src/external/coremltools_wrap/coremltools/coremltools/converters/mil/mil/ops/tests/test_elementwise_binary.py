#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.testing_reqs import *

from .testing_utils import run_compare_builder

backends = testing_reqs.backends


class TestElementwiseBinary:
    # All in this test share the same backends
    @pytest.mark.parametrize(
        "use_cpu_only, backend, mode",
        itertools.product(
            [True, False],
            backends,
            [
                "add",
                "floor_div",
                "maximum",
                "minimum",
                "mod",
                "mul",
                "pow",
                "real_div",
                "sub",
            ],
        ),
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend, mode):
        if mode == "add":
            x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
            y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
            expected_outputs = np.array([[0, 4, 0], [8, 0, 12]], dtype=np.float32)

            build = lambda x, y: mb.add(x=x, y=y)
        elif mode == "floor_div":
            x = np.array([[10, 20, 30], [40, 50, 60]], dtype=np.float32)
            y = np.array([[11, 12, 13], [14, 15, 16]], dtype=np.float32)
            expected_outputs = np.array([[0, 1, 2], [2, 3, 3]], dtype=np.float32)

            build = lambda x, y: mb.floor_div(x=x, y=y)
        elif mode == "maximum":
            x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
            y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
            expected_outputs = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)

            build = lambda x, y: mb.maximum(x=x, y=y)
        elif mode == "minimum":
            x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
            y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
            expected_outputs = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)

            build = lambda x, y: mb.minimum(x=x, y=y)
        elif mode == "mod":
            x = np.array([[10, 20, 30], [40, 50, 60]], dtype=np.float32)
            y = np.array([[11, 12, 13], [14, 15, 16]], dtype=np.float32)
            expected_outputs = np.array([[10, 8, 4], [12, 5, 12]], dtype=np.float32)

            build = lambda x, y: mb.mod(x=x, y=y)
        elif mode == "mul":
            x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
            y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
            expected_outputs = np.array([[-1, 4, -9], [16, -25, 36]], dtype=np.float32)

            build = lambda x, y: mb.mul(x=x, y=y)
        elif mode == "pow":
            x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
            y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
            expected_outputs = np.array(
                [[1, 4, 0.037], [256, 0.00032, 46656]], dtype=np.float32
            )

            build = lambda x, y: mb.pow(x=x, y=y)
        elif mode == "real_div":
            x = np.array([[10, 20, 30], [40, 50, 60]], dtype=np.float32)
            y = np.array([[11, 12, 13], [14, 15, 16]], dtype=np.float32)
            expected_outputs = np.array(
                [[0.90909091, 1.66666667, 2.30769231], [2.85714286, 3.33333333, 3.75]],
                dtype=np.float32,
            )

            build = lambda x, y: mb.real_div(x=x, y=y)
        elif mode == "sub":
            x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
            y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
            expected_outputs = np.array([[2, 0, 6], [0, 10, 0]], dtype=np.float32)

            build = lambda x, y: mb.sub(x=x, y=y)

        expected_output_types = (2, 3, types.fp32)
        input_placeholders = {
            "x": mb.placeholder(shape=x.shape),
            "y": mb.placeholder(shape=y.shape),
        }
        input_values = {"x": x, "y": y}
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
    def test_builder_add(self):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        expected_outputs = np.array([[0, 4, 0], [8, 0, 12]], dtype=np.float32)
        v = mb.add(x=x, y=y)
        assert is_close(expected_outputs, v.val)

    @ssa_fn
    def test_builder_floor_div(self):
        x = np.array([[10, 20, 30], [40, 50, 60]], dtype=np.float32)
        y = np.array([[11, 12, 13], [14, 15, 16]], dtype=np.float32)
        expected_outputs = np.array([[0, 1, 2], [2, 3, 3]], dtype=np.float32)
        v = mb.floor_div(x=x, y=y)
        assert is_close(expected_outputs, v.val)

    @ssa_fn
    def test_builder_maximum(self):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        expected_outputs = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        v = mb.maximum(x=x, y=y)
        assert is_close(expected_outputs, v.val)

    @ssa_fn
    def test_builder_minimum(self):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        expected_outputs = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        v = mb.minimum(x=x, y=y)
        assert is_close(expected_outputs, v.val)

    @ssa_fn
    def test_builder_mod(self):
        x = np.array([[10, 20, 30], [40, 50, 60]], dtype=np.float32)
        y = np.array([[11, 12, 13], [14, 15, 16]], dtype=np.float32)
        expected_outputs = np.array([[10, 8, 4], [12, 5, 12]], dtype=np.float32)
        v = mb.mod(x=x, y=y)
        assert is_close(expected_outputs, v.val)

    @ssa_fn
    def test_builder_mul(self):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        expected_outputs = np.array([[-1, 4, -9], [16, -25, 36]], dtype=np.float32)
        v = mb.mul(x=x, y=y)
        assert is_close(expected_outputs, v.val)

    @ssa_fn
    def test_builder_pow(self):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        expected_outputs = np.array(
            [[1, 4, 0.037], [256, 0.00032, 46656]], dtype=np.float32
        )
        v = mb.pow(x=x, y=y)
        assert is_close(expected_outputs, v.val)

    @ssa_fn
    def test_builder_real_div(self):
        x = np.array([[10, 20, 30], [40, 50, 60]], dtype=np.float32)
        y = np.array([[11, 12, 13], [14, 15, 16]], dtype=np.float32)
        expected_outputs = np.array(
            [[0.90909091, 1.66666667, 2.30769231], [2.85714286, 3.33333333, 3.75]],
            dtype=np.float32,
        )
        v = mb.real_div(x=x, y=y)
        assert is_close(expected_outputs, v.val)

    @ssa_fn
    def test_builder_real_div_both_ints(self):
        x = np.array([5], dtype=np.int32)
        y = np.array([2], dtype=np.int32)
        expected_outputs = np.array([2.5], dtype=np.float32)
        v = mb.real_div(x=x, y=y)
        assert is_close(expected_outputs, v.val)
        # real_div should produce float values regardless of input type
        assert isinstance(v.val[0], (float, np.float32))
        # make sure the dtype is float
        assert types.is_float(v.dtype)
        # make sure the symbolic type matches the value type
        assert v._sym_type.get_primitive() == v._sym_val.get_primitive()

    @ssa_fn
    def test_builder_sub(self):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        expected_outputs = np.array([[2, 0, 6], [0, 10, 0]], dtype=np.float32)
        v = mb.sub(x=x, y=y)
        assert is_close(expected_outputs, v.val)


class TestEqual:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {
            "x": mb.placeholder(shape=x.shape),
            "y": mb.placeholder(shape=y.shape),
        }
        input_values = {"x": x, "y": y}

        def build(x, y):
            return mb.equal(x=x, y=y), mb.equal(x=-3, y=y)

        expected_output_types = [
            (2, 3, types.bool),
            (2, 3, types.bool),
        ]
        expected_outputs = [
            np.array([[0, 1, 0], [1, 0, 1]], dtype=np.bool),
            np.array([[0, 0, 1], [0, 0, 0]], dtype=np.bool),
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

    @ssa_fn
    def test_builder_eval(self):
        x_val = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        expected_outputs = np.array([[0, 1, 0], [1, 0, 1]], dtype=np.bool)
        v = mb.equal(x=x_val, y=y_val)
        assert is_close(expected_outputs, v.val)


class TestGreater:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {
            "x": mb.placeholder(shape=x.shape),
            "y": mb.placeholder(shape=y.shape),
        }
        input_values = {"x": x, "y": y}

        def build(x, y):
            return mb.greater(x=x, y=y), mb.greater(x=x, y=3.5)

        expected_output_types = [
            (2, 3, types.bool),
            (2, 3, types.bool),
        ]
        expected_outputs = [
            np.array([[1, 0, 1], [0, 1, 0]], dtype=np.bool),
            np.array([[0, 0, 0], [1, 1, 1]], dtype=np.bool),
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

    @ssa_fn
    def test_builder_eval(self):
        x_val = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        expected_outputs = np.array([[1, 0, 1], [0, 1, 0]], dtype=np.bool)
        v = mb.greater(x=x_val, y=y_val)
        assert is_close(expected_outputs, v.val)


class TestGreaterEqual:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {
            "x": mb.placeholder(shape=x.shape),
            "y": mb.placeholder(shape=y.shape),
        }
        input_values = {"x": x, "y": y}

        def build(x, y):
            return mb.greater_equal(x=x, y=y), mb.greater_equal(x=x, y=3.5)

        expected_output_types = [
            (2, 3, types.bool),
            (2, 3, types.bool),
        ]
        expected_outputs = [
            np.array([[1, 1, 1], [1, 1, 1]], dtype=np.bool),
            np.array([[0, 0, 0], [1, 1, 1]], dtype=np.bool),
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

    @ssa_fn
    def test_builder_eval(self):
        x_val = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        expected_outputs = np.array([[1, 1, 1], [1, 1, 1]], dtype=np.bool)
        v = mb.greater_equal(x=x_val, y=y_val)
        assert is_close(expected_outputs, v.val)


class TestLess:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {
            "x": mb.placeholder(shape=x.shape),
            "y": mb.placeholder(shape=y.shape),
        }
        input_values = {"x": x, "y": y}

        def build(x, y):
            return mb.less(x=x, y=y)

        expected_output_types = (2, 3, types.bool)
        expected_outputs = np.array([[0, 0, 0], [0, 0, 0]], dtype=np.bool)

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
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke2(self, use_cpu_only, backend):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=x.shape)}
        input_values = {"x": x}

        def build(x):
            # y is const
            y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
            return mb.less(x=x, y=y)

        expected_output_types = (2, 3, types.bool)
        expected_outputs = np.array([[0, 0, 0], [0, 0, 0]], dtype=np.bool)

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
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_broadcast(self, use_cpu_only, backend):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=x.shape)}
        input_values = {"x": x}

        def build(x):
            # y is const
            return mb.less(x=x, y=3.5)

        expected_output_types = (2, 3, types.bool)
        expected_outputs = np.array([[1, 1, 1], [0, 0, 0]], dtype=np.bool)

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
        x_val = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        expected_outputs = np.array([[0, 0, 0], [0, 0, 0]], dtype=np.bool)
        v = mb.less(x=x_val, y=y_val)
        assert is_close(expected_outputs, v.val)


class TestLessEqual:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {
            "x": mb.placeholder(shape=x.shape),
            "y": mb.placeholder(shape=y.shape),
        }
        input_values = {"x": x, "y": y}

        def build(x, y):
            return mb.less_equal(x=x, y=y)

        expected_output_types = (2, 3, types.bool)
        expected_outputs = np.array([[0, 1, 0], [1, 0, 1]], dtype=np.bool)

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
        x_val = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        expected_outputs = np.array([[0, 1, 0], [1, 0, 1]], dtype=np.bool)
        v = mb.less_equal(x=x_val, y=y_val)
        assert is_close(expected_outputs, v.val)


class TestNotEqual:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {
            "x": mb.placeholder(shape=x.shape),
            "y": mb.placeholder(shape=y.shape),
        }
        input_values = {"x": x, "y": y}

        def build(x, y):
            return mb.not_equal(x=x, y=y)

        expected_output_types = (2, 3, types.bool)
        expected_outputs = np.array([[1, 0, 1], [0, 1, 0]], dtype=np.bool)

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
        x_val = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        y_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        expected_outputs = np.array([[1, 0, 1], [0, 1, 0]], dtype=np.bool)
        v = mb.not_equal(x=x_val, y=y_val)
        assert is_close(expected_outputs, v.val)
