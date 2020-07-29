#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.testing_reqs import *
from coremltools.converters.mil.testing_utils import get_core_ml_prediction
from coremltools._deps import _IS_MACOS
from .testing_utils import UNK_SYM, run_compare_builder

backends = testing_reqs.backends


class TestRandomBernoulli:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):

        x_val = np.array([0.0], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [
                mb.add(x=x, y=x),
                mb.random_bernoulli(shape=np.array([2, 1, 3], np.int32), prob=1.0),
                mb.random_bernoulli(shape=np.array([3, 1, 2], np.int32), prob=0.0),
            ]

        expected_outputs = [
            np.array(np.zeros(shape=(1,)), np.float32),
            np.array(np.ones(shape=(2, 1, 3)), np.float32),
            np.array(np.zeros(shape=(3, 1, 2)), np.float32),
        ]

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

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, prob, dynamic",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(1, 6)],
            [1.0, 0.0],
            [True, False],
        ),
    )
    def test_builder_to_backend_stress(
        self, use_cpu_only, backend, rank, prob, dynamic
    ):
        shape = np.random.randint(low=1, high=4, size=rank).astype(np.int32)
        x_val = np.array([0.0], dtype=np.float32)
        if dynamic:
            input_placeholders = {
                "x": mb.placeholder(shape=x_val.shape),
                "dyn_shape": mb.placeholder(shape=shape.shape, dtype=types.int32),
            }
            input_values = {"x": x_val, "dyn_shape": shape}
        else:
            input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
            input_values = {"x": x_val}

        def build(x):
            return [mb.add(x=x, y=x), mb.random_bernoulli(shape=shape, prob=prob)]

        def build_dyn(x, dyn_shape):
            return [mb.add(x=x, y=x), mb.random_bernoulli(shape=dyn_shape, prob=prob)]

        expected_outputs = [
            np.array(np.zeros(shape=(1,)), np.float32),
            np.random.binomial(1, prob, shape),
        ]

        if dynamic:
            expected_output_types = [
                tuple([UNK_SYM for _ in o.shape]) + (types.fp32,)
                for o in expected_outputs
            ]
        else:
            expected_output_types = [
                o.shape[:] + (types.fp32,) for o in expected_outputs
            ]

        builder = build_dyn if dynamic else build

        run_compare_builder(
            builder,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs=expected_outputs,
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestRandomCategorical:
    def softmax(self, data):
        e_data = np.exp(data - np.max(data))
        return e_data / e_data.sum()

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array([1], dtype=np.int32)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [
                mb.random_categorical(x=x, seed=1),
                mb.random_categorical(x=x, seed=1, size=4),
            ]

        expected_outputs = [
            np.array(np.zeros(shape=(1,)), dtype=np.float32),
            np.array(np.zeros(shape=(4,)), dtype=np.float32),
        ]

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

    @pytest.mark.parametrize(
        "use_cpu_only, backend, n_sample, n_class",
        itertools.product([True, False], backends, [50000], [2, 10, 20]),
    )
    def test_builder_to_backend_stress(self, use_cpu_only, backend, n_sample, n_class):
        output_name = "random_categorical"
        logits = np.random.rand(2, n_class)
        probs = [self.softmax(logits[0]), self.softmax(logits[1])]

        # Test logits input
        input_placeholders = {"x": mb.placeholder(shape=(2, n_class))}
        input_values = {"x": logits}

        def build(x):
            return [
                mb.random_categorical(
                    x=x, size=n_sample, mode="logits", name=output_name
                )
            ]

        if _IS_MACOS:
            prediction = get_core_ml_prediction(
                build, input_placeholders, input_values, backend=backend
            )

            ref0 = np.random.multinomial(n_sample, probs[0])
            ref1 = np.random.multinomial(n_sample, probs[1])

            pred0 = prediction[output_name].reshape(2, n_sample)[0]
            pred1 = prediction[output_name].reshape(2, n_sample)[1]

            # convert to bincount and validate probabilities
            pred0 = np.bincount(np.array(pred0).astype(np.int), minlength=n_class)
            pred1 = np.bincount(np.array(pred1).astype(np.int), minlength=n_class)

            assert np.allclose(np.true_divide(pred0, n_sample), probs[0], atol=1e-2)
            assert np.allclose(
                np.true_divide(pred0, n_sample),
                np.true_divide(ref0, n_sample),
                atol=1e-2,
            )

            assert np.allclose(np.true_divide(pred1, n_sample), probs[1], atol=1e-2)
            assert np.allclose(
                np.true_divide(pred1, n_sample),
                np.true_divide(ref1, n_sample),
                atol=1e-2,
            )

        # Test probs input
        input_placeholders = {"x": mb.placeholder(shape=(2, n_class))}
        input_values = {"x": np.array(probs)}

        def build(x):
            return [
                mb.random_categorical(
                    x=x, size=n_sample, mode="probs", name=output_name
                )
            ]

        if _IS_MACOS:
            prediction = get_core_ml_prediction(
                build, input_placeholders, input_values, backend=backend
            )

            pred0 = prediction[output_name].reshape(2, n_sample)[0]
            pred1 = prediction[output_name].reshape(2, n_sample)[1]

            # convert to bincount and validate probabilities
            pred0 = np.bincount(np.array(pred0).astype(np.int), minlength=n_class)
            pred1 = np.bincount(np.array(pred1).astype(np.int), minlength=n_class)

            assert np.allclose(np.true_divide(pred0, n_sample), probs[0], atol=1e-2)
            assert np.allclose(
                np.true_divide(pred0, n_sample),
                np.true_divide(ref0, n_sample),
                atol=1e-2,
            )

            assert np.allclose(np.true_divide(pred1, n_sample), probs[1], atol=1e-2)
            assert np.allclose(
                np.true_divide(pred1, n_sample),
                np.true_divide(ref1, n_sample),
                atol=1e-2,
            )


class TestRandomNormal:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array([0.0], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [
                mb.add(x=x, y=x),
                mb.random_normal(
                    shape=np.array([2, 1, 3], np.int32), mean=1.0, stddev=0.0
                ),
                mb.random_normal(
                    shape=np.array([3, 1, 2], np.int32), mean=0.0, stddev=0.0
                ),
            ]

        expected_outputs = [
            np.array(np.zeros(shape=(1,)), np.float32),
            np.array(np.ones(shape=(2, 1, 3)), np.float32),
            np.array(np.zeros(shape=(3, 1, 2)), np.float32),
        ]

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

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, mean, dynamic",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(1, 6)],
            [1.0, 0.0],
            [True, False],
        ),
    )
    def test_builder_to_backend_stress(
        self, use_cpu_only, backend, rank, mean, dynamic
    ):
        shape = np.random.randint(low=1, high=4, size=rank).astype(np.int32)
        x_val = np.array([0.0], dtype=np.float32)
        if dynamic:
            input_placeholders = {
                "x": mb.placeholder(shape=x_val.shape),
                "dyn_shape": mb.placeholder(shape=shape.shape, dtype=types.int32),
            }
            input_values = {"x": x_val, "dyn_shape": shape}
        else:
            input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
            input_values = {"x": x_val}

        def build(x):
            return [
                mb.add(x=x, y=x),
                mb.random_normal(shape=shape, mean=mean, stddev=0.0),
            ]

        def build_dyn(x, dyn_shape):
            return [
                mb.add(x=x, y=x),
                mb.random_normal(shape=dyn_shape, mean=mean, stddev=0.0),
            ]

        expected_outputs = [
            np.array(np.zeros(shape=(1,)), np.float32),
            np.random.normal(loc=mean, scale=0.0, size=shape),
        ]

        if dynamic:
            expected_output_types = [
                tuple([UNK_SYM for _ in o.shape]) + (types.fp32,)
                for o in expected_outputs
            ]
        else:
            expected_output_types = [
                o.shape[:] + (types.fp32,) for o in expected_outputs
            ]

        builder = build_dyn if dynamic else build
        run_compare_builder(
            builder,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs=expected_outputs,
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestRandomUniform:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array([0.0], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [
                mb.add(x=x, y=x),
                mb.random_uniform(
                    shape=np.array([2, 1, 3], np.int32), low=0.0, high=0.0
                ),
                mb.random_uniform(
                    shape=np.array([3, 1, 2], np.int32), low=1.0, high=1.0
                ),
            ]

        expected_outputs = [
            np.array(np.zeros(shape=(1,)), np.float32),
            np.array(np.zeros(shape=(2, 1, 3)), np.float32),
            np.array(np.ones(shape=(3, 1, 2)), np.float32),
        ]

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

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, low, high, dynamic",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(1, 6)],
            [0.0],
            [0.0],
            [True, False],
        ),
    )
    def test_builder_to_backend_stress(
        self, use_cpu_only, backend, rank, low, high, dynamic
    ):
        shape = np.random.randint(low=1, high=4, size=rank).astype(np.int32)
        x_val = np.array([0.0], dtype=np.float32)
        if dynamic:
            input_placeholders = {
                "x": mb.placeholder(shape=x_val.shape),
                "dyn_shape": mb.placeholder(shape=shape.shape, dtype=types.int32),
            }
            input_values = {"x": x_val, "dyn_shape": shape}
        else:
            input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
            input_values = {"x": x_val}

        def build(x):
            return [
                mb.add(x=x, y=x),
                mb.random_uniform(shape=shape, low=low, high=high),
            ]

        def build_dyn(x, dyn_shape):
            return [
                mb.add(x=x, y=x),
                mb.random_uniform(shape=dyn_shape, low=low, high=high),
            ]

        expected_outputs = [
            np.array(np.zeros(shape=(1,)), np.float32),
            np.random.uniform(low=low, high=high, size=shape),
        ]

        if dynamic:
            expected_output_types = [
                tuple([UNK_SYM for _ in o.shape]) + (types.fp32,)
                for o in expected_outputs
            ]
        else:
            expected_output_types = [
                o.shape[:] + (types.fp32,) for o in expected_outputs
            ]

        builder = build_dyn if dynamic else build
        run_compare_builder(
            builder,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs=expected_outputs,
            use_cpu_only=use_cpu_only,
            backend=backend,
        )
