#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from scipy import special
import scipy
from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.testing_reqs import *

from .testing_utils import run_compare_builder

backends = testing_reqs.backends


class TestClampedReLU:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {
            "x": mb.placeholder(shape=t.shape),
        }
        input_values = {"x": t}

        def build(x):
            return mb.clamped_relu(x=x, alpha=2.0, beta=1.0)

        expected_output_types = (2, 3, types.fp32)
        expected_outputs = np.array([[-2, 1, -6], [1, -10, 1]], dtype=np.float32)

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
        x_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        v = mb.clamped_relu(x=x_val, alpha=2.0, beta=1.0)

        x = np.minimum(np.maximum(x_val, 0), 1.0)
        y = np.minimum(np.minimum(x_val, 0) * 2.0, 1.0)
        assert is_close(x + y, v.val)

    @pytest.mark.parametrize(
        "use_cpu_only, backend, dim, alpha, beta",
        itertools.product([True], backends, [2, 4, 8], [2.0, 3.0], [4.0, 5.0]),
    )
    def test_builder_to_backend_stress(self, use_cpu_only, backend, dim, alpha, beta):
        shape_x = np.array([dim, dim])
        x_val = np.random.rand(*shape_x)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [mb.clamped_relu(x=x, alpha=alpha, beta=beta)]

        x = np.minimum(np.maximum(x_val, 0), 1.0)
        y = np.minimum(np.minimum(x_val, 0) * 2.0, 1.0)

        expected_outputs = [x + y]
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


class TestELU:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {
            "x": mb.placeholder(shape=t.shape),
        }
        input_values = {"x": t}

        def build(x):
            return mb.elu(x=x, alpha=2.0)

        expected_output_types = (2, 3, types.fp32)
        expected_outputs = np.array(
            [[-1.2642411, 2.0, -1.9004259], [4.0, -1.9865241, 6.0]], dtype=np.float32
        )

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
        x_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        v = mb.elu(x=x_val, alpha=2.0)

        b = np.copy(x_val)
        b[b < 0] = 2.0 * (np.exp(b[b < 0]) - 1)

        assert is_close(b, v.val)


class TestGeLU:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {
            "x": mb.placeholder(shape=t.shape),
        }
        input_values = {"x": t}

        def build(x):
            return mb.gelu(x=x)

        expected_output_types = (2, 3, types.fp32)
        expected_outputs = np.array(
            [
                [-1.58691406e-01, 1.95410156e00, -4.04968858e-03],
                [3.99987316e00, -1.49011612e-06, 6.00000000e00],
            ],
            dtype=np.float32,
        )

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
            atol=1e-3,
            rtol=1e-3,
        )

    @ssa_fn
    def test_builder_eval(self):
        x_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)

        mode = "TANH_APPROXIMATION"
        v = mb.gelu(x=x_val, mode=mode)
        a = np.sqrt(2 / np.pi) * (x_val + 0.044715 * np.power(x_val, 3))
        out = 0.5 * x_val * (1 + np.tanh(a))
        assert is_close(out, v.val)

        mode = "SIGMOID_APPROXIMATION"
        v = mb.gelu(x=x_val, mode=mode)
        out = x_val * (1 / (1 + np.exp(-(1.702 * x_val))))
        assert is_close(out, v.val)

        v = mb.gelu(x=x_val)
        out = 0.5 * x_val * (1 + scipy.special.erf(x_val / np.sqrt(2)))
        assert is_close(out, v.val)

    @pytest.mark.parametrize(
        "use_cpu_only, backend, dim, mode",
        itertools.product(
            [True, False],
            backends,
            [2, 6],
            ["EXACT", "TANH_APPROXIMATION", "SIGMOID_APPROXIMATION"],
        ),
    )
    def test_builder_to_backend_stress(self, use_cpu_only, backend, dim, mode):
        shape = np.array([dim, dim])
        x_val = np.random.rand(*shape)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [mb.gelu(x=x, mode=mode)]

        if mode == "TANH_APPROXIMATION":
            a = np.sqrt(2 / np.pi) * (x_val + 0.044715 * np.power(x_val, 3))
            out = 0.5 * x_val * (1 + np.tanh(a))
        elif mode == "SIGMOID_APPROXIMATION":
            out = x_val * (1 / (1 + np.exp(-(1.702 * x_val))))
        else:
            out = 0.5 * x_val * (1 + scipy.special.erf(x_val / np.sqrt(2)))

        expected_outputs = [out]
        expected_output_types = [o.shape[:] + (types.fp32,) for o in expected_outputs]

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs=expected_outputs,
            use_cpu_only=use_cpu_only,
            backend=backend,
            atol=1e-3,
            rtol=1e-3,
        )


