#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.frontend.tensorflow.test import (
    testing_utils as tf_testing_utils,
)
from coremltools.converters.mil.frontend.tensorflow2.test.testing_utils import (
    make_tf2_graph as make_tf_graph,
    run_compare_tf2 as run_compare_tf,
)
from coremltools.converters.mil.testing_reqs import *

tf = pytest.importorskip("tensorflow", minversion="2.1.0")

backends = testing_reqs.backends

# -----------------------------------------------------------------------------
# Overwrite utilities to enable different conversion / compare method
tf_testing_utils.frontend = "TensorFlow2"
tf_testing_utils.make_tf_graph = make_tf_graph
tf_testing_utils.run_compare_tf = run_compare_tf

# -----------------------------------------------------------------------------
# Import TF 2.x-compatible TF 1.x test cases
from coremltools.converters.mil.frontend.tensorflow.test.test_custom_ops import (
    TestCompositeOp,
)
from coremltools.converters.mil.frontend.tensorflow.test.test_ops import (
    TestActivationElu,
    TestActivationLeakyReLU,
    TestActivationReLU,
    TestActivationReLU6,
    TestActivationSelu,
    TestActivationSigmoid,
    TestActivationSoftmax,
    TestActivationSoftPlus,
    TestActivationSoftSign,
    TestAddN,
    TestBroadcastTo,
    TestBatchToSpaceND,
    TestCond,
    TestConcat,  # Redirects to ConcatV2 in TF2
    TestConv,
    TestConv3d,
    TestDepthwiseConv,
    TestElementWiseBinary,
    TestIsFinite,
    TestLinear,
    TestNormalization,
    TestPad,
    TestPack,
    TestPooling1d,
    TestPooling2d,
    TestPooling3d,
    TestSeparableConv,
    TestSpaceToBatchND,
    TestTensorArray,
    TestWhileLoop,
    TestReshape,
    TestSelect,
    TestSlice,
    TestZerosLike,
)

del TestWhileLoop.test_nested_while_body  # tf.function() error in TF2


class TestNormalizationTF2:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, epsilon",
        itertools.product([True, False], backends, [1e-1, 1e-10]),
    )
    def test_fused_batch_norm_v3(self, use_cpu_only, backend, epsilon):
        input_shape = np.random.randint(low=1, high=4, size=4)
        attr_shape = [list(input_shape)[-1]]

        m = random_gen(shape=attr_shape, rand_min=-1.0, rand_max=1.0)
        v = random_gen(shape=attr_shape, rand_min=0.0, rand_max=10.0)
        o = random_gen(shape=attr_shape, rand_min=1.0, rand_max=10.0)
        s = random_gen(shape=attr_shape, rand_min=-1.0, rand_max=1.0)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.raw_ops.FusedBatchNormV3(
                x=x,
                scale=s,
                offset=o,
                mean=m,
                variance=v,
                epsilon=epsilon,
                is_training=False,
            )[0]

        model, inputs, outputs = build_model
        input_values = [random_gen(shape=input_shape)]
        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            backend=backend,
            atol=1e-2,
            rtol=1e-3,
        )


class TestElementWiseBinaryTF2:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True], backends, [rank for rank in range(1, 4)]),  # False
    )
    def test_add_v2(self, use_cpu_only, backend, rank):
        x_shape = list(np.random.randint(low=2, high=5, size=rank))
        y_shape = x_shape[:]
        for i in range(rank):
            if np.random.randint(4) == 0:
                y_shape[i] = 1
        if np.random.randint(2) == 0:
            y_shape = [1] + y_shape

        if use_cpu_only:
            dtype = np.float32
        else:
            dtype = np.float16

        @make_tf_graph([x_shape, y_shape])
        def build_model(x, y):
            return tf.raw_ops.AddV2(x=x, y=y)

        model, inputs, outputs = build_model

        input_values = [
            np.random.randint(low=-1000, high=1000, size=x_shape).astype(dtype),
            np.random.randint(low=-1000, high=1000, size=y_shape).astype(dtype),
        ]

        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )


class TestControlFlowFromAutoGraph:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_if_unary_const(self, use_cpu_only, backend):
        @make_tf_graph([(1,)])
        def build_model(x):
            if x > 0.5:
                y = x - 0.5
            else:
                y = x + 0.5
            return y

        model, inputs, outputs = build_model
        input_values = [np.array([0.7], dtype=np.float32)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_if_unary_double_if_positive_else_square(self, use_cpu_only, backend):
        @make_tf_graph([(1,)])
        def build_model(x):
            if x >= 0:
                out = x + x
            else:
                out = x * x
            return out

        model, inputs, outputs = build_model
        input_values = [np.array([2], dtype=np.float32)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_if_binary_add_if_else_mul(self, use_cpu_only, backend):
        @make_tf_graph([(1,), (1,)])
        def build_model(x, y):
            if x > y:
                out = x + x
            else:
                out = x * x
            return out

        model, inputs, outputs = build_model
        input_values = [
            np.array([3], dtype=np.float32),
            np.array([7], dtype=np.float32),
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_while_loop_square(self, use_cpu_only, backend):
        @make_tf_graph([(1,)])
        def build_model(x):
            i = 0
            while i < 10:
                x *= 2
                i += 1
            return x

        model, inputs, outputs = build_model
        input_values = [np.array([2.0], dtype=np.float32)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_while_loop_power(self, use_cpu_only, backend):
        @make_tf_graph([(1,)])
        def build_model(x):
            i = 0
            while i < 3:
                x *= x
                i += 1
            return x

        model, inputs, outputs = build_model
        input_values = [np.array([2.0], dtype=np.float32)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_while_loop_nested_body(self, use_cpu_only, backend):
        @make_tf_graph([(1,)])
        def build_model(x):
            i, j = 0, 10
            while i < j:
                while 2 * i < i + 2:
                    i += 1
                    x -= 1
                i += 2
                x *= 2
            return x

        model, inputs, outputs = build_model
        input_values = [np.array([9.0], dtype=np.float32)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )


class TestTensorList:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, size_dynamic_shape",
        itertools.product(
            [True, False],
            backends,
            [(1, True, None), (1, True, (1,)), (2, False, (1,))],
        ),
    )
    def test_write_read_and_stack(self, use_cpu_only, backend, size_dynamic_shape):
        size, dynamic_size, element_shape = size_dynamic_shape

        @make_tf_graph([(1,), (1,)])
        def build_model(x, y):
            ta = tf.TensorArray(
                tf.float32,
                size=size,
                dynamic_size=dynamic_size,
                element_shape=element_shape,
            )
            ta = ta.write(0, x)
            ta = ta.write(1, y)
            return ta.read(0), ta.read(1), ta.stack()

        model, inputs, outputs = build_model
        input_values = [
            np.array([3.14], dtype=np.float32),
            np.array([6.17], dtype=np.float32),
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, size_dynamic_shape",
        itertools.product(
            [True, False],
            backends,
            [(0, True, None), (1, True, (1,)), (3, False, (1,))],
        ),
    )
    def test_unstack_and_read(self, use_cpu_only, backend, size_dynamic_shape):
        size, dynamic_size, element_shape = size_dynamic_shape

        @make_tf_graph([(3, 1)])
        def build_model(x):
            ta = tf.TensorArray(
                tf.float32,
                size=size,
                dynamic_size=dynamic_size,
                element_shape=element_shape,
            )
            ta = ta.unstack(x)
            return ta.read(0), ta.read(1), ta.read(2)

        model, inputs, outputs = build_model
        input_values = [np.array([[3.14], [6.17], [12.14]], dtype=np.float32)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, size_dynamic_shape",
        itertools.product(
            [True, False],
            backends,
            [(2, True, None), (1, True, (1,)), (3, False, (1,))],
        ),
    )
    def test_write_and_gather(self, use_cpu_only, backend, size_dynamic_shape):
        size, dynamic_size, element_shape = size_dynamic_shape

        @make_tf_graph([(1,), (1,)])
        def build_model(x, y):
            ta = tf.TensorArray(
                tf.float32,
                size=size,
                dynamic_size=dynamic_size,
                element_shape=element_shape,
            )
            ta = ta.write(0, x)
            ta = ta.write(1, y)
            return ta.gather(indices=[0, 1])

        model, inputs, outputs = build_model
        input_values = [
            np.array([3.14], dtype=np.float32),
            np.array([6.17], dtype=np.float32),
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, size_dynamic_shape",
        itertools.product(
            [True, False],
            backends,
            [(2, True, None), (1, True, (1,)), (3, False, (1,))],
        ),
    )
    def test_scatter_and_read(self, use_cpu_only, backend, size_dynamic_shape):
        size, dynamic_size, element_shape = size_dynamic_shape

        @make_tf_graph([(3, 1)])
        def build_model(x):
            ta = tf.TensorArray(
                tf.float32,
                size=size,
                dynamic_size=dynamic_size,
                element_shape=element_shape,
            )
            ta = ta.scatter(indices=[0, 1, 2], value=x)
            return ta.read(0), ta.read(1), ta.read(2)

        model, inputs, outputs = build_model
        input_values = [np.array([[3.14], [6.17], [12.14]], dtype=np.float32)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, size_dynamic_shape",
        itertools.product([True, False], backends, [(2, False, (None, 8))]),
    )
    def test_partial_element_shape(self, use_cpu_only, backend, size_dynamic_shape):
        size, dynamic_size, element_shape = size_dynamic_shape

        @make_tf_graph([(3, 1, 8)])
        def build_model(x):
            ta = tf.TensorArray(
                tf.float32,
                size=size,
                dynamic_size=dynamic_size,
                element_shape=element_shape,
            )
            ta = ta.scatter(indices=[0, 1, 2], value=x)
            return ta.read(0), ta.read(1), ta.read(2)

        model, inputs, outputs = build_model
        input_values = [np.random.rand(3, 1, 8).astype(np.float32)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )
