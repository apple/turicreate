#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.testing_reqs import *

from .testing_utils import run_compare_builder

backends = testing_reqs.backends


class TestLinear:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array([[-4.7182, 11.94], [-3.3939, 9.2166]], dtype=np.float32)
        weight_val = np.array([[1.2313, -0.095], [-1.4075, -0.8816]], dtype=np.float32)
        bias_val = np.array([1.0, 2.0], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [mb.linear(x=x, weight=weight_val, bias=bias_val)]

        expected_output_types = [(2, 2, types.fp32)]
        expected_outputs = [
            np.array(
                [[-5.9438195, -1.8854373], [-4.054486, -1.3484411]], dtype=np.float32
            )
        ]

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs,
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @ssa_fn
    def test_builder_eval(self):
        x_val = random_gen(shape=(2, 2), rand_min=-37, rand_max=64)
        weight_val = random_gen(shape=(2, 2), rand_min=-91, rand_max=84)
        bias_val = random_gen(shape=(2,), rand_min=0.0, rand_max=9.0)
        v = mb.linear(x=x_val, weight=weight_val, bias=bias_val)
        assert is_close(np.matmul(x_val, weight_val.T) + bias_val, v.val)

    @pytest.mark.parametrize(
        "use_cpu_only, backend, dim",
        itertools.product([True, False], backends, [2, 4, 8]),
    )
    def test_builder_to_backend_stress(self, use_cpu_only, backend, dim):
        shape = np.array([dim, dim])
        x_val = np.random.rand(*shape)
        weight_val = np.random.rand(*shape).astype(np.float32)
        bias_val = np.random.rand(dim).astype(np.float32)
        input_placeholders = {
            "x": mb.placeholder(shape=x_val.shape),
        }
        input_values = {"x": x_val}

        def build(x):
            return [mb.linear(x=x, weight=weight_val, bias=bias_val)]

        expected_outputs = [np.matmul(x_val, np.transpose(weight_val)) + bias_val]

        expected_output_types = [o.shape[:] + (types.fp32,) for o in expected_outputs]

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs=expected_outputs,
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestMatMul:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array([[-4.0, 13.0], [-3.0, 9.0]], dtype=np.float32)
        y_val = np.array([[1.0, -7.0], [-1.0, -8.0]], dtype=np.float32)
        input_placeholders = {
            "x": mb.placeholder(shape=x_val.shape),
            "y": mb.placeholder(shape=y_val.shape),
        }
        input_values = {"x": x_val, "y": y_val}

        def build(x, y):
            return [
                mb.matmul(x=x_val, y=y),
                mb.matmul(x=x, y=y_val),
                mb.matmul(x=x, y=y),
                mb.matmul(x=x, y=y, transpose_x=True, transpose_y=True),
                mb.matmul(x=x_val, y=y, transpose_x=True, transpose_y=True),
                mb.matmul(x=x, y=y_val, transpose_x=True, transpose_y=True),
                mb.matmul(x=x, y=y_val, transpose_x=True, transpose_y=False),
                mb.matmul(x=x, y=y_val, transpose_x=False, transpose_y=True),
            ]

        expected_output_types = [
            (2, 2, types.fp32),
            (2, 2, types.fp32),
            (2, 2, types.fp32),
            (2, 2, types.fp32),
            (2, 2, types.fp32),
            (2, 2, types.fp32),
            (2, 2, types.fp32),
            (2, 2, types.fp32),
        ]
        expected_outputs = [
            np.array([[-17.0, -76.0], [-12.0, -51.0]], dtype=np.float32),
            np.array([[-17.0, -76.0], [-12.0, -51.0]], dtype=np.float32),
            np.array([[-17.0, -76.0], [-12.0, -51.0]], dtype=np.float32),
            np.array([[17.0, 28.0], [-50.0, -85.0]], dtype=np.float32),
            np.array([[17.0, 28.0], [-50.0, -85.0]], dtype=np.float32),
            np.array([[17.0, 28.0], [-50.0, -85.0]], dtype=np.float32),
            np.array([[-1.0, 52.0], [4.0, -163.0]], dtype=np.float32),
            np.array([[-95.0, -100.0], [-66.0, -69.0]], dtype=np.float32),
        ]

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs,
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @ssa_fn
    def test_builder_eval(self):
        x_val = random_gen(shape=(2, 2, 4), rand_min=-37, rand_max=64)
        y_val = random_gen(shape=(2, 4, 2), rand_min=-91, rand_max=84)
        v = mb.matmul(x=x_val, y=y_val)
        assert is_close(np.matmul(x_val, y_val), v.val)

    @pytest.mark.parametrize(
        "use_cpu_only, backend, shapes",
        itertools.product(
            [True, False],
            backends,
            [
                ((3, 2, 3, 4), (3, 2, 4, 5)),
                ((1, 1, 1, 3, 4), (1, 3, 2, 4, 5)),
                ((1, 3, 1, 2, 3), (1, 4, 3, 2)),
                ((1, 3, 4), (3, 2, 4, 6)),
                ((7, 4), (3, 9, 5, 4, 3)),
            ],
        ),
    )
    def test_builder_to_backend_stress(self, use_cpu_only, backend, shapes):
        shape_x, shape_y = shapes
        x_val = np.random.rand(*shape_x)
        y_val = np.random.rand(*shape_y)
        input_placeholders = {
            "x": mb.placeholder(shape=x_val.shape),
            "y": mb.placeholder(shape=y_val.shape),
        }
        input_values = {"x": x_val, "y": y_val}

        def build(x, y):
            return [mb.matmul(x=x, y=y, transpose_x=False, transpose_y=False)]

        expected_outputs = [np.matmul(x_val, y_val)]
        expected_output_types = [o.shape[:] + (types.fp32,) for o in expected_outputs]

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs,
            use_cpu_only=use_cpu_only,
            backend=backend,
        )