class TestLeakyReLU:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {
            "x": mb.placeholder(shape=t.shape),
        }
        input_values = {"x": t}

        def build(x):
            return mb.leaky_relu(x=x, alpha=2.0)

        expected_output_types = (2, 3, types.fp32)
        expected_outputs = np.array([[-2, 2, -6], [4, -10, 6]], dtype=np.float32)

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
        x_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        v = mb.leaky_relu(x=x_val, alpha=2.0)

        b = np.copy(x_val)
        b[b < 0] *= 2.0
        assert is_close(b, v.val)


class TestLinearActivation:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            return mb.linear_activation(x=x, alpha=2.0, beta=3.0)

        expected_output_types = (2, 3, types.fp32)
        expected_outputs = np.array([[1, 7, -3], [11, -7, 15]], dtype=np.float32)

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
        x_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        v = mb.linear_activation(x=x_val, alpha=2.0, beta=3.0)
        assert is_close(x_val * 2.0 + 3.0, v.val)

    @pytest.mark.parametrize(
        "use_cpu_only, backend, dim",
        itertools.product([True, False], backends, [2, 4, 8]),
    )
    def test_builder_to_backend_stress(self, use_cpu_only, backend, dim):
        shape = np.array([dim, dim])
        x_val = np.random.rand(*shape)
        alpha = np.random.uniform()
        beta = np.random.uniform()
        input_placeholders = {
            "x": mb.placeholder(shape=x_val.shape),
        }
        input_values = {"x": x_val}

        def build(x):
            return [mb.linear_activation(x=x, alpha=alpha, beta=beta)]

        expected_outputs = [x_val * alpha + beta]
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


# TODO (rdar://59954690): Broken when there is 1 channel
class TestPReLU:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[[-1, 3, 6]], [[-1, 2, -3]], [[4, -5, 6]]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            return mb.prelu(x=x, alpha=np.array([1, 2, 3], dtype=np.float32))

        expected_output_types = (3, 1, 3, types.fp32)
        expected_outputs = np.array(
            [[[-1, 3, 6]], [[-2, 2, -6]], [[4, -15, 6]]], dtype=np.float32
        )

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
        x_val = np.array([[[-1, 3, 6]], [[-1, 2, -3]], [[4, -5, 6]]], dtype=np.float32)
        alpha = np.array([1, 2, 3], dtype=np.float32)
        v = mb.prelu(x=x_val, alpha=alpha)

        alpha_br = alpha

        for i in range(1, len(x_val.shape)):
            alpha_br = np.expand_dims(alpha_br, i)

        x_pos = np.maximum(x_val, 0)
        b = np.minimum(x_val, 0)

        assert is_close(x_pos + b * alpha_br, v.val)

    @ssa_fn
    def test_builder_eval1(self):
        x_val = np.array([[[-1, 3, 6]], [[-1, 2, -3]], [[4, -5, 6]]], dtype=np.float32)
        with pytest.raises(ValueError, match=r".* dimension -3 .*"):
            v = mb.prelu(x=x_val, alpha=np.array([1, 2], dtype=np.float32))

    @ssa_fn
    def test_builder_eval2(self):
        x_val = np.array([[[-1, 3, 6]], [[-1, 2, -3]], [[4, -5, 6]]], dtype=np.float32)
        with pytest.raises(ValueError, match=r"alpha .* rank 1"):
            v = mb.prelu(x=x_val, alpha=np.array([[1, 2, 3]], dtype=np.float32))

    @ssa_fn
    def test_builder_eval3(self):
        x_val = np.array([[[-1, 3, 6]], [[-1, 2, -3]], [[4, -5, 6]]], dtype=np.float32)
        with pytest.raises(ValueError, match=r"x .* rank 3"):
            v = mb.prelu(x=[1], alpha=np.array([[1, 2, 3]], dtype=np.float32))

    # TODO (rdar://59672999): NN does not support PReLU with 1 input channel
    @pytest.mark.parametrize(
        "use_cpu_only, backend, dim, chan",
        itertools.product([True, False], backends, [1, 2, 4, 8], [2, 3, 4]),
    )
    def test_builder_to_backend_stress(self, use_cpu_only, backend, dim, chan):
        shape = np.array([chan, dim, dim])
        x_val = np.random.rand(*shape)
        alpha_val = np.random.rand(chan).astype(np.float32)

        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [mb.prelu(x=x, alpha=alpha_val)]

        alpha_br = np.copy(alpha_val)
        for i in range(1, len(x_val.shape)):
            alpha_br = np.expand_dims(alpha_br, i)
        x_pos = np.maximum(x_val, 0)
        b = np.minimum(x_val, 0)

        expected_outputs = [x_pos + b * alpha_br]
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


