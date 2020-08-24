#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.testing_reqs import *

from .testing_utils import run_compare_builder

backends = testing_reqs.backends


class TestNormalizationBatchNorm:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array(
            [
                [
                    [[-16.0, 13.0], [11.0, -16.0]],
                    [[13.0, -15.0], [13.0, 9.0]],
                    [[-9.0, -4.0], [-6.0, 3.0]],
                ]
            ],
            dtype=np.float32,
        )
        mean_val = np.array([9.0, 6.0, 3.0], dtype=np.float32)
        variance_val = np.array([6.0, 1.0, 7.0], dtype=np.float32)
        gamma_val = np.array([1.0, 1.0, 1.0], dtype=np.float32)
        beta_val = np.array([1.0, 3.0, 0.0], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [
                mb.batch_norm(x=x, mean=mean_val, variance=variance_val),
                mb.batch_norm(
                    x=x,
                    mean=mean_val,
                    variance=variance_val,
                    gamma=gamma_val,
                    beta=beta_val,
                    epsilon=1e-4,
                ),
            ]

        expected_output_types = [
            (1, 3, 2, 2, types.fp32),
            (1, 3, 2, 2, types.fp32),
        ]
        expected_outputs = [
            np.array(
                [
                    [
                        [[-10.206199, 1.6329918], [0.8164959, -10.206199]],
                        [[6.999965, -20.999895], [6.999965, 2.9999852]],
                        [[-4.53557, -2.6457493], [-3.4016776, 0.0]],
                    ]
                ],
                dtype=np.float32,
            ),
            np.array(
                [
                    [
                        [[-9.206122, 2.6329796], [1.8164899, -9.206122]],
                        [[9.99965, -17.998951], [9.99965, 5.9998503]],
                        [[-4.535541, -2.6457324], [-3.4016557, 0.0]],
                    ]
                ],
                dtype=np.float32,
            ),
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


class TestNormalizationInstanceNorm:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array(
            [
                [
                    [[-16.0, 13.0], [11.0, 16.0]],
                    [[13.0, 15.0], [13.0, 9.0]],
                    [[-9.0, 4.0], [-6.0, 3.0]],
                ]
            ],
            dtype=np.float32,
        )
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return mb.instance_norm(x=x, epsilon=1e-2)

        expected_output_types = [(1, 3, 2, 2, types.fp32)]
        expected_outputs = [
            np.array(
                [
                    [
                        [[-1.71524656, 0.54576027], [0.38982874, 0.77965748]],
                        [[0.22917463, 1.14587319], [0.22917463, -1.60422242]],
                        [[-1.2470212, 1.06887531], [-0.71258354, 0.89072943]],
                    ]
                ],
                dtype=np.float32,
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

    @pytest.mark.skipif(not testing_reqs._HAS_TORCH, reason="PyTorch not found.")
    @pytest.mark.parametrize(
        "use_cpu_only, backend, epsilon",
        itertools.product([True, False], backends, [1e-5, 1e-10]),
    )
    def test_builder_to_backend_stress(self, use_cpu_only, backend, epsilon):
        shape = np.random.randint(low=2, high=6, size=4)
        x_val = random_gen(shape=shape, rand_min=-10.0, rand_max=10.0)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return mb.instance_norm(x=x, epsilon=epsilon)

        torch_op = torch.nn.InstanceNorm2d(num_features=shape[1], eps=epsilon)
        expected_outputs = [torch_op(torch.as_tensor(x_val)).numpy()]
        expected_output_types = [o.shape[:] + (types.fp32,) for o in expected_outputs]

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs,
            use_cpu_only=use_cpu_only,
            backend=backend,
            atol=1e-3,
            rtol=1e-4,
        )


class TestNormalizationL2Norm:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array([[[1.0, -7.0], [5.0, -6.0], [-3.0, -5.0]]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [mb.l2_norm(x=x, axes=[-1], epsilon=1e-10)]

        expected_output_types = [(1, 3, 2, types.fp32)]
        expected_outputs = [
            np.array(
                [
                    [
                        [0.08304548, -0.58131838],
                        [0.41522741, -0.4982729],
                        [-0.24913645, -0.41522741],
                    ]
                ],
                dtype=np.float32,
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


class TestNormalizationLayerNorm:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array([[[1.0, -7.0], [5.0, -6.0], [-3.0, -5.0]]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [
                # V2->V1 lowering (op_mappings.py): if branch
                mb.layer_norm(x=x, axes=[2], epsilon=1e-4),
                # V2->V1 lowering (op_mappings.py): else branch
                mb.layer_norm(x=x, axes=[-2, -1], epsilon=1e-4),
            ]

        expected_output_types = [(1, 3, 2, types.fp32), (1, 3, 2, types.fp32)]
        expected_outputs = [
            np.array(
                [
                    [
                        [0.9999969, -0.9999969],
                        [0.99999839, -0.99999839],
                        [0.99995005, -0.99995005],
                    ]
                ],
                dtype=np.float32,
            ),
            np.array(
                [
                    [
                        [0.8268512, -1.0630943],
                        [1.771824, -0.8268511],
                        [-0.11812156, -0.590608],
                    ]
                ],
                dtype=np.float32,
            ),
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
        def np_layer_norm(x, axes, gamma, beta, epsilon=1e-5):
            normalized_shape = x.shape[-len(axes) :]
            gamma = np.ones(shape=normalized_shape) if gamma is None else gamma
            beta = np.zeros(shape=normalized_shape) if beta is None else beta
            num = x - np.mean(x, axis=tuple(axes), keepdims=True)
            dem = np.sqrt(
                np.sum(np.square(num), axis=tuple(axes), keepdims=True)
                / np.prod(normalized_shape)
                + epsilon
            )
            return num / dem * gamma + beta

        x_val = random_gen(shape=(1, 3, 4, 4), rand_min=-100.0, rand_max=100.0)
        g = random_gen(shape=(4, 4), rand_min=1.0, rand_max=2.0)
        b = random_gen(shape=(4, 4), rand_min=0.0, rand_max=1.0)
        res = mb.layer_norm(x=x_val, axes=[-2, -1], gamma=g, beta=b)
        ref = np_layer_norm(x=x_val, axes=[-2, -1], gamma=g, beta=b)
        assert is_close(ref, res.val)


class TestNormalizationLocalResponseNorm:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array([[[1.0, -7.0], [5.0, -6.0], [-3.0, -5.0]]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [
                mb.local_response_norm(x=x, size=2),
                mb.local_response_norm(x=x, size=3, alpha=0.0001, beta=0.75, k=1.0),
            ]

        expected_output_types = [(1, 3, 2, types.fp32), (1, 3, 2, types.fp32)]
        expected_outputs = [
            np.array(
                [
                    [
                        [0.99996257, -6.98716545],
                        [4.99531746, -5.99191284],
                        [-2.99898791, -4.99531746],
                    ]
                ],
                dtype=np.float32,
            ),
            np.array(
                [
                    [
                        [0.99997497, -6.99143696],
                        [4.99687672, -5.99460602],
                        [-2.99932504, -4.99687672],
                    ]
                ],
                dtype=np.float32,
            ),
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

    @pytest.mark.skipif(not testing_reqs._HAS_TORCH, reason="PyTorch not found.")
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, size, alpha, beta, k",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(3, 6)],
            [2, 3, 5],
            [0.0001, 0.01],
            [0.75, 1.0],
            [1.0, 2.0],
        ),
    )
    def test_builder_to_backend_stress(
        self, use_cpu_only, backend, rank, size, alpha, beta, k
    ):
        shape = np.random.randint(low=2, high=5, size=rank)
        x_val = random_gen(shape=shape)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return mb.local_response_norm(x=x, size=size, alpha=alpha, beta=beta, k=k)

        torch_lrn = torch.nn.LocalResponseNorm(size=size, alpha=alpha, beta=beta, k=k)
        expected_outputs = [torch_lrn(torch.as_tensor(x_val)).numpy()]
        expected_output_types = [o.shape[:] + (types.fp32,) for o in expected_outputs]

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs,
            use_cpu_only=use_cpu_only,
            backend=backend,
            atol=1e-2,
            rtol=1e-3,
        )
