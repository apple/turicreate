#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import random
from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.frontend.tensorflow2.test.testing_utils import (
    run_compare_tf_keras,
)
from coremltools.converters.mil.testing_reqs import *

backends = testing_reqs.backends

tf = pytest.importorskip("tensorflow", minversion="2.1.0")


class TestActivation:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, op",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(1, 6)],
            [
                tf.keras.layers.ELU,
                tf.keras.layers.LeakyReLU,
                tf.keras.layers.ReLU,
                tf.keras.layers.PReLU,
                tf.keras.layers.Softmax,
                tf.keras.layers.ThresholdedReLU,
            ],
        ),
    )
    def test_layer(self, use_cpu_only, backend, rank, op):
        shape = np.random.randint(low=2, high=4, size=rank)
        model = tf.keras.Sequential([op(batch_input_shape=shape)])
        run_compare_tf_keras(
            model,
            [random_gen(shape, -10, 10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, op",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(1, 6)],
            [
                tf.keras.activations.elu,
                tf.keras.activations.exponential,
                tf.keras.activations.hard_sigmoid,
                tf.keras.activations.linear,
                tf.keras.activations.relu,
                tf.keras.activations.selu,
                tf.keras.activations.sigmoid,
                tf.keras.activations.softmax,
                tf.keras.activations.softplus,
                tf.keras.activations.softsign,
                tf.keras.activations.tanh,
            ],
        ),
    )
    def test_activation(self, use_cpu_only, backend, rank, op):
        kwargs = (
            {"atol": 1e-3, "rtol": 1e-4}
            if op == tf.keras.activations.exponential and use_cpu_only is False
            else {}
        )
        if op == tf.keras.activations.softmax and rank == 1:
            return  # skip apply softmax to a tensor that is 1D
        shape = np.random.randint(low=2, high=4, size=rank)
        model = tf.keras.Sequential(
            [tf.keras.layers.Activation(op, batch_input_shape=shape)]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, -10, 10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
            **kwargs
        )


class TestBinary:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, op",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(2, 6)],
            [
                tf.keras.layers.Add,
                tf.keras.layers.Average,
                tf.keras.layers.Subtract,
                tf.keras.layers.Maximum,
                tf.keras.layers.Minimum,
            ],
        ),
    )
    def test(self, use_cpu_only, backend, rank, op):
        shape = np.random.randint(low=1, high=4, size=rank)
        input_x = tf.keras.layers.Input(batch_input_shape=tuple(shape))
        input_y = tf.keras.layers.Input(batch_input_shape=tuple(shape))
        out = op()([input_x, input_y])
        model = tf.keras.Model(inputs=[input_x, input_y], outputs=out)
        run_compare_tf_keras(
            model,
            [random_gen(shape, -10, 10), random_gen(shape, -10, 10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, axes, normalize",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(2, 3)],
            [-1,],
            [True, False],
        ),
    )
    def test_dot(self, use_cpu_only, rank, backend, axes, normalize):
        shape = np.random.randint(low=2, high=4, size=rank)
        input_x = tf.keras.layers.Input(batch_input_shape=tuple(shape))
        input_y = tf.keras.layers.Input(batch_input_shape=tuple(shape))
        out = tf.keras.layers.Dot(axes=axes, normalize=normalize)([input_x, input_y])
        model = tf.keras.Model(inputs=[input_x, input_y], outputs=out)
        run_compare_tf_keras(
            model,
            [random_gen(shape, -10, 10), random_gen(shape, -10, 10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestConcatenate:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, axis",
        itertools.product(
            [True, False], backends, [rank for rank in range(5, 6)], [-1, -2],
        ),
    )
    def test(self, use_cpu_only, backend, rank, axis):
        shape = np.random.randint(low=2, high=4, size=rank)
        inputs = []
        for _ in range(2):
            inputs.append(tf.keras.layers.Input(batch_input_shape=tuple(shape)))
        out = tf.keras.layers.Concatenate(axis=axis)(inputs)
        model = tf.keras.Model(inputs=inputs, outputs=out)
        run_compare_tf_keras(
            model,
            [random_gen(shape), random_gen(shape)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestConvolution:
    @pytest.mark.parametrize(
        ",".join(
            [
                "use_cpu_only",
                "backend",
                "op",
                "padding",
                "data_format",
                "spatial_dim_and_ks",
                "strides",
                "dilations",
                "batch_size",
            ]
        ),
        itertools.product(
            [True, False],
            backends,
            [
                tf.keras.layers.Conv1D,
                tf.keras.layers.Conv2D,
                tf.keras.layers.Conv3D,
                tf.keras.layers.LocallyConnected1D,
                tf.keras.layers.LocallyConnected2D,
            ],
            ["same", "valid"],
            ["channels_last"],
            [(2, 4, 4, 2, 2, 2), (3, 7, 5, 1, 3, 2)],
            [(1, 1, 1), (1, 2, 3), (1, 3, 2)],
            [
                (1, 1, 1)
            ],  # rdar://62951360 (Enhance SpaceToBatchND op to support more dialation rate of Conv)
            [1, 3],
        ),
    )
    def test_conv(
        self,
        use_cpu_only,
        backend,
        op,
        padding,
        data_format,
        spatial_dim_and_ks,
        strides,
        dilations,
        batch_size,
    ):
        s1, s2, s3, k1, k2, k3 = spatial_dim_and_ks
        c_in, c_out = 2, 3
        input_shape = None
        kernel_size = None
        if op in {tf.keras.layers.Conv1D, tf.keras.layers.LocallyConnected1D}:
            input_shape = (batch_size, s3, c_in)
            kernel_size = k3
            strides = strides[2]
            dilations = dilations[2]
        elif op in {tf.keras.layers.Conv2D, tf.keras.layers.LocallyConnected2D}:
            input_shape = (batch_size, s2, s3, c_in)
            kernel_size = (k2, k3)
            strides = (strides[1], strides[2])
            dilations = dilations[1:]
        elif op == tf.keras.layers.Conv3D:
            input_shape = (batch_size, s1, s2, s3, c_in)
            kernel_size = (k1, k2, k3)

        if op in {
            tf.keras.layers.LocallyConnected1D,
            tf.keras.layers.LocallyConnected2D,
        }:
            if padding != "valid":
                return  # tf.keras only supports "valid"
            model = tf.keras.Sequential(
                [
                    op(
                        batch_input_shape=input_shape,
                        filters=c_out,
                        kernel_size=kernel_size,
                        strides=strides,
                        padding=padding.upper(),
                        data_format=data_format,
                    )
                ]
            )
        else:
            model = tf.keras.Sequential(
                [
                    op(
                        batch_input_shape=input_shape,
                        filters=c_out,
                        kernel_size=kernel_size,
                        strides=strides,
                        padding=padding.upper(),
                        data_format=data_format,
                        dilation_rate=dilations,
                    )
                ]
            )

        run_compare_tf_keras(
            model,
            [random_gen(input_shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        ",".join(
            [
                "use_cpu_only",
                "backend",
                "op",
                "padding",
                "data_format",
                "spatial_dim_and_ks",
                "output_padding",
                "strides",
                "dilations",
                "batch_size",
            ]
        ),
        itertools.product(
            [True, False],
            backends,
            # TODO: rdar://63968613 ([deconv3d] Deconv_3d top_shapes_for_bottom_shapes does not sets output channel if output shape is provided)
            [tf.keras.layers.Conv2DTranspose],  # tf.keras.layers.Conv3DTranspose],
            ["same", "valid"],
            ["channels_last"],
            [(7, 11, 12, 1, 2, 2), (9, 5, 7, 3, 3, 3)],
            [(1, 1, 1)],
            [(2, 2, 2), (2, 3, 3)],
            [(1, 1, 1)],  # Dilation > 1 not supported by TF
            [1, 3],
        ),
    )
    def test_conv_transpose(
        self,
        use_cpu_only,
        backend,
        op,
        padding,
        data_format,
        spatial_dim_and_ks,
        output_padding,
        strides,
        dilations,
        batch_size,
    ):
        s1, s2, s3, k1, k2, k3 = spatial_dim_and_ks
        c_in, c_out = 2, 3
        input_shape = None
        kernel_size = None
        if op == tf.keras.layers.Conv2DTranspose:
            input_shape = (batch_size, s2, s3, c_in)
            kernel_size = (k2, k3)
            strides = (strides[1], strides[2])
            dilations = dilations[1:]
            output_padding = (output_padding[1], output_padding[2])
        elif op == tf.keras.layers.Conv3DTranspose:
            input_shape = (batch_size, s1, s2, s3, c_in)
            kernel_size = (k1, k2, k3)

        model = tf.keras.Sequential(
            [
                op(
                    batch_input_shape=input_shape,
                    filters=c_out,
                    kernel_size=kernel_size,
                    strides=strides,
                    padding=padding.upper(),
                    output_padding=output_padding,
                    data_format=data_format,
                    dilation_rate=dilations,
                )
            ]
        )

        run_compare_tf_keras(
            model,
            [random_gen(input_shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        ",".join(
            [
                "use_cpu_only",
                "backend",
                "op",
                "padding",
                "data_format",
                "spatial_dim_and_ks",
                "strides",
                "dilations",
                "batch_size",
            ]
        ),
        itertools.product(
            [True, False],
            backends,
            [tf.keras.layers.DepthwiseConv2D],
            ["same", "valid"],
            ["channels_last"],
            [(11, 12, 3, 2), (12, 11, 2, 3)],
            [(1, 1), (2, 2)],
            [(1, 1), (2, 2)],
            [1, 3],
        ),
    )
    def test_depth_wise_conv(
        self,
        use_cpu_only,
        backend,
        op,
        padding,
        data_format,
        spatial_dim_and_ks,
        strides,
        dilations,
        batch_size,
    ):
        s1, s2, k1, k2 = spatial_dim_and_ks
        c_in, c_out = 2, 6

        if len(strides) != np.sum(strides) and len(dilations) != np.sum(dilations):
            # TF produces incorrect output for non-one strides + dilations
            return

        input_shape = (batch_size, s1, s2, c_in)
        model = tf.keras.Sequential(
            [
                op(
                    batch_input_shape=input_shape,
                    kernel_size=(k1, k2),
                    strides=strides,
                    padding=padding.upper(),
                    data_format=data_format,
                    dilation_rate=dilations,
                )
            ]
        )

        run_compare_tf_keras(
            model,
            [random_gen(input_shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        ",".join(
            [
                "use_cpu_only",
                "backend",
                "op",
                "padding",
                "data_format",
                "spatial_dim_and_ks",
                "strides",
                "dilations",
                "batch_size",
            ]
        ),
        itertools.product(
            [True, False],
            backends,
            [tf.keras.layers.SeparableConv1D, tf.keras.layers.SeparableConv2D],
            ["same", "valid"],
            ["channels_last"],
            [(14, 14, 2, 2), (11, 9, 3, 2), (12, 11, 2, 3)],
            [(1, 1), (2, 2), (3, 3)],
            [(1, 1)],
            [1, 3],
        ),
    )
    def test_separable_conv(
        self,
        use_cpu_only,
        backend,
        op,
        padding,
        data_format,
        spatial_dim_and_ks,
        strides,
        dilations,
        batch_size,
    ):
        s1, s2, k1, k2 = spatial_dim_and_ks
        c_in, c_out = 2, 3
        input_shape = None
        kernel_size = None
        if op == tf.keras.layers.SeparableConv1D:
            input_shape = (batch_size, s2, c_in)
            kernel_size = k2
            strides = strides[1]
            dilations = dilations[1]
        elif op == tf.keras.layers.SeparableConv2D:
            input_shape = (batch_size, s1, s2, c_in)
            kernel_size = (k1, k2)

        model = tf.keras.Sequential(
            [
                op(
                    batch_input_shape=input_shape,
                    filters=c_out,
                    kernel_size=kernel_size,
                    strides=strides,
                    padding=padding.upper(),
                    data_format=data_format,
                    dilation_rate=dilations,
                )
            ]
        )

        run_compare_tf_keras(
            model,
            [random_gen(input_shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestCropping:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, begin_end",
        itertools.product(
            [True, False], backends, [(0, 0), (1, 1), (1, 2), (2, 1), (2, 4), (3, 2)],
        ),
    )
    def test_cropping_1d(self, use_cpu_only, backend, begin_end):
        shape = (1, 10, 3)
        model = tf.keras.Sequential(
            [tf.keras.layers.Cropping1D(batch_input_shape=shape, cropping=begin_end)]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-1, rand_max=1)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, begin_end1, begin_end2",
        itertools.product(
            [True, False],
            backends,
            [(0, 0), (1, 1), (1, 2), (2, 1), (2, 4)],
            [(0, 0), (1, 1), (1, 2), (2, 1), (4, 2)],
        ),
    )
    def test_cropping_2d(self, use_cpu_only, backend, begin_end1, begin_end2):
        shape = (1, 10, 10, 3)
        model = tf.keras.Sequential(
            [
                tf.keras.layers.Cropping2D(
                    batch_input_shape=shape, cropping=(begin_end1, begin_end2)
                )
            ]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-1, rand_max=1)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, begin_end1, begin_end2, begin_end3",
        itertools.product(
            [True, False],
            backends,
            [(0, 0), (1, 1), (1, 2), (2, 1), (2, 4)],
            [(0, 0), (1, 1), (1, 2), (2, 1), (4, 2)],
            [(0, 0), (1, 1), (1, 2), (2, 1), (2, 4)],
        ),
    )
    def test_cropping_3d(
        self, use_cpu_only, backend, begin_end1, begin_end2, begin_end3
    ):
        shape = (1, 10, 10, 10, 3)
        model = tf.keras.Sequential(
            [
                tf.keras.layers.Cropping3D(
                    batch_input_shape=shape,
                    cropping=(begin_end1, begin_end2, begin_end3),
                )
            ]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-1, rand_max=1)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestDense:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, units, activation, use_bias",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(2, 6)],
            [2, 4, 8],
            [tf.nn.relu, tf.nn.softmax, tf.nn.swish],
            [True, False],
        ),
    )
    def test(self, use_cpu_only, backend, rank, units, activation, use_bias):
        shape = np.random.randint(low=2, high=4, size=rank)
        model = tf.keras.Sequential(
            [
                tf.keras.layers.Dense(
                    batch_input_shape=shape,
                    units=units,
                    activation=activation,
                    use_bias=use_bias,
                )
            ]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestEmbedding:
    @pytest.mark.xfail(reason="rdar://63414784")
    @pytest.mark.parametrize(
        "use_cpu_only, backend, dims, batch_size, input_length",
        itertools.product(
            [True, False],
            backends,
            [(4, 1), (8, 3), (16, 5), (32, 7), (64, 9)],
            [1, 3, 5],
            [2, 4, 10],
        ),
    )
    def test(self, use_cpu_only, backend, dims, batch_size, input_length):
        # input shape: 2D tensor (batch_size, input_length)
        # output shape: 3D tensor (batch_size, input_length, output_dim)
        shape = (batch_size, input_length)
        model = tf.keras.Sequential(
            [
                tf.keras.layers.Embedding(
                    batch_input_shape=shape,
                    input_dim=dims[0],
                    output_dim=dims[1],
                    input_length=input_length,
                )
            ]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=0, rand_max=dims[0])],
            use_cpu_only=use_cpu_only,
            backend=backend,
            atol=1e-3,
            rtol=1e-4,
        )


class TestFlatten:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, data_format",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(1, 6)],
            ["channels_last", "channels_first"],
        ),
    )
    def test(self, use_cpu_only, backend, rank, data_format):
        shape = np.random.randint(low=2, high=4, size=rank)
        model = tf.keras.Sequential(
            [tf.keras.layers.Flatten(batch_input_shape=shape, data_format=data_format,)]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestLambda:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, function",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(1, 6)],
            [
                lambda x: x + x,
                lambda x: x * 3.14 - 1.0,
                lambda x: np.sqrt(4) + x,
                lambda x: tf.math.abs(x),
            ],
        ),
    )
    def test_unary(self, use_cpu_only, backend, rank, function):
        shape = np.random.randint(low=2, high=4, size=rank)
        model = tf.keras.Sequential(
            [tf.keras.layers.Lambda(batch_input_shape=shape, function=function,)]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-5, rand_max=5)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestNormalization:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, axis, momentum, epsilon",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(1, 6)],
            [0, -1],
            [0.99, 0.85],
            [1e-2, 1e-5],
        ),
    )
    def test_batch_normalization(
        self, use_cpu_only, backend, rank, axis, momentum, epsilon
    ):
        shape = np.random.randint(low=2, high=4, size=rank)
        model = tf.keras.Sequential(
            [
                tf.keras.layers.BatchNormalization(
                    batch_input_shape=shape,
                    axis=axis,
                    momentum=momentum,
                    epsilon=epsilon,
                )
            ]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank_and_axis, momentum, epsilon",
        itertools.product(
            [True, False], backends, [(4, 1), (4, -3)], [0.99, 0.85], [1e-2, 1e-5],
        ),
    )
    def test_fused_batch_norm_v3(
        self, use_cpu_only, backend, rank_and_axis, momentum, epsilon
    ):
        rank, axis = rank_and_axis
        shape = np.random.randint(low=2, high=4, size=rank)
        model = tf.keras.Sequential(
            [
                tf.keras.layers.BatchNormalization(
                    batch_input_shape=shape,
                    axis=axis,
                    momentum=momentum,
                    epsilon=epsilon,
                )
            ]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, axis, epsilon",
        itertools.product(
            [True, False], backends, [rank for rank in range(3, 4)], [-1,], [1e-10],
        ),
    )
    def test_layer_normalization(self, use_cpu_only, backend, rank, axis, epsilon):
        shape = np.random.randint(low=2, high=4, size=rank)
        model = tf.keras.Sequential(
            [
                tf.keras.layers.LayerNormalization(
                    batch_input_shape=shape, axis=axis, epsilon=epsilon, trainable=False
                )
            ]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-100, rand_max=100)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, axis, epsilon, center, scale",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(4, 5)],
            [-1],
            [1e-3, 1e-5],
            [True, False],
            [True, False],
        ),
    )
    def test_instance_normalization(
        self, use_cpu_only, backend, rank, axis, epsilon, center, scale
    ):
        tensorflow_addons = pytest.importorskip("tensorflow_addons")
        from tensorflow_addons.layers import InstanceNormalization

        shape = np.random.randint(low=2, high=4, size=rank)
        model = tf.keras.Sequential(
            [
                InstanceNormalization(
                    batch_input_shape=shape,
                    axis=axis,
                    epsilon=epsilon,
                    center=center,
                    scale=scale,
                )
            ]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-1, rand_max=1)],
            use_cpu_only=use_cpu_only,
            backend=backend,
            atol=1e-3,
            rtol=1e-4,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, groups, axis, epsilon, center, scale",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(4, 5)],
            [1, 2, 3],
            [-1],
            [1e-3, 1e-5],
            [True, False],
            [True, False],
        ),
    )
    def test_group_normalization(
        self, use_cpu_only, backend, rank, groups, axis, epsilon, center, scale
    ):
        tensorflow_addons = pytest.importorskip("tensorflow_addons")
        from tensorflow_addons.layers import GroupNormalization

        shape = np.random.randint(low=2, high=4, size=rank)
        shape[-1] = shape[-1] * groups  # groups must be a multiple of channels
        model = tf.keras.Sequential(
            [
                GroupNormalization(
                    batch_input_shape=shape,
                    groups=groups,
                    axis=axis,
                    epsilon=epsilon,
                    center=center,
                    scale=scale,
                )
            ]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-1, rand_max=1)],
            use_cpu_only=use_cpu_only,
            backend=backend,
            atol=1e-3,
            rtol=1e-4,
        )


class TestPadding:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, op, data_format, padding",
        itertools.product(
            [True, False],
            backends,
            [
                tf.keras.layers.ZeroPadding1D,
                tf.keras.layers.ZeroPadding2D,
                tf.keras.layers.ZeroPadding3D,
            ],
            ["channels_first", "channels_last"],
            [(1, 1, 1), (2, 2, 2), (3, 3, 3), (1, 3, 4), (2, 3, 5)],
        ),
    )
    def test(self, use_cpu_only, backend, op, data_format, padding):
        shape = None
        kwargs = {}
        if op == tf.keras.layers.ZeroPadding1D:
            padding = padding[-1]
            shape = np.random.randint(low=2, high=4, size=3)
        elif op == tf.keras.layers.ZeroPadding2D:
            padding = padding[1:]
            kwargs = {"data_format": data_format}
            shape = np.random.randint(low=2, high=4, size=4)
        elif op == tf.keras.layers.ZeroPadding3D:
            kwargs = {"data_format": data_format}
            shape = np.random.randint(low=2, high=4, size=5)
        model = tf.keras.Sequential(
            [op(batch_input_shape=shape, padding=padding, **kwargs)]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestPermute:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank_and_perm",
        itertools.product(
            [True, False],
            backends,
            [
                (rank, perm)
                for rank in range(3, 6)
                for perm in list(itertools.permutations(range(rank)[1:]))
            ],
        ),
    )
    def test(self, use_cpu_only, backend, rank_and_perm):
        rank, perm = rank_and_perm
        shape = np.random.randint(low=2, high=4, size=rank)
        model = tf.keras.Sequential(
            [tf.keras.layers.Permute(batch_input_shape=shape, dims=perm)]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestPooling:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, op, data_format",
        itertools.product(
            [True, False],
            backends,
            [
                tf.keras.layers.GlobalAveragePooling1D,
                tf.keras.layers.GlobalAveragePooling2D,
                tf.keras.layers.GlobalAveragePooling3D,
                tf.keras.layers.GlobalMaxPool1D,
                tf.keras.layers.GlobalMaxPool2D,
                tf.keras.layers.GlobalMaxPool3D,
            ],
            ["channels_first", "channels_last"],
        ),
    )
    def test_global_pooling(self, use_cpu_only, backend, op, data_format):
        shape = None
        if op in {
            tf.keras.layers.GlobalAveragePooling1D,
            tf.keras.layers.GlobalMaxPool1D,
        }:
            shape = np.random.randint(low=2, high=4, size=3)
        elif op in {
            tf.keras.layers.GlobalAveragePooling2D,
            tf.keras.layers.GlobalMaxPool2D,
        }:
            shape = np.random.randint(low=2, high=4, size=4)
        elif op in {
            tf.keras.layers.GlobalAveragePooling3D,
            tf.keras.layers.GlobalMaxPool3D,
        }:
            shape = np.random.randint(low=2, high=4, size=5)
        model = tf.keras.Sequential(
            [op(batch_input_shape=shape, data_format=data_format)]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, op, data_format, pool_size",
        itertools.product(
            [True, False],
            backends,
            [
                tf.keras.layers.AveragePooling1D,
                tf.keras.layers.AveragePooling2D,
                tf.keras.layers.AveragePooling3D,
                tf.keras.layers.MaxPool1D,
                tf.keras.layers.MaxPool2D,
                tf.keras.layers.MaxPool3D,
            ],
            ["channels_first", "channels_last"],
            [(2, 2, 1), (2, 3, 2), (1, 2, 3)],
        ),
    )
    def test_pooling(self, use_cpu_only, backend, op, data_format, pool_size):
        shape = None
        if op in {tf.keras.layers.AveragePooling1D, tf.keras.layers.MaxPool1D}:
            shape = np.random.randint(low=3, high=9, size=3)
            pool_size = pool_size[2]
        elif op in {tf.keras.layers.AveragePooling2D, tf.keras.layers.MaxPool2D}:
            if data_format == "channels_first":
                return  # AvgPoolingOp only supports NHWC on CPU
            shape = np.random.randint(low=3, high=9, size=4)
            pool_size = pool_size[1:]
        elif op in {tf.keras.layers.AveragePooling3D, tf.keras.layers.MaxPool3D}:
            shape = np.random.randint(low=3, high=9, size=5)
        model = tf.keras.Sequential(
            [op(batch_input_shape=shape, pool_size=pool_size, data_format=data_format)]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestRecurrent:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, units, activation, "
        "recurrent_activation, use_bias, return_sequences",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(3, 4)],
            [1, 3],
            [None, tf.nn.tanh],
            [None, tf.nn.relu],
            [True, False],
            [True, False],
        ),
    )
    def test_lstm(
        self,
        use_cpu_only,
        backend,
        rank,
        units,
        activation,
        recurrent_activation,
        use_bias,
        return_sequences,
    ):
        shape = np.random.randint(low=1, high=4, size=rank)
        model = tf.keras.Sequential(
            [
                tf.keras.layers.LSTM(
                    batch_input_shape=shape,
                    units=units,
                    activation=activation,
                    recurrent_activation=recurrent_activation,
                    use_bias=use_bias,
                    return_sequences=return_sequences,
                ),
            ]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-1, rand_max=1)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_lstmcell(self, use_cpu_only, backend):
        shape = np.random.randint(low=1, high=4, size=3)
        model = tf.keras.Sequential(
            [
                tf.keras.layers.RNN(
                    batch_input_shape=shape, cell=tf.keras.layers.LSTMCell(units=3)
                )
            ]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-1, rand_max=1)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_lstm_time_distributed_dense(self, use_cpu_only, backend):
        shape = list(np.random.randint(low=1, high=4, size=3))
        k_in = tf.keras.layers.Input(batch_size=shape[0], shape=shape[1:])
        lstm = tf.keras.layers.LSTM(units=32, return_sequences=True)(k_in)
        k_out = tf.keras.layers.TimeDistributed(tf.keras.layers.Dense(1))(lstm)
        model = tf.keras.Model(inputs=k_in, outputs=k_out)

        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-1, rand_max=1)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestRepeatVector:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, n",
        itertools.product([True, False], backends, [2, 3, 5, 7],),
    )
    def test(self, use_cpu_only, backend, n):
        # input shape 2D tensor (batch size, features)
        # output shape 3D tensor (batch size, n, features)
        shape = np.random.randint(low=1, high=4, size=2)
        model = tf.keras.Sequential(
            [tf.keras.layers.RepeatVector(batch_input_shape=shape, n=n)]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestReshape:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, infer_shape",
        itertools.product(
            [True, False], backends, [rank for rank in range(1, 6)], [True, False],
        ),
    )
    def test(self, use_cpu_only, backend, rank, infer_shape):
        shape = np.random.randint(low=2, high=4, size=rank)
        # target shape does not include the batch dimension
        target_shape = random.sample(list(shape[1:]), len(shape[1:]))
        if len(target_shape) > 0 and infer_shape:
            target_shape[-1] = -1
        model = tf.keras.Sequential(
            [
                tf.keras.layers.Reshape(
                    batch_input_shape=shape, target_shape=target_shape
                )
            ]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestSkips:
    # ops in this class should be ignored / pass-through during conversion

    @pytest.mark.parametrize(
        "use_cpu_only, backend, skip_op",
        itertools.product(
            [True, False],
            backends,
            [
                tf.keras.layers.Dropout,
                tf.keras.layers.AlphaDropout,
                tf.keras.layers.GaussianDropout,
                tf.keras.layers.SpatialDropout1D,
                tf.keras.layers.SpatialDropout2D,
                tf.keras.layers.SpatialDropout3D,
            ],
        ),
    )
    def test_skip_dropout(self, use_cpu_only, backend, skip_op):
        shape = np.random.randint(low=1, high=4, size=5)
        if skip_op == tf.keras.layers.SpatialDropout1D:
            shape = shape[:3]
        elif skip_op == tf.keras.layers.SpatialDropout2D:
            shape = shape[:4]
        model = tf.keras.Sequential([skip_op(batch_input_shape=shape, rate=0.5)])
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_skip_noise(self, use_cpu_only, backend):
        shape = np.random.randint(low=1, high=4, size=5)
        model = tf.keras.Sequential(
            [
                # GaussianNoise should do nothing in inference mode
                tf.keras.layers.GaussianNoise(batch_input_shape=shape, stddev=0.5)
            ]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, l1, l2",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(5, 6)],
            [0.0, 0.5, 1.0],
            [0.0, 0.5, 1.0],
        ),
    )
    def test_skip_regularization(self, use_cpu_only, backend, rank, l1, l2):
        shape = np.random.randint(low=2, high=4, size=rank)
        model = tf.keras.Sequential(
            [
                tf.keras.layers.ActivityRegularization(
                    batch_input_shape=shape, l1=l1, l2=l2
                )
            ]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestUpSampling:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, op, upsample_factor, data_format, interpolation",
        itertools.product(
            [True, False],
            backends,
            [
                tf.keras.layers.UpSampling1D,
                tf.keras.layers.UpSampling2D,
                tf.keras.layers.UpSampling3D,
            ],
            [(2, 2, 1), (4, 3, 2), (1, 2, 3)],
            ["channels_first", "channels_last"],
            ["nearest", "bilinear"],
        ),
    )
    def test(
        self, use_cpu_only, backend, op, upsample_factor, data_format, interpolation
    ):
        kwargs = {}
        shape = None
        if op == tf.keras.layers.UpSampling1D:
            shape = np.random.randint(low=2, high=4, size=3)
            upsample_factor = upsample_factor[2]
        elif op == tf.keras.layers.UpSampling2D:
            kwargs = {"data_format": data_format, "interpolation": interpolation}
            shape = np.random.randint(low=2, high=4, size=4)
            upsample_factor = (upsample_factor[1], upsample_factor[2])
        elif op == tf.keras.layers.UpSampling3D:
            kwargs = {"data_format": data_format}
            shape = np.random.randint(low=2, high=4, size=5)

        model = tf.keras.Sequential(
            [op(batch_input_shape=shape, size=upsample_factor, **kwargs)]
        )
        run_compare_tf_keras(
            model,
            [random_gen(shape, rand_min=-10, rand_max=10)],
            use_cpu_only=use_cpu_only,
            backend=backend,
        )