class TestReLU:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            return mb.relu(x=x)

        expected_output_types = (2, 3, types.fp32)
        expected_outputs = np.array([[0, 2, 0], [4, 0, 6]], dtype=np.float32)

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
        x_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        v = mb.relu(x=x_val)
        assert is_close(np.maximum(x_val, 0), v.val)


class TestReLU6:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[-1, 7, -3], [4, -5, 8]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            return mb.relu6(x=x)

        expected_output_types = (2, 3, types.fp32)
        expected_outputs = np.array([[0, 6, 0], [4, 0, 6]], dtype=np.float32)

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
        x_val = np.array([[-1, 7, -3], [4, -5, 8]], dtype=np.float32)
        v = mb.relu6(x=x_val)
        assert is_close(np.minimum(np.maximum(x_val, 0), 6), v.val)


class TestScaledTanh:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            return mb.scaled_tanh(x=x, alpha=2.0, beta=1.0)

        expected_output_types = (2, 3, types.fp32)
        expected_outputs = np.array(
            [[-1.5231884, 1.9280552, -1.9901096], [1.9986587, -1.9998184, 1.9999754]],
            dtype=np.float32,
        )

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
        x_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        v = mb.scaled_tanh(x=x_val, alpha=2.0, beta=1.0)
        assert is_close(2.0 * np.tanh(x_val * 1.0), v.val)

    @pytest.mark.parametrize(
        "use_cpu_only, backend, dim, alpha, beta",
        itertools.product([True], backends, [2, 4, 8], [2.0, 3.0], [4.0, 5.0]),
    )
    def test_builder_to_backend_stress(self, use_cpu_only, backend, dim, alpha, beta):
        shape_x = np.array([dim, dim])
        x_val = np.random.rand(*shape_x)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [mb.scaled_tanh(x=x, alpha=alpha, beta=beta)]

        expected_outputs = [alpha * np.tanh(x_val * beta)]
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


class TestSigmoid:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            return mb.sigmoid(x=x)

        expected_output_types = (2, 3, types.fp32)
        expected_outputs = np.array(
            [
                [0.2689414213699951, 0.8807970779778823, 0.04742587],
                [0.98201376, 0.00669285, 0.9975274],
            ],
            dtype=np.float32,
        )

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
        x_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        v = mb.sigmoid(x=x_val)
        assert is_close(1 / (1 + np.exp(-x_val)), v.val)


class TestSigmoidHard:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            return mb.sigmoid_hard(x=x, alpha=1.0, beta=2.0)

        expected_output_types = (2, 3, types.fp32)
        expected_outputs = np.array(
            [[1.0, 1.0, 0.0], [1.0, 0.0, 1.0]], dtype=np.float32
        )

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
        x_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        alpha = 1.0
        beta = 2.0
        v = mb.sigmoid_hard(x=x_val, alpha=alpha, beta=beta)
        assert is_close(np.minimum(np.maximum((alpha * x_val) + beta, 0), 1), v.val)

    @pytest.mark.parametrize(
        "use_cpu_only, backend, dim, alpha, beta",
        itertools.product([True], backends, [2, 4, 8], [2.0, 3.0], [4.0, 5.0]),
    )
    def test_builder_to_backend_stress(self, use_cpu_only, backend, dim, alpha, beta):
        shape_x = np.array([dim, dim])
        x_val = np.random.rand(*shape_x)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [mb.sigmoid_hard(x=x, alpha=alpha, beta=beta)]

        expected_outputs = [np.minimum(np.maximum((alpha * x_val) + beta, 0), 1)]
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


class TestSoftplus:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            return mb.softplus(x=x)

        expected_output_types = (2, 3, types.fp32)
        expected_outputs = np.array(
            [[0.31326166, 2.126928, 0.04858733], [4.01815, 0.00671535, 6.0024757]],
            dtype=np.float32,
        )

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
        x_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        v = mb.softplus(x=x_val)
        assert is_close(
            np.log(1 + np.exp(-np.abs(x_val))) + np.maximum(x_val, 0), v.val
        )


# TODO (rdar://59954690): NN Segfaults when converting from MIL ParametricSoftplus layer
# No torch test because there is no direct torch translation to this layer
class TestSoftplusParametric:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[[-1, 3, 6]], [[-1, 2, -3]], [[4, -5, 6]]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            return mb.softplus_parametric(
                x=x,
                alpha=np.array([1, 2, 3], dtype=np.float32),
                beta=np.array([4, 5, 6], dtype=np.float32),
            )

        expected_output_types = (3, 1, 3, types.fp32)
        expected_outputs = np.array(
            [
                [[1.8142700e-02, 1.2000000e01, 2.4000000e01]],
                [[1.3427734e-02, 2.0000000e01, 7.1525574e-07]],
                [[7.2000000e01, 0.0000000e00, 1.0800000e02]],
            ],
            dtype=np.float32,
        )

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
        x_val = np.array([[[-1, 3, 6]], [[-1, 2, -3]], [[4, -5, 6]]], dtype=np.float32)
        v = mb.softplus_parametric(
            x=x_val,
            alpha=np.array([1, 2, 3], dtype=np.float32),
            beta=np.array([4, 5, 6], dtype=np.float32),
        )

        alpha_br = np.array([1, 2, 3], dtype=np.float32)
        beta_br = np.array([4, 5, 6], dtype=np.float32)
        for i in range(1, len(x_val.shape)):
            alpha_br = np.expand_dims(alpha_br, i)
            beta_br = np.expand_dims(beta_br, i)
        out = alpha_br * np.log(np.exp(x_val * beta_br) + 1)

        assert is_close(out, v.val)

    @ssa_fn
    def test_builder_eval2(self):
        x_val = np.array([[[-1, 3, 6]], [[-1, 2, -3]], [[4, -5, 6]]], dtype=np.float32)
        with pytest.raises(ValueError, match=r".* dimension -3 .*"):
            v = mb.softplus_parametric(
                x=x_val,
                alpha=np.array([1, 2], dtype=np.float32),
                beta=np.array([4, 5, 6], dtype=np.float32),
            )

    @ssa_fn
    def test_builder_eval3(self):
        x_val = np.array([[[-1, 3, 6]], [[-1, 2, -3]], [[4, -5, 6]]], dtype=np.float32)
        with pytest.raises(ValueError, match=r"alpha .* rank 1"):
            v = mb.softplus_parametric(
                x=x_val,
                alpha=np.array([[1, 2, 3]], dtype=np.float32),
                beta=np.array([4, 5, 6], dtype=np.float32),
            )

    @ssa_fn
    def test_builder_eval4(self):
        x_val = np.array([[[-1, 3, 6]], [[-1, 2, -3]], [[4, -5, 6]]], dtype=np.float32)
        with pytest.raises(ValueError, match=r"x .* rank 3"):
            v = mb.softplus_parametric(
                x=[1],
                alpha=np.array([[1, 2, 3]], dtype=np.float32),
                beta=np.array([4, 5, 6], dtype=np.float32),
            )

    @ssa_fn
    def test_builder_eval5(self):
        x_val = np.array([[[-1, 3, 6]], [[-1, 2, -3]], [[4, -5, 6]]], dtype=np.float32)
        with pytest.raises(ValueError, match=r".* dimension -3 .*"):
            v = mb.softplus_parametric(
                x=x_val,
                alpha=np.array([1, 2, 3], dtype=np.float32),
                beta=np.array([5, 6], dtype=np.float32),
            )

    @ssa_fn
    def test_builder_eval6(self):
        x_val = np.array([[[-1, 3, 6]], [[-1, 2, -3]], [[4, -5, 6]]], dtype=np.float32)
        with pytest.raises(ValueError, match=r"beta .* rank 1"):
            v = mb.softplus_parametric(
                x=x_val,
                alpha=np.array([1, 2, 3], dtype=np.float32),
                beta=np.array([[4, 5, 6]], dtype=np.float32),
            )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, dim, chan",
        itertools.product([True, False], backends, [1, 2, 4, 8], [1, 2, 3]),
    )
    def test_builder_to_backend_stress(self, use_cpu_only, backend, dim, chan):
        shape = np.array([chan, dim, dim])
        x_val = np.random.rand(*shape)
        alpha_val = np.random.rand(chan).astype(np.float32)
        beta_val = np.random.rand(chan).astype(np.float32)

        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [mb.softplus_parametric(x=x, alpha=alpha_val, beta=beta_val)]

        alpha_br = np.copy(alpha_val)
        beta_br = np.copy(beta_val)
        for i in range(1, len(x_val.shape)):
            alpha_br = np.expand_dims(alpha_br, i)
            beta_br = np.expand_dims(beta_br, i)
        expected_outputs = [alpha_br * np.log(np.exp(x_val * beta_br) + 1)]
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


class TestSoftmax:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_buidler_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            return mb.softmax(x=x, axis=0)

        expected_output_types = (2, 3, types.fp32)
        expected_outputs = np.array(
            [
                [6.69285092e-03, 9.99088949e-01, 1.23394576e-04],
                [9.93307149e-01, 9.11051194e-04, 9.99876605e-01],
            ],
            dtype=np.float32,
        )
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
        x_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        v = mb.softmax(x=x_val, axis=0)
        assert is_close(scipy.special.softmax(x_val, axis=0), v.val)


class TestSoftsign:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            return mb.softsign(x=x)

        expected_output_types = (2, 3, types.fp32)
        expected_outputs = np.array(
            [[-0.5, 0.66666667, -0.75], [0.8, -0.83333333, 0.85714286]],
            dtype=np.float32,
        )

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
        x_val = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        v = mb.softsign(x=x_val)
        assert is_close(x_val / (1 + np.abs(x_val)), v.val)


class TestThresholdedReLU:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[-1, 2, -3], [4, -5, 6]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            return mb.thresholded_relu(x=x, alpha=2.0)

        expected_output_types = (2, 3, types.fp32)
        expected_outputs = np.array([[0, 2, 0], [4, 0, 6]], dtype=np.float32)

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
        x_val = np.array([[0, 2, 0], [4, 0, 6]], dtype=np.float32)
        v = mb.thresholded_relu(x=x_val, alpha=2.0)
        assert is_close(np.maximum(x_val - 2.0, 0), v.val)

    @pytest.mark.parametrize(
        "use_cpu_only, backend, dim, alpha",
        itertools.product([True], backends, [2, 4, 8], [2.0, 3.0]),
    )
    def test_builder_to_backend_stress(self, use_cpu_only, backend, dim, alpha):
        shape_x = np.array([dim, dim])
        x_val = np.random.rand(*shape_x)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [mb.thresholded_relu(x=x, alpha=alpha)]

        expected_outputs = [np.maximum(x_val - alpha, 0)]
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
