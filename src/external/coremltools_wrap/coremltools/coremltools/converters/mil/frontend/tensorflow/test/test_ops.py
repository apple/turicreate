#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.testing_reqs import *
from coremltools.converters.mil.frontend.tensorflow.test.testing_utils import (
    make_tf_graph,
    run_compare_tf,
    layer_counts,
)
import math

backends = testing_reqs.backends

tf = pytest.importorskip("tensorflow")


class TestPlaceholderAsOutput:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(6)]),
    )
    def test(self, use_cpu_only, backend, rank):
        input_shape = np.random.randint(low=1, high=4, size=rank)

        @make_tf_graph([input_shape, input_shape, input_shape])
        def build_model(x, y, z):
            return x, y, x + 1, x + y

        model, inputs, outputs = build_model
        input_values = [random_gen(input_shape, -1, 1), random_gen(input_shape, -1, 1)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestActivationElu:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(1, 6)]),
    )
    def test(self, use_cpu_only, backend, rank):
        input_shape = np.random.randint(low=1, high=4, size=rank)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.nn.elu(x)

        model, inputs, outputs = build_model

        input_values = [random_gen(input_shape, -1, 1)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestAddN:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, num_inputs",
        itertools.product([True, False], backends, list(range(6)), list(range(1, 5)),),
    )
    def test(self, use_cpu_only, backend, rank, num_inputs):
        if use_cpu_only is False and rank == 5 and num_inputs == 9:
            # <rdar://63680019> Failure on this specific parameter set
            return
        if backend == "mil_proto" and rank == 0:
            return
        input_shape = np.random.randint(low=1, high=4, size=rank)
        input_shapes = [input_shape[:] for _ in range(num_inputs)]

        @make_tf_graph(input_shapes)
        def build_model(*inputs):
            return tf.raw_ops.AddN(inputs=inputs)

        model, inputs, outputs = build_model
        input_values = [random_gen(shape, -1, 1) for shape in input_shapes]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestActivationLeakyReLU:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(1, 6)]),
    )
    def test(self, use_cpu_only, backend, rank):
        input_shape = np.random.randint(low=1, high=4, size=rank)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.nn.leaky_relu(x, 0.2)

        model, inputs, outputs = build_model

        input_values = [random_gen(input_shape, -1, 1)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestActivationReLU:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(1, 6)]),
    )
    def test(self, use_cpu_only, backend, rank):
        input_shape = np.random.randint(low=1, high=4, size=rank)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.nn.relu(x)

        model, inputs, outputs = build_model

        input_values = [random_gen(input_shape, -10.0, 10)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestActivationReLU6:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(1, 6)]),
    )
    def test(self, use_cpu_only, backend, rank):
        input_shape = np.random.randint(low=1, high=4, size=rank)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.nn.relu6(x)

        model, inputs, outputs = build_model

        input_values = [random_gen(input_shape, -1, 1)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestGeluTanhApproximation:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True], backends, [rank for rank in range(2, 3)]),
    )
    def test(self, use_cpu_only, backend, rank):
        input_shape = np.random.randint(low=1, high=4, size=rank)

        @make_tf_graph([input_shape])
        def build_model(x):
            a = 0.5 * (
                1.0 + tf.tanh((math.sqrt(2 / math.pi) * (x + 0.044715 * tf.pow(x, 3))))
            )
            return a * x

        model, inputs, outputs = build_model

        input_values = [random_gen(input_shape, -5, 5)]
        input_dict = dict(zip(inputs, input_values))
        spec = run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )
        assert len(spec.neuralNetwork.layers) == 1
        assert spec.neuralNetwork.layers[0].WhichOneof("layer") == "gelu"


class TestActivationSigmoid:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(1, 6)]),
    )
    def test(self, use_cpu_only, backend, rank):
        input_shape = np.random.randint(low=1, high=4, size=rank)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.math.sigmoid(x)

        model, inputs, outputs = build_model

        input_values = [random_gen(input_shape, -1, 1)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestActivationSoftPlus:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(1, 6)]),
    )
    def test(self, use_cpu_only, backend, rank):
        input_shape = np.random.randint(low=1, high=4, size=rank)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.math.softplus(x)

        model, inputs, outputs = build_model

        input_values = [random_gen(input_shape, -1, 1)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestActivationSoftmax:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank_and_axes",
        itertools.product(
            [True, False],
            backends,
            [(rank, axis) for rank in range(1, 6) for axis in range(-1, rank)],
        ),
    )
    def test(self, use_cpu_only, backend, rank_and_axes):
        rank, axis = rank_and_axes
        input_shape = np.random.randint(low=1, high=4, size=rank)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.nn.softmax(x, axis=axis)

        model, inputs, outputs = build_model

        input_values = [random_gen(input_shape, -1, 1)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestActivationSoftSign:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(1, 6)]),
    )
    def test(self, use_cpu_only, backend, rank):
        input_shape = np.random.randint(low=1, high=4, size=rank)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.math.softsign(x)

        model, inputs, outputs = build_model

        input_values = [random_gen(input_shape, -1, 1)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestActivationSelu:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(1, 6)]),
    )
    def test(self, use_cpu_only, backend, rank):
        input_shape = np.random.randint(low=1, high=4, size=rank)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.nn.selu(x)

        model, inputs, outputs = build_model

        input_values = [random_gen(input_shape, -1.0, 1.0)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestSelect:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, broadcast, dynamic",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(1, 6)],
            [True, False],
            [True, False],
        ),
    )
    def test_select(self, use_cpu_only, backend, rank, broadcast, dynamic):
        shape = np.random.randint(low=1, high=4, size=rank)
        cond_shape = np.array([shape[0]]) if broadcast else shape

        cond_val = np.random.randint(low=0, high=2, size=cond_shape).astype(np.bool)
        a_val = random_gen(shape=shape, rand_min=-1962.0, rand_max=0.0)
        b_val = random_gen(shape=shape, rand_min=0.0, rand_max=1964.0)

        if dynamic:
            cond_shape = [None] * len(cond_shape) + [tf.bool]
            a_shape = [None] * len(shape) + [tf.float32]
            b_shape = [None] * len(shape) + [tf.float32]
        else:
            cond_shape = cond_shape.tolist() + [tf.bool]
            a_shape = shape.tolist() + [tf.float32]
            b_shape = shape.tolist() + [tf.float32]

        @make_tf_graph([cond_shape, a_shape, b_shape])
        def build_model_select(cond, a, b):
            return tf.raw_ops.Select(condition=cond, x=a, y=b)

        model, inputs, outputs = build_model_select
        inputs_dic = dict(zip(inputs, [cond_val, a_val, b_val]))
        run_compare_tf(
            model, inputs_dic, outputs, use_cpu_only=use_cpu_only, backend=backend
        )


class TestWhere:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(1, 6)]),
    )
    def test_where_1_input(self, use_cpu_only, backend, rank):
        with tf.Graph().as_default() as graph:
            shape = np.random.randint(low=1, high=4, size=rank)
            x_val = np.random.randint(low=-1, high=2, size=shape).astype(np.float32)
            x = tf.placeholder(tf.float32, shape=shape)
            run_compare_tf(
                graph,
                {x: x_val},
                tf.where(x),
                use_cpu_only=use_cpu_only,
                backend=backend,
            )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(1, 6)]),
    )
    def test_where(self, use_cpu_only, backend, rank):
        shape = np.random.randint(low=1, high=4, size=rank)
        cond_val = np.random.randint(low=0, high=2, size=shape).astype(np.int32)
        a_val = random_gen(shape=shape, rand_min=-1962.0, rand_max=0.0)
        b_val = random_gen(shape=shape, rand_min=0.0, rand_max=1964.0)
        with tf.Graph().as_default() as graph:
            cond = tf.placeholder(tf.bool, shape=shape)
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=shape)
            ref = tf.where(cond, a, b)
            run_compare_tf(
                graph,
                {cond: cond_val, a: a_val, b: b_val},
                ref,
                use_cpu_only=use_cpu_only,
                backend=backend,
            )


class TestCond:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], ["nn_proto"],)
    )
    def test_cond_naive(self, use_cpu_only, backend):
        @make_tf_graph([(1,), (1,)])
        def build_model(x, y):
            return tf.cond(tf.constant(True), lambda: x + y, lambda: x * y)

        model, inputs, outputs = build_model
        input_values = [
            np.array([1], dtype=np.float32),
            np.array([6], dtype=np.float32),
        ]
        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_cond(self, use_cpu_only, backend):
        @make_tf_graph([(1,), (1,)])
        def build_model(x, y):
            z = tf.multiply(x, y)
            pred = tf.less(tf.math.reduce_mean(x), tf.math.reduce_mean(y))
            return tf.cond(pred, lambda: tf.add(x, z), lambda: tf.square(y))

        model, inputs, outputs = build_model
        input_values = [
            np.array([1], dtype=np.float32),
            np.array([2], dtype=np.float32),
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_cond_multi_returns(self, use_cpu_only, backend):
        @make_tf_graph([(1,), (1,)])
        def build_model(x, y):
            z = tf.multiply(x, y)
            pred = tf.less(tf.math.reduce_mean(x), tf.math.reduce_mean(y))

            def true_fn():
                return tf.add(x, z), tf.math.multiply(x, z)

            def false_fn():
                return tf.square(y), tf.sqrt(z)

            return tf.cond(pred, true_fn, false_fn)

        model, inputs, outputs = build_model
        input_values = [
            np.array([1], dtype=np.float32),
            np.array([2], dtype=np.float32),
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_cond_with_identity(self, use_cpu_only, backend):
        @make_tf_graph([(1,), (1,)])
        def build_model(x, y):
            z = tf.multiply(x, y)
            pred = tf.less(tf.math.reduce_mean(x), tf.math.reduce_mean(y))
            return tf.cond(pred, lambda: z, lambda: tf.square(y))

        model, inputs, outputs = build_model
        input_values = [
            np.array([1], dtype=np.float32),
            np.array([2], dtype=np.float32),
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_cond_multi_returns_with_identity(self, use_cpu_only, backend):
        @make_tf_graph([(1,), (1,)])
        def build_model(x, y):
            z = tf.multiply(x, y)
            pred = tf.less(tf.math.reduce_mean(x), tf.math.reduce_mean(y))

            def true_fn():
                return tf.add(x, z), x

            def false_fn():
                return tf.square(y), z

            return tf.cond(pred, true_fn, false_fn)

        model, inputs, outputs = build_model
        input_values = [
            np.array([1], dtype=np.float32),
            np.array([2], dtype=np.float32),
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_cond_nested_0(self, use_cpu_only, backend):
        @make_tf_graph([(1,), (1,)])
        def build_model(x, y):
            z = tf.multiply(x, y)
            t = tf.less(tf.math.reduce_mean(x), tf.math.reduce_mean(y))
            f = tf.less(tf.math.reduce_mean(z), tf.math.reduce_mean(y))
            inner_cond = tf.cond(
                f, lambda: tf.pow(x, y), lambda: tf.math.subtract(x, y)
            )
            return tf.cond(t, lambda: inner_cond, lambda: tf.square(y))

        model, inputs, outputs = build_model

        input_values = [
            np.array([2], dtype=np.float32),
            np.array([3], dtype=np.float32),
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_cond_nested_1(self, use_cpu_only, backend):
        @make_tf_graph([(1,), (1,)])
        def build_model(x, y):
            z = tf.multiply(x, y)
            t = tf.less(tf.math.reduce_mean(x), tf.math.reduce_mean(y))
            f = tf.less(tf.math.reduce_mean(z), tf.math.reduce_mean(y))
            cond_1 = tf.cond(f, lambda: tf.pow(x, y), lambda: tf.math.subtract(x, y))
            cond_2 = tf.cond(t, lambda: tf.multiply(x, y), lambda: tf.math.mod(x, y))
            cond_3 = tf.cond(f, lambda: tf.math.divide(x, y), lambda: cond_2)
            return tf.cond(t, lambda: cond_1, lambda: cond_3)

        model, inputs, outputs = build_model

        input_values = [
            np.array([2], dtype=np.float32),
            np.array([3], dtype=np.float32),
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )


class TestWhileLoop:

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends))
    def test_while_loop_with_changing_shape(self, use_cpu_only, backend):
        @make_tf_graph([(2,1),(2,1)])
        def build_model(x,y):
            c = lambda i,j: tf.less(tf.shape(j)[1], 5)
            b = lambda i,j: (i, tf.concat([i,j], axis=1))
            return tf.while_loop(c, b, [x,y], shape_invariants=[x.get_shape(), tf.TensorShape([2, None])])

        model, inputs, outputs = build_model
        input_values = [np.array([[1],[2]], dtype=np.float32),np.array([[1],[2]], dtype=np.float32)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(model, input_dict, outputs,
                       use_cpu_only=use_cpu_only,
                       backend=backend)

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_while_loop_0(self, use_cpu_only, backend):
        @make_tf_graph([(1,)])
        def build_model(x):
            c = lambda i: tf.greater(tf.math.reduce_mean(i), 5)
            b = lambda i: i - 1
            return tf.while_loop(c, b, [x])

        model, inputs, outputs = build_model
        input_values = [np.array([10], dtype=np.float32)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_while_loop_1(self, use_cpu_only, backend):
        @make_tf_graph([(1,), (1,)])
        def build_model(x, y):
            c = lambda i, j: tf.greater(tf.math.reduce_mean(i), tf.math.reduce_mean(j))
            b = lambda i, j: (tf.add(i, 1), tf.square(j))
            return tf.while_loop(c, b, [x, y])

        model, inputs, outputs = build_model
        input_values = [
            np.array([1], dtype=np.float32),
            np.array([2], dtype=np.float32),
        ]
        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_while_loop_2(self, use_cpu_only, backend):
        @make_tf_graph([(1,), (1, 2)])
        def build_model(x, y):
            c = lambda i, j: tf.greater(tf.math.reduce_mean(i), 5)
            b = lambda i, j: (i - 3, j * 2)
            return tf.while_loop(c, b, [x, y])

        model, inputs, outputs = build_model
        input_values = [
            np.array([10], dtype=np.float32),
            np.array([[2, 3]], dtype=np.float32),
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_while_loop_3(self, use_cpu_only, backend):
        @make_tf_graph([(1,), (1, 2), (1,)])
        def build_model(x, y, z):
            c = lambda i, j, k: tf.greater(
                tf.math.reduce_mean(i), tf.math.reduce_mean(j)
            )
            b = lambda i, j, k: (i / 3, j ** 2, k - 2)
            return tf.while_loop(c, b, [x, y, z])

        model, inputs, outputs = build_model
        input_values = [
            np.array([10], dtype=np.float32),
            np.array([[2, 3]], dtype=np.float32),
            np.array([5], dtype=np.float32),
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_while_loop_4(self, use_cpu_only, backend):
        @make_tf_graph([(1,), (1, 2), (1,), (2, 1)])
        def build_model(x, y, z, m):
            c = lambda i, j, k, l: tf.greater(
                tf.math.reduce_mean(i), tf.math.reduce_mean(j)
            )
            b = lambda i, j, k, l: (i / 3, j ** 2, k - 2, l % 2)
            return tf.while_loop(c, b, [x, y, z, m])

        model, inputs, outputs = build_model
        input_values = [
            np.array([10], dtype=np.float32),
            np.array([[2, 3]], dtype=np.float32),
            np.array([5], dtype=np.float32),
            np.array([[2], [3]], dtype=np.float32),
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_nested_while_body(self, use_cpu_only, backend):
        @make_tf_graph([(1,), (1,)])
        def build_model(x, y):
            # The following while loop:
            #
            # i, j = 0, 10
            # while i < j:
            #   while 2*i < i+2:
            #     i += 1
            #   i += 2

            def cond2(i):
                return tf.less(2 * tf.math.reduce_mean(i), tf.math.reduce_mean(i + 2))

            def body2(i):
                return i + 1

            def cond1(i, j):
                return tf.less(tf.math.reduce_mean(i), tf.math.reduce_mean(j))

            def body1(i, j):
                new_i = tf.while_loop(cond2, body2, [i])
                return new_i + 2, j

            return tf.while_loop(cond1, body1, [x, y])

        model, inputs, outputs = build_model
        input_values = [
            np.array([0], dtype=np.float32),
            np.array([10], dtype=np.float32),
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_nested_while_cond(self, use_cpu_only, backend):
        @make_tf_graph([(1,), (1,)])
        def build_model(x, y):
            # The following while loop:
            #
            # def cond(i, j):
            #  while 2*i < i+2:
            #    i += 1
            #  return i < j
            #
            # i, j = 0, 10
            # while cond(i, j):
            #   i += 2
            #   j += 1

            def cond2(i):
                return tf.less(2 * tf.math.reduce_mean(i), tf.math.reduce_mean(i + 2))

            def body2(i):
                return i + 1

            def cond1(i, j):
                new_i = tf.while_loop(cond2, body2, [i])
                return tf.less(tf.squeeze(new_i), tf.squeeze(j))

            def body1(i, j):
                return i + 2, j + 1

            return tf.while_loop(cond1, body1, [x, y])

        model, inputs, outputs = build_model
        input_values = [
            np.array([0], dtype=np.float32),
            np.array([10], dtype=np.float32),
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )


class TestConv:
    @pytest.mark.parametrize(
        ",".join(
            [
                "use_cpu_only",
                "backend",
                "conv_dim",  # 1d or 2d conv
                "padding",
                "data_format",
                "HWkHkW",
                "strides",
                "dilations",
                "dynamic_weights",
                "batch_size",
            ]
        ),
        itertools.product(
            [True, False],
            backends,
            ["conv1d", "conv2d"],
            ["SAME", "VALID", [[2, 3], [3, 2]]],
            ["NHWC"],  # NCHW not supported by TF.
            [(11, 12, 3, 2), (12, 11, 2, 3)],
            [(1, 1), (2, 3)],
            [(1, 1), (2, 3)],
            [True, False],
            [1, 3],
        ),
    )
    def test(
        self,
        use_cpu_only,
        backend,
        conv_dim,
        padding,
        data_format,
        HWkHkW,
        strides,
        dilations,
        dynamic_weights,
        batch_size,
    ):
        H, W, kH, kW = HWkHkW
        N, C_in, C_out = batch_size, 2, 3
        if data_format == "NHWC":
            input_shape = (N, W, C_in) if conv_dim == "conv1d" else (N, H, W, C_in)
            if isinstance(padding, list):
                padding = [[0, 0]] + padding + [[0, 0]]
            if conv_dim == "conv1d":
                data_format = "NWC"
                if isinstance(padding, list):
                    # No explicit padding for conv1d in TF
                    return
        else:  # 'NCHW'
            input_shape = (N, C_in, W) if conv_dim == "conv1d" else (N, C_in, H, W)
            if isinstance(padding, list):
                padding = [[0, 0], [0, 0]] + padding
            if conv_dim == "conv1d":
                data_format = "NCW"
                if isinstance(padding, list):
                    # No explicit padding for conv1d in TF
                    return
        W_shape = (kW, C_in, C_out) if conv_dim == "conv1d" else (kH, kW, C_in, C_out)
        dilations = dilations[1] if conv_dim == "conv1d" else dilations

        # We do not support dynamic weight when dilations != 1.
        if dynamic_weights and dilations == (1, 1):

            @make_tf_graph([input_shape, W_shape])
            def build_model_dynamic_weights(x, W):
                if conv_dim == "conv1d":
                    conv = tf.nn.conv1d(
                        x,
                        W,
                        stride=strides[1],
                        padding=padding,
                        dilations=dilations,
                        data_format=data_format,
                    )
                else:
                    conv = tf.nn.conv2d(
                        x,
                        W,
                        strides=strides,
                        padding=padding,
                        dilations=dilations,
                        data_format=data_format,
                    )
                return conv

            model, inputs, outputs = build_model_dynamic_weights
            input_values = [
                random_gen(input_shape, -10.0, 10.0),
                random_gen(W_shape, -1.0, 1.0),
            ]
            input_dict = dict(zip(inputs, input_values))

        else:

            @make_tf_graph([input_shape])
            def build_model_static_weights(x):
                W = tf.constant(np.random.rand(*W_shape), tf.float32)
                if conv_dim == "conv1d":
                    conv = tf.nn.conv1d(
                        x,
                        W,
                        stride=strides[1],
                        padding=padding,
                        dilations=dilations,
                        data_format=data_format,
                    )
                else:
                    conv = tf.nn.conv2d(
                        x,
                        W,
                        strides=strides,
                        padding=padding,
                        dilations=dilations,
                        data_format=data_format,
                    )
                return conv

            model, inputs, outputs = build_model_static_weights
            input_values = [random_gen(input_shape, -10.0, 10.0)]
            input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestConv3d:
    @pytest.mark.parametrize(
        ",".join(
            [
                "use_cpu_only",
                "backend",
                "data_format",
                "input_size",
                "kernel_size",
                "strides",
                "dilations",
                "padding_type",
                "batch_size",
            ]
        ),
        itertools.product(
            [True, False],  # use_cpu_only
            backends,
            ["NDHWC"],  # NCDHW not supported by TF.
            [(7, 11, 13), (32, 16, 8)],  # input_size
            [(1, 1, 1), (3, 3, 3), (1, 2, 3)],  # kernel_size
            [(1, 1, 1), (2, 2, 2), (3, 2, 1)],  # strides
            [
                (1, 1, 1)
            ],  # , (2, 2, 2), (2, 3, 1)],  # dilations: dilations greater than 1 not supported on CPU
            ["SAME", "VALID"],  # padding_type
            [1, 3],  # batch_size
        ),
    )
    def test_tf(
        self,
        use_cpu_only,
        backend,
        data_format,
        input_size,
        kernel_size,
        strides,
        dilations,
        padding_type,
        batch_size,
    ):
        C_in = np.random.randint(low=1, high=4)
        C_out = np.random.randint(low=1, high=(C_in + 1))
        input_shape = [batch_size] + list(input_size) + [C_in]
        weights_shape = list(kernel_size) + [C_in, C_out]
        # TF1 and TF2 tf.nn.conv3d require dilations and strides to have length 5 or greater, with values of 1 for
        # indices 0 and 4 (batch and channel in NDHWC format)
        tf_strides = [1] + list(strides) + [1]
        tf_dilations = [1] + list(dilations) + [1]

        @make_tf_graph([input_shape])
        def build_model_static_weights(x):
            W = tf.constant(np.random.rand(*weights_shape), tf.float32)
            return tf.nn.conv3d(
                x,
                W,
                strides=tf_strides,
                padding=padding_type,
                data_format=data_format,
                dilations=tf_dilations,
            )

        model, inputs, outputs = build_model_static_weights
        input_values = [random_gen(input_shape, -10.0, 10.0)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            backend=backend,
            frontend_only=False,
            atol=1e-03,  # default 1e-04
            rtol=2e-03,  # default 1e-05
        )


class TestDepthwiseConv:
    @pytest.mark.parametrize(
        ",".join(
            [
                "use_cpu_only",
                "backend",
                "padding",
                "HWkHkW",
                "strides",
                "dilations",
                "dynamic_weights",
                "batch_size",
            ]
        ),
        itertools.product(
            [True, False],
            backends,
            ["SAME", "VALID"],
            [(11, 12, 3, 2), (12, 11, 2, 3)],
            # TF doesn't support non-square strides for depthwise
            # https://github.com/tensorflow/tensorflow/issues/33005
            [(1, 1, 1, 1), (1, 2, 2, 1)],
            [
                (1, 1),
                (2, 2),
            ],  # rdar://60668562 (MIL: Conversion for TF op 'SpaceToBatchND' not implemented.)
            [True, False],
            [1, 3],
        ),
    )
    def test_depthwise_conv(
        self,
        use_cpu_only,
        backend,
        padding,
        HWkHkW,
        strides,
        dilations,
        dynamic_weights,
        batch_size,
    ):
        if np.sum(strides) != len(strides) and np.sum(dilations) != len(dilations):
            # TF doesn't compute correct output for non-one stride+dilation
            return
        H, W, kH, kW = HWkHkW
        N, C_in, C_out = batch_size, 2, 6
        input_shape = (N, H, W, C_in)
        data_format = "NHWC"
        assert C_out % C_in == 0
        multiplier = int(C_out / C_in)
        W_shape = (kH, kW, C_in, multiplier)

        def test_static_W():
            W = np.random.rand(*W_shape).astype(np.float32)

            @make_tf_graph([input_shape])
            def build_model_static_weights(x):
                return tf.nn.depthwise_conv2d(
                    x,
                    W,
                    strides=strides,
                    padding=padding,
                    dilations=dilations,
                    data_format=data_format,
                )

            model, inputs, outputs = build_model_static_weights

            input_values = [(np.random.rand(*input_shape).astype(np.float32))]
            input_dict = dict(zip(inputs, input_values))

            proto = run_compare_tf(
                model,
                input_dict,
                outputs,
                use_cpu_only=use_cpu_only,
                backend=backend,
                frontend_only=False,
            )

            assert layer_counts(proto, "reorganizeData") == 0

        def test_dynamic_W():
            @make_tf_graph([input_shape, W_shape])
            def build_model_dynamic_weights(x, W):
                return tf.nn.depthwise_conv2d(
                    x,
                    W,
                    strides=strides,
                    padding=padding,
                    dilations=dilations,
                    data_format=data_format,
                )

            model, inputs, outputs = build_model_dynamic_weights

            input_values = [
                (np.random.rand(*input_shape).astype(np.float32)),
                (np.random.rand(*W_shape).astype(np.float32)),
            ]
            input_dict = dict(zip(inputs, input_values))

            run_compare_tf(
                model,
                input_dict,
                outputs,
                use_cpu_only=use_cpu_only,
                backend=backend,
                frontend_only=False,
            )

        # We do not support dynamic weight when dilations != 1.
        test_dynamic_W() if dynamic_weights and dilations == (1, 1) else test_static_W()


class TestSeparableConv:
    @pytest.mark.parametrize(
        ",".join(
            [
                "use_cpu_only",
                "backend",
                "padding",
                "HWkHkW",
                "strides",
                "dilations",
                "dynamic_weights",
                "batch_size",
            ]
        ),
        itertools.product(
            [True, False],
            backends,
            ["SAME", "VALID"],
            [(11, 12, 3, 2), (12, 11, 2, 3)],
            [(1, 1, 1, 1), (1, 2, 2, 1)],
            [(1, 1), (2, 2)],
            [True, False],
            [1, 3],
        ),
    )
    def test_separable_conv(
        self,
        use_cpu_only,
        backend,
        padding,
        HWkHkW,
        strides,
        dilations,
        dynamic_weights,
        batch_size,
    ):
        H, depthwise_filter, kH, kW = HWkHkW
        N, C_in, C_out = batch_size, 2, 6
        input_shape = (N, H, depthwise_filter, C_in)
        data_format = "NHWC"
        assert C_out % C_in == 0
        multiplier = int(C_out / C_in)
        depthwise_filter_shape = (kH, kW, C_in, multiplier)
        pointwise_filter_shape = [1, 1, multiplier * C_in, C_out]
        if dilations != (1, 1):
            strides = (1, 1, 1, 1)

        def test_dynamic_W():
            @make_tf_graph(
                [input_shape, depthwise_filter_shape, pointwise_filter_shape]
            )
            def build_model_dynamic_weights(x, depthwise_filter, pointwise_filter):
                return tf.nn.separable_conv2d(
                    x,
                    depthwise_filter,
                    pointwise_filter,
                    strides=strides,
                    padding=padding,
                    dilations=dilations,
                    data_format=data_format,
                )

            model, inputs, outputs = build_model_dynamic_weights

            input_values = [
                (np.random.rand(*input_shape).astype(np.float32)),
                (np.random.rand(*depthwise_filter_shape).astype(np.float32)),
                (np.random.rand(*pointwise_filter_shape).astype(np.float32)),
            ]
            input_dict = dict(zip(inputs, input_values))

            run_compare_tf(
                model,
                input_dict,
                outputs,
                use_cpu_only=use_cpu_only,
                backend=backend,
                frontend_only=False,
            )

        def test_static_W():
            depthwise_filter = np.random.rand(*depthwise_filter_shape).astype(
                np.float32
            )
            pointwise_filter = np.random.rand(*pointwise_filter_shape).astype(
                np.float32
            )

            @make_tf_graph([input_shape])
            def build_model_static_weights(x):
                return tf.nn.separable_conv2d(
                    x,
                    depthwise_filter,
                    pointwise_filter,
                    strides=strides,
                    padding=padding,
                    dilations=dilations,
                    data_format=data_format,
                )

            model, inputs, outputs = build_model_static_weights

            input_values = [(np.random.rand(*input_shape).astype(np.float32))]
            input_dict = dict(zip(inputs, input_values))

            run_compare_tf(
                model,
                input_dict,
                outputs,
                use_cpu_only=use_cpu_only,
                backend=backend,
                frontend_only=False,
            )

        test_static_W()
        test_dynamic_W()


class TestConvTranspose:
    @pytest.mark.parametrize(
        ",".join(
            [
                "use_cpu_only",
                "backend",
                "conv_dim",  # 1d or 2d conv
                "padding",
                "data_format",
                "HWkHkW",
                "strides",
                "dilations",
            ]
        ),
        itertools.product(
            [True, False],
            backends,
            ["conv1d", "conv2d"],
            ["SAME", "VALID"],
            ["NHWC"],  # NCHW not supported by TF
            [(12, 12, 2, 2), (2, 2, 2, 3), (5, 5, 3, 3)],
            [(1, 1), (1, 2)],
            [(1, 1)],  # Dilation > 1 not supported by TF
        ),
    )
    def test_conv_transpose(
        self,
        use_cpu_only,
        backend,
        conv_dim,
        padding,
        data_format,
        HWkHkW,
        strides,
        dilations,
    ):
        H, W, kH, kW = HWkHkW
        N, C_in, C_out = 1, 1, 2

        if padding == "SAME":
            oH = H * strides[0]
            oW = W * strides[1]
        else:
            oH = (H - 1) * strides[0] + (kH - 1) * dilations[0] + 1
            oW = (W - 1) * strides[1] + (kW - 1) * dilations[1] + 1

        if data_format == "NHWC":
            input_shape = (N, W, C_in) if conv_dim == "conv1d" else (N, H, W, C_in)
            if conv_dim == "conv1d":
                data_format = "NWC"
            output_shape = (
                [N, oH, C_out] if conv_dim == "conv1d" else [N, oH, oW, C_out]
            )
        else:  # 'NCHW'
            input_shape = (N, C_in, W) if conv_dim == "conv1d" else (N, C_in, H, W)
            if conv_dim == "conv1d":
                data_format = "NCW"
            output_shape = (
                [N, C_out, oH] if conv_dim == "conv1d" else [N, C_out, oH, oW]
            )

        w_shape = (kH, C_out, C_in) if conv_dim == "conv1d" else (kH, kW, C_out, C_in)

        def test_static_W():
            x_input = np.random.randn(*input_shape)
            w_val = np.random.randn(*w_shape)
            with tf.Graph().as_default() as graph:
                x = tf.placeholder(tf.float32, shape=input_shape)
                w = tf.constant(w_val, tf.float32)
                if conv_dim == "conv1d":
                    conv = tf.nn.conv1d_transpose(
                        x,
                        w,
                        output_shape=output_shape,
                        strides=strides[0],
                        padding=padding,
                        dilations=dilations[0],
                        data_format=data_format,
                    )
                else:
                    conv = tf.nn.conv2d_transpose(
                        x,
                        w,
                        output_shape=output_shape,
                        strides=strides,
                        padding=padding,
                        dilations=dilations,
                        data_format=data_format,
                    )
                run_compare_tf(
                    graph,
                    {x: x_input},
                    conv,
                    use_cpu_only=use_cpu_only,
                    frontend_only=False,
                    backend=backend,
                )

        test_static_W()

    @pytest.mark.skip(reason="rdar://65198011 (Re-enable Conv3dTranspose and DynamicTile unit tests)")
    @pytest.mark.parametrize(
        ",".join(
            [
                "use_cpu_only",
                "backend",
                "padding",
                "data_format",
                "DHWkDkHkW",
                "strides",
                "dilations",
            ]
        ),
        itertools.product(
            [True, False],
            backends,
            [
                "SAME"
            ],  # VALID padding requires padding due to different output shape computation
            # and blocked by rdar://63245116 ([deconv3d] Deconv 3d with deconv_out* parameter executes
            # deconvolution kernel instead of deconv3d kernel on CPU)
            ["NDHWC"],
            [
                (10, 12, 14, 2, 3, 5),
                (4, 4, 4, 2, 3, 1),
                (6, 6, 6, 3, 3, 3),
                (5, 5, 5, 2, 4, 2),
            ],
            [(1, 1, 1), (1, 2, 3)],
            [(1, 1, 1)],  # Dilation > 1 not supported by TF
        ),
    )
    def test_conv3d_transpose(
        self, use_cpu_only, backend, padding, data_format, DHWkDkHkW, strides, dilations
    ):
        D, H, W, kD, kH, kW = DHWkDkHkW
        N, C_in, C_out = 2, 1, 2

        if padding == "SAME":
            oD = D * strides[0]
            oH = H * strides[1]
            oW = W * strides[2]
        else:
            oD = (D - 1) * strides[0] + (kD - 1) * dilations[0] + 1
            oH = (H - 1) * strides[1] + (kH - 1) * dilations[1] + 1
            oW = (W - 1) * strides[2] + (kW - 1) * dilations[2] + 1
        if data_format == "NDHWC":
            input_shape = (N, D, H, W, C_in)
            output_shape = [N, oD, oH, oW, C_out]
        else:  # 'NCDHW'
            input_shape = (N, C_in, D, H, W)
            output_shape = [N, C_out, oD, oH, oW]

        w_shape = (kD, kH, kW, C_out, C_in)
        x_input = np.random.randn(*input_shape)
        w_val = np.random.randn(*w_shape)

        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=input_shape)
            w = tf.constant(w_val, tf.float32)
            conv = tf.nn.conv3d_transpose(
                x,
                w,
                output_shape=output_shape,
                strides=strides,
                padding=padding,
                dilations=dilations,
                data_format=data_format,
            )

            run_compare_tf(
                graph,
                {x: x_input},
                conv,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )


class TestElementWiseBinary:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, mode",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(0, 4)],
            [
                "add",
                "floor_div",
                "floor_mod",
                "maximum",
                "minimum",
                "mod",
                "mul",
                "pow",
                "real_div",
                "sub",
                "squared_difference",
            ],
        ),
    )
    def test_binary(self, use_cpu_only, backend, rank, mode):
        # TODO: rdar://problem/63030405. Rank 0 tensor for MIL
        if rank == 0 and backend == "mil_proto":
            return
        x_shape = list(np.random.randint(low=2, high=4, size=rank))
        y_shape = x_shape[:]
        for i in range(rank):
            if np.random.randint(4) == 0:
                y_shape[i] = 1
        if np.random.randint(2) == 0:
            y_shape = [1] + y_shape

        # lower precision input data for non-CPU tests
        dtype = np.float32 if use_cpu_only else np.float16

        if mode == "add":
            res = tf.math.add
            x_val = random_gen(x_shape, -1000, 1000, dtype=dtype).astype(np.float32)
            y_val = random_gen(y_shape, -1000, 1000, dtype=dtype).astype(np.float32)
        elif mode == "floor_div":
            res = tf.math.floordiv
            x_val = random_gen(x_shape, 0, 1000, dtype=dtype).astype(np.float32)
            y_val = random_gen(y_shape, 1, 20, dtype=dtype).astype(np.float32)
        elif mode == "floor_mod":
            res = tf.math.floormod
            x_val = random_gen(x_shape, 0, 100, dtype=dtype).astype(np.float32)
            y_val = random_gen(y_shape, 1, 20, dtype=dtype).astype(np.float32)
        elif mode == "maximum":
            res = tf.math.maximum
            x_val = random_gen(x_shape, -10, 10, dtype=dtype).astype(np.float32)
            y_val = random_gen(y_shape, -10, 10, dtype=dtype).astype(np.float32)
        elif mode == "minimum":
            res = tf.math.minimum
            x_val = random_gen(x_shape, -10, 10, dtype=dtype).astype(np.float32)
            y_val = random_gen(y_shape, -10, 10, dtype=dtype).astype(np.float32)
        elif mode == "mod":
            res = tf.math.mod
            x_val = random_gen(x_shape, 0, 1000, dtype=dtype).astype(np.float32)
            y_val = random_gen(y_shape, 1, 20, dtype=dtype).astype(np.float32)
        elif mode == "mul":
            res = tf.math.multiply
            x_val = random_gen(x_shape, -100, 100, dtype=dtype).astype(np.float32)
            y_val = random_gen(y_shape, -100, 100, dtype=dtype).astype(np.float32)
        elif mode == "pow":
            res = tf.math.pow
            x_val = np.random.randint(low=-5, high=5, size=x_shape).astype(np.float32)
            y_val = np.random.randint(low=-5, high=5, size=y_shape).astype(np.float32)
        elif mode == "real_div":
            res = tf.math.truediv
            x_val = random_gen(x_shape, 0, 1000, dtype=dtype).astype(np.float32)
            y_val = random_gen(y_shape, 1, 20, dtype=dtype).astype(np.float32)
        elif mode == "sub":
            res = tf.math.subtract
            x_val = random_gen(x_shape, -1000, 1000, dtype=dtype).astype(np.float32)
            y_val = random_gen(y_shape, -1000, 1000, dtype=dtype).astype(np.float32)
        elif mode == "squared_difference":
            if backend == "mil_proto":
                return  # TODO
            res = tf.math.squared_difference
            x_val = random_gen(x_shape, -5, 5, dtype=dtype).astype(np.float32)
            y_val = random_gen(y_shape, -5, 5, dtype=dtype).astype(np.float32)

        @make_tf_graph([x_shape, y_shape])
        def build_model(x, y):
            return res(x, y)

        model, inputs, outputs = build_model
        input_values = [x_val, y_val]

        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(0, 4)]),
    )
    def test_equal(self, use_cpu_only, backend, rank):
        if rank == 0 and backend == "mil_proto":
            return
        x_shape = list(np.random.randint(low=2, high=4, size=rank))
        y_shape = x_shape[:]
        for i in range(rank):
            if np.random.randint(4) == 0:
                y_shape[i] = 1
        if np.random.randint(2) == 0:
            y_shape = [1] + y_shape

        # lower precision input data for non-CPU tests
        dtype = np.float32 if use_cpu_only else np.float16

        @make_tf_graph([x_shape, y_shape])
        def build_model(x, y):
            return tf.equal(x, y)

        model, inputs, outputs = build_model

        input_values = [
            random_gen(x_shape, -5, 3, dtype=dtype).astype(np.float32),
            random_gen(y_shape, -5, 3, dtype=dtype).astype(np.float32),
        ]

        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(0, 4)]),
    )
    def test_greater(self, use_cpu_only, backend, rank):
        if rank == 0 and backend == "mil_proto":
            return
        x_shape = list(np.random.randint(low=2, high=4, size=rank))
        y_shape = x_shape[:]
        for i in range(rank):
            if np.random.randint(4) == 0:
                y_shape[i] = 1
        if np.random.randint(2) == 0:
            y_shape = [1] + y_shape

        # lower precision input data for non-CPU tests
        dtype = np.float32 if use_cpu_only else np.float16

        @make_tf_graph([x_shape, y_shape])
        def build_model(x, y):
            return tf.greater(x, y)

        model, inputs, outputs = build_model

        input_values = [
            random_gen(x_shape, -5, 3, dtype=dtype).astype(np.float32),
            random_gen(y_shape, -5, 3, dtype=dtype).astype(np.float32),
        ]

        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(0, 4)]),
    )
    def test_greater_equal(self, use_cpu_only, backend, rank):
        if rank == 0 and backend == "mil_proto":
            return
        x_shape = list(np.random.randint(low=2, high=4, size=rank))
        y_shape = x_shape[:]
        for i in range(rank):
            if np.random.randint(4) == 0:
                y_shape[i] = 1
        if np.random.randint(2) == 0:
            y_shape = [1] + y_shape

        # lower precision input data for non-CPU tests
        dtype = np.float32 if use_cpu_only else np.float16

        @make_tf_graph([x_shape, y_shape])
        def build_model(x, y):
            return tf.greater_equal(x, y)

        model, inputs, outputs = build_model

        input_values = [
            random_gen(x_shape, -5, 3, dtype=dtype).astype(np.float32),
            random_gen(y_shape, -5, 3, dtype=dtype).astype(np.float32),
        ]

        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(0, 4)]),
    )
    def test_less(self, use_cpu_only, backend, rank):
        if rank == 0 and backend == "mil_proto":
            return
        x_shape = list(np.random.randint(low=2, high=4, size=rank))
        y_shape = x_shape[:]
        for i in range(rank):
            if np.random.randint(4) == 0:
                y_shape[i] = 1
        if np.random.randint(2) == 0:
            y_shape = [1] + y_shape

        # lower precision input data for non-CPU tests
        dtype = np.float32 if use_cpu_only else np.float16

        @make_tf_graph([x_shape, y_shape])
        def build_model(x, y):
            return tf.less(x, y)

        model, inputs, outputs = build_model

        input_values = [
            random_gen(x_shape, -5, 3, dtype=dtype).astype(np.float32),
            random_gen(y_shape, -5, 3, dtype=dtype).astype(np.float32),
        ]

        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(0, 4)]),
    )
    def test_less_equal(self, use_cpu_only, backend, rank):
        if rank == 0 and backend == "mil_proto":
            return
        x_shape = list(np.random.randint(low=2, high=4, size=rank))
        y_shape = x_shape[:]
        for i in range(rank):
            if np.random.randint(4) == 0:
                y_shape[i] = 1
        if np.random.randint(2) == 0:
            y_shape = [1] + y_shape

        # lower precision input data for non-CPU tests
        dtype = np.float32 if use_cpu_only else np.float16

        @make_tf_graph([x_shape, y_shape])
        def build_model(x, y):
            return tf.less_equal(x, y)

        model, inputs, outputs = build_model

        input_values = [
            random_gen(x_shape, -5, 3, dtype=dtype).astype(np.float32),
            random_gen(y_shape, -5, 3, dtype=dtype).astype(np.float32),
        ]

        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(0, 4)]),
    )
    def test_not_equal(self, use_cpu_only, backend, rank):
        if rank == 0 and backend == "mil_proto":
            return
        x_shape = list(np.random.randint(low=2, high=4, size=rank))
        y_shape = x_shape[:]
        for i in range(rank):
            if np.random.randint(4) == 0:
                y_shape[i] = 1
        if np.random.randint(2) == 0:
            y_shape = [1] + y_shape

        # lower precision input data for non-CPU tests
        dtype = np.float32 if use_cpu_only else np.float16

        @make_tf_graph([x_shape, y_shape])
        def build_model(x, y):
            return tf.not_equal(x, y)

        model, inputs, outputs = build_model

        input_values = [
            random_gen(x_shape, -5, 3, dtype=dtype).astype(np.float32),
            random_gen(y_shape, -5, 3, dtype=dtype).astype(np.float32),
        ]

        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )


class TestElementWiseUnary:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, mode",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(1, 6)],
            [
                "abs",
                "acos",
                "asin",
                "atan",
                "atanh",
                "cast",
                "ceil",
                "clip",
                "cos",
                "cosh",
                "erf",
                "exp",
                "floor",
                "inverse",
                "log",
                "negative",
                "round",
                "rsqrt",
                "sign",
                "sin",
                "sinh",
                "sqrt",
                "square",
                "tan",
                "tanh",
            ],
        ),
    )
    def test_unary(self, use_cpu_only, backend, rank, mode):
        atol, rtol = 1e-4, 1e-5
        input_shape = np.random.randint(low=2, high=4, size=rank)
        if use_cpu_only:
            dtype = np.float32
        else:
            dtype = np.float16

        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=input_shape)
            if mode == "abs":
                res = tf.abs(x)
                val = random_gen(input_shape, rand_min=-1, rand_max=1)
            elif mode == "acos":
                res = tf.acos(x)
                val = random_gen(input_shape, rand_min=-1, rand_max=1)
            elif mode == "asin":
                res = tf.asin(x)
                val = random_gen(input_shape, rand_min=-1, rand_max=1)
            elif mode == "atan":
                res = tf.atan(x)
                val = random_gen(input_shape, rand_min=-100, rand_max=100)
            elif mode == "atanh":
                if backend == "mil_proto":
                    # TODO
                    return
                res = tf.atanh(x)
                val = random_gen(input_shape, rand_min=-0.9, rand_max=0.9)
            elif mode == "cast":
                if backend == "mil_proto":
                    # TODO <rdar://problem/61400566> [MIL] Add cast operation in MIL backend and enable tests
                    return
                eps_from_int = 0.0
                if not use_cpu_only:
                    eps_from_int = 0.1
                res = tf.cast(x, dtype=tf.int32)
                val = random_gen(
                    input_shape,
                    rand_min=-10,
                    rand_max=10,
                    eps_from_int=eps_from_int,
                    dtype=dtype,
                )
            elif mode == "ceil":
                res = tf.ceil(x)
                eps_from_int = 0.0
                if not use_cpu_only:
                    eps_from_int = 0.1
                val = random_gen(
                    input_shape,
                    rand_min=-100,
                    rand_max=100,
                    eps_from_int=eps_from_int,
                    dtype=dtype,
                )
            elif mode == "clip":
                if backend == "mil_proto":
                    # TODO
                    return
                res = tf.clip_by_value(x, clip_value_min=0.0, clip_value_max=5.0)
                val = random_gen(input_shape, rand_min=-5, rand_max=10)
            elif mode == "cos":
                res = tf.cos(x)
                rand_range = 1000
                if not use_cpu_only:
                    rand_range = 10
                val = random_gen(input_shape, rand_min=-rand_range, rand_max=rand_range)
            elif mode == "cosh":
                res = tf.cosh(x)
                val = random_gen(input_shape, rand_min=-4, rand_max=4)
            elif mode == "erf":
                res = tf.math.erf(x)
                val = random_gen(input_shape, rand_min=1, rand_max=6)
            elif mode == "exp":
                if not use_cpu_only:
                    # We skip GPU here, since exp(1) already differs in backend.
                    return
                res = tf.exp(x)
                val = random_gen(input_shape, rand_min=-4, rand_max=20)
            elif mode == "floor":
                res = tf.floor(x)
                eps_from_int = 0.0
                if not use_cpu_only:
                    eps_from_int = 0.1
                val = random_gen(
                    input_shape,
                    rand_min=-100,
                    rand_max=100,
                    eps_from_int=eps_from_int,
                    dtype=dtype,
                )
            elif mode == "inverse":
                if backend == "mil_proto":
                    return  # TODO
                res = tf.reciprocal(x)
                val = random_gen(input_shape, rand_min=0.1, rand_max=10)
            elif mode == "log":
                res = tf.log(x)
                val = random_gen(input_shape, rand_min=0.2, rand_max=1000)
            elif mode == "negative":
                if backend == "mil_proto":
                    return  # TODO
                res = tf.math.negative(x)
                val = random_gen(input_shape, rand_min=-100.0, rand_max=100.0)
            elif mode == "round":
                res = tf.round(x)
                val = random_gen(
                    input_shape, rand_min=-1000, rand_max=1000, dtype=dtype
                )
            elif mode == "rsqrt":
                res = tf.rsqrt(x)
                val = random_gen(input_shape, rand_min=0.5, rand_max=1000)
            elif mode == "sign":
                res = tf.sign(x)
                val = random_gen(input_shape, rand_min=-5, rand_max=5)
            elif mode == "sin":
                res = tf.sin(x)
                rand_range = 1000
                if not use_cpu_only:
                    rand_range = 10
                val = random_gen(input_shape, rand_min=-rand_range, rand_max=rand_range)
            elif mode == "sinh":
                res = tf.sinh(x)
                val = random_gen(input_shape, rand_min=-10, rand_max=10)
            elif mode == "sqrt":
                res = tf.sqrt(x)
                val = random_gen(input_shape, rand_min=0.5, rand_max=1000)
            elif mode == "square":
                if backend == "mil_proto":
                    return  # TODO
                res = tf.math.square(x)
                val = random_gen(input_shape, rand_min=-5, rand_max=5)
                atol, rtol = 1e-2, 1e-3
            elif mode == "tan":
                res = tf.tan(x)
                val = random_gen(input_shape, rand_min=-1000, rand_max=1000)
            elif mode == "tanh":
                res = tf.tanh(x)
                val = random_gen(input_shape, rand_min=-1000, rand_max=1000)

            run_compare_tf(
                graph,
                {x: val},
                res,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
                atol=atol,
                rtol=rtol,
            )


class TestImageResizing:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, input_shape, target_shape, align_corners, half_pixel_centers",
        itertools.product(
            [True, False],
            backends,
            [(1, 10, 20, 1), (2, 5, 1, 3)],
            [(25, 30), (2, 20)],
            [True, False],
            [True, False],
        ),
    )
    def test_resize_bilinear(
        self,
        use_cpu_only,
        backend,
        input_shape,
        target_shape,
        align_corners,
        half_pixel_centers,
    ):
        if half_pixel_centers and align_corners:
            return
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=input_shape)
            ref = tf.raw_ops.ResizeBilinear(
                images=x,
                size=target_shape,
                half_pixel_centers=half_pixel_centers,
                align_corners=align_corners,
            )
            run_compare_tf(
                graph,
                {x: random_gen(input_shape, rand_min=-100, rand_max=100)},
                ref,
                use_cpu_only=use_cpu_only,
                backend=backend,
            )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, input_shape, upsample_factor, data_format",
        itertools.product(
            [True, False],
            backends,
            [(1, 1, 1, 3), (1, 10, 5, 3)],
            [(1, 2), (4, 3)],
            ["channels_last", "channels_first"],
        ),
    )
    def test_upsampling_2d(
        self, use_cpu_only, backend, input_shape, upsample_factor, data_format
    ):
        if data_format == "channels_last":
            input_shape = (
                input_shape[0],
                input_shape[2],
                input_shape[3],
                input_shape[1],
            )
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=input_shape)
            ref = tf.keras.layers.UpSampling2D(
                size=upsample_factor, data_format=data_format, interpolation="nearest"
            )(x)
            run_compare_tf(
                graph,
                {x: random_gen(input_shape, rand_min=-100, rand_max=100)},
                ref,
                use_cpu_only=use_cpu_only,
                backend=backend,
            )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, input_shape, num_of_crops, crop_size, method, dynamic",
        itertools.product(
            [True, False],
            backends,
            [(1, 64, 64, 1)],
            [1, 3, 5],
            [(2, 2), (1, 1), (4, 4), (128, 128)],
            ["bilinear"],
            [False, True],
        ),
    )
    def test_crop_and_resize(
        self,
        use_cpu_only,
        backend,
        input_shape,
        num_of_crops,
        crop_size,
        method,
        dynamic,
    ):
        input = np.random.randn(*input_shape)
        boxes = np.random.uniform(size=(num_of_crops, 4))
        box_indices = np.random.randint(
            size=(num_of_crops,), low=0, high=input_shape[0]
        )

        def test_static():
            with tf.Graph().as_default() as graph:
                x = tf.placeholder(tf.float32, shape=input_shape)
                output = tf.raw_ops.CropAndResize(
                    image=x,
                    boxes=boxes,
                    box_ind=box_indices,
                    crop_size=crop_size,
                    method=method,
                )
                run_compare_tf(
                    graph,
                    {x: input},
                    output,
                    use_cpu_only=use_cpu_only,
                    backend=backend,
                )

        def test_dynamic():
            with tf.Graph().as_default() as graph:
                x = tf.placeholder(tf.float32, shape=input_shape)
                boxes_pl = tf.placeholder(tf.float32, shape=boxes.shape)
                box_indices_pl = tf.placeholder(tf.int32, shape=box_indices.shape)
                output = tf.raw_ops.CropAndResize(
                    image=x,
                    boxes=boxes_pl,
                    box_ind=box_indices_pl,
                    crop_size=crop_size,
                    method=method,
                )
                run_compare_tf(
                    graph,
                    {x: input, boxes_pl: boxes, box_indices_pl: box_indices},
                    output,
                    use_cpu_only=use_cpu_only,
                    backend=backend,
                )

        test_dynamic() if dynamic else test_static()

    @pytest.mark.parametrize(
        "use_cpu_only, backend, width, height, strides, sizes, padding,",
        list(
            itertools.product(
                [True, False],
                backends,
                [1, 3, 5, 50],
                [1, 2, 7, 30],
                [(1, 1), (2, 1), (3, 5), (7, 13)],
                [(1, 1), (1, 2), (5, 3), (13, 7)],
                ["VALID", "SAME"],
            )
        ),
    )
    def test_extract_patches(
        self, use_cpu_only, backend, width, height, strides, sizes, padding
    ):
        # TODO: theoritically, the current extractpatches code handle batch size rather than 1,
        # but there seems to have a bug in crop_resize when using GPU and batch_size > 1.
        # We should test batch_size > 1 after the issue is fixed.
        # <rdar://problem/61602238>
        input = np.random.rand(1, height, width, 128)
        if padding == "VALID":
            size_h = min(sizes[0], height)
            size_w = min(sizes[1], width)
        else:
            size_h = sizes[0]
            size_w = sizes[1]
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=input.shape)
            output = tf.extract_image_patches(
                images=x,
                ksizes=[1, size_h, size_w, 1],
                strides=[1, strides[0], strides[1], 1],
                rates=[1, 1, 1, 1],
                padding=padding,
            )
            run_compare_tf(
                graph, {x: input}, output, use_cpu_only=use_cpu_only, backend=backend
            )


class TestLinear:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, dim, transpose_a, transpose_b, use_constant",
        itertools.product(
            [True, False],
            backends,
            [2, 4, 8],
            [True, False],
            [True, False],
            [True, False],
        ),
    )
    def test_matmul(
        self, use_cpu_only, backend, dim, transpose_a, transpose_b, use_constant
    ):
        shape_x = np.array([dim, dim * 2, dim * 4])
        shape_y = np.array([dim * 4, dim * 2])

        flip = (not transpose_a and transpose_b) or (transpose_a and not transpose_b)
        shape_y = np.flip(shape_y) if flip else shape_y

        if not use_constant:

            @make_tf_graph([shape_x, shape_y])
            def build_model(x, y):
                return tf.linalg.matmul(
                    x, y, transpose_a=transpose_a, transpose_b=transpose_b
                )

            input_values = [
                random_gen(shape=shape_x, rand_min=-100, rand_max=100),
                random_gen(shape=shape_y, rand_min=-1.0, rand_max=1.0),
            ]
        else:
            y = random_gen(shape=shape_y, rand_min=-1.0, rand_max=1.0)

            @make_tf_graph([shape_x])
            def build_model(x):
                return tf.linalg.matmul(
                    x, y, transpose_a=transpose_a, transpose_b=transpose_b
                )

            input_values = [random_gen(shape=shape_x, rand_min=-100, rand_max=100)]

        model, inputs, outputs = build_model

        input_dict = dict(zip(inputs, input_values))

        proto = run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

        for layer in proto.neuralNetwork.layers:
            if layer.WhichOneof("layer") == "batchedMatmul":
                wp = layer.batchedMatmul.weights
                if use_constant:
                    assert len(wp.floatValue) != 0
                else:
                    assert len(wp.floatValue) == 0


class TestNormalization:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, shape_mode, epsilon",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(3, 6)],
            [True, False],
            [1e-1, 1e-10],
        ),
    )
    def test_batch_norm(self, use_cpu_only, backend, rank, shape_mode, epsilon):
        input_shape = np.random.randint(low=1, high=4, size=rank)
        if shape_mode:
            # same shape with 1 for being normalized over
            attr_shape = list(input_shape)
            attr_shape[1] = 1
            attr_shape[2] = 1
        else:
            # 1D tensor of the same size as channel dimension
            attr_shape = [list(input_shape)[-1]]

        @make_tf_graph([input_shape, attr_shape, attr_shape, attr_shape, attr_shape])
        def build_model(x, m, v, o, s):
            return tf.nn.batch_normalization(
                x, mean=m, variance=v, offset=o, scale=s, variance_epsilon=epsilon
            )

        model, inputs, outputs = build_model

        input_values = [
            random_gen(shape=input_shape, rand_min=-100.0, rand_max=100.0),
            random_gen(shape=attr_shape, rand_min=-1.0, rand_max=1.0),
            random_gen(shape=attr_shape, rand_min=0.0, rand_max=10.0),
            random_gen(shape=attr_shape, rand_min=1.0, rand_max=10.0),
            random_gen(shape=attr_shape, rand_min=-1.0, rand_max=1.0),
        ]
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

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, shape_mode, epsilon, scale_after_normalization",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(3, 6)],
            [True, False],
            [1e-1, 1e-10],
            [True, False],
        ),
    )
    def test_batch_norm_with_global_normalization(
        self,
        use_cpu_only,
        backend,
        rank,
        shape_mode,
        epsilon,
        scale_after_normalization,
    ):
        input_shape = np.random.randint(low=1, high=4, size=rank)
        if shape_mode:
            # same shape with 1 for being normalized over
            attr_shape = list(input_shape)
            attr_shape[1] = 1
            attr_shape[2] = 1
        else:
            # 1D tensor of the same size as channel dimension
            attr_shape = [list(input_shape)[-1]]

        if scale_after_normalization:

            @make_tf_graph(
                [input_shape, attr_shape, attr_shape, attr_shape, attr_shape]
            )
            def build_model(x, m, v, b, g):
                return tf.nn.batch_norm_with_global_normalization(
                    x,
                    mean=m,
                    variance=v,
                    beta=b,
                    gamma=g,
                    variance_epsilon=epsilon,
                    scale_after_normalization=scale_after_normalization,
                )

        else:

            @make_tf_graph([input_shape, attr_shape, attr_shape, attr_shape])
            def build_model(x, m, v, b):
                return tf.nn.batch_norm_with_global_normalization(
                    x,
                    mean=m,
                    variance=v,
                    beta=b,
                    gamma=None,
                    variance_epsilon=epsilon,
                    scale_after_normalization=scale_after_normalization,
                )

        model, inputs, outputs = build_model

        input_values = [
            random_gen(shape=input_shape, rand_min=-100.0, rand_max=100.0),
            random_gen(shape=attr_shape, rand_min=-1.0, rand_max=1.0),
            random_gen(shape=attr_shape, rand_min=0.0, rand_max=10.0),
            random_gen(shape=attr_shape, rand_min=1.0, rand_max=10.0),
        ]
        if scale_after_normalization:
            input_values.append(
                random_gen(shape=attr_shape, rand_min=-1.0, rand_max=1.0)
            )

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

    @pytest.mark.parametrize(
        "use_cpu_only, backend, epsilon",
        itertools.product([True, False], backends, [1e-1, 1e-10]),
    )
    def test_fused_batch_norm(self, use_cpu_only, backend, epsilon):
        # TensorFlow's FusedBatchNorm is only for 4D inputs
        input_shape = np.random.randint(low=1, high=4, size=4)
        attr_shape = [list(input_shape)[-1]]

        m = random_gen(shape=attr_shape, rand_min=-1.0, rand_max=1.0)
        v = random_gen(shape=attr_shape, rand_min=0.0, rand_max=10.0)
        o = random_gen(shape=attr_shape, rand_min=1.0, rand_max=10.0)
        s = random_gen(shape=attr_shape, rand_min=-1.0, rand_max=1.0)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.compat.v1.nn.fused_batch_norm(
                x,
                mean=m,
                variance=v,
                offset=o,
                scale=s,
                epsilon=epsilon,
                is_training=False,
            )[0]

        model, inputs, outputs = build_model

        input_values = [random_gen(shape=input_shape, rand_min=-100.0, rand_max=100.0)]

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

    @pytest.mark.skip(reason="<rdar://63680019> Specific failure on CI")
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, axes, epsilon",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(3, 6)],
            [(-1,), (-2,), (0, 1)],
            [1e-5, 1e-10],
        ),
    )
    def test_l2_normalize(self, use_cpu_only, backend, rank, axes, epsilon):
        input_shape = np.random.randint(low=1, high=4, size=rank)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.math.l2_normalize(x, axis=axes, epsilon=epsilon)

        model, inputs, outputs = build_model

        input_values = [random_gen(input_shape, rand_min=-10, rand_max=10)]

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

    @pytest.mark.parametrize(
        "use_cpu_only, backend, size, alpha, beta, k",
        itertools.product(
            [True, False], backends, [1, 2, 3], [0.0001, 0.01], [0.75, 1.0], [1.0, 2.0],
        ),
    )
    def test_local_response_normalization(
        self, use_cpu_only, backend, size, alpha, beta, k
    ):
        # TensorFlow's local_response_normalization only supports rank 4
        input_shape = np.random.randint(low=3, high=4, size=4)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.nn.local_response_normalization(
                x, depth_radius=size, bias=k, alpha=alpha, beta=beta
            )

        model, inputs, outputs = build_model

        input_values = [random_gen(shape=input_shape, rand_min=-100, rand_max=100)]

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


class TestPooling1d:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, kernel_sizes, strides, pad_type",
        itertools.product(
            [True, False], backends, [(1,)], [(1,), (2,)], ["same", "valid"]
        ),
    )
    def test_avg_pool_1d(self, use_cpu_only, backend, kernel_sizes, strides, pad_type):
        input_shape = np.random.randint(low=2, high=4, size=3)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.nn.avg_pool1d(
                x, ksize=kernel_sizes[:], strides=strides[:], padding=pad_type.upper()
            )

        model, inputs, outputs = build_model

        input_values = [random_gen(shape=input_shape, rand_min=-100, rand_max=100)]

        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, kernel_sizes, strides, pad_type",
        itertools.product(
            [True, False], backends, [(1,)], [(1,), (2,)], ["same", "valid"]
        ),
    )
    def test_max_pool_1d(self, use_cpu_only, backend, kernel_sizes, strides, pad_type):
        input_shape = np.random.randint(low=2, high=4, size=3)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.nn.max_pool1d(
                x, ksize=kernel_sizes[:], strides=strides[:], padding=pad_type.upper()
            )

        model, inputs, outputs = build_model

        input_values = [random_gen(shape=input_shape, rand_min=-100, rand_max=100)]

        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )


class TestPooling2d:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, kernel_sizes, strides, pad_type",
        itertools.product(
            [True, False],
            backends,
            [(1,), (2,), (1, 1), (1, 2), (2, 2)],
            [(1,), (2,), (1, 1), (1, 2), (2, 2)],
            ["same", "valid"],
        ),
    )
    def test_avg_pool_2d(self, use_cpu_only, backend, kernel_sizes, strides, pad_type):
        input_shape = np.random.randint(low=2, high=4, size=4)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.nn.avg_pool(
                x, ksize=kernel_sizes[:], strides=strides[:], padding=pad_type.upper()
            )

        model, inputs, outputs = build_model

        input_values = [random_gen(shape=input_shape, rand_min=-100, rand_max=100)]

        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, kernel_sizes, strides, pad_type",
        itertools.product(
            [True, False],
            backends,
            [(1,), (2,), (1, 1), (1, 2), (2, 2)],
            [(1,), (2,), (1, 1), (1, 2), (2, 2)],
            ["same", "valid"],
        ),
    )
    def test_max_pool_2d(self, use_cpu_only, backend, kernel_sizes, strides, pad_type):
        input_shape = np.random.randint(low=2, high=4, size=4)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.nn.max_pool(
                x, ksize=kernel_sizes[:], strides=strides[:], padding=pad_type.upper()
            )

        model, inputs, outputs = build_model

        input_values = [random_gen(shape=input_shape, rand_min=-100, rand_max=100)]

        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )


class TestPooling3d:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, kernel_sizes, strides, pad_type",
        itertools.product(
            [True, False],
            backends,
            [(1,), (2,), (1, 1, 1), (1, 2, 3), (2, 2, 3), (3, 3, 3)],
            [(1,), (2,), (1, 1, 1), (1, 2, 3), (2, 2, 3), (3, 3, 3)],
            ["same", "valid"],
        ),
    )
    def test_avg_pool_3d(self, use_cpu_only, backend, kernel_sizes, strides, pad_type):
        input_shape = np.random.randint(low=3, high=4, size=5)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.nn.avg_pool3d(
                x, ksize=kernel_sizes[:], strides=strides[:], padding=pad_type.upper()
            )

        model, inputs, outputs = build_model

        input_values = [random_gen(shape=input_shape, rand_min=-100, rand_max=100)]

        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, kernel_sizes, strides, pad_type",
        itertools.product(
            [True, False],
            backends,
            [(1,), (2,), (1, 1, 1), (1, 2, 3), (2, 2, 3), (3, 3, 3)],
            [(1,), (2,), (1, 1, 1), (1, 2, 3), (2, 2, 3), (3, 3, 3)],
            ["same", "valid"],
        ),
    )
    def test_max_pool_3d(self, use_cpu_only, backend, kernel_sizes, strides, pad_type):
        input_shape = np.random.randint(low=3, high=4, size=5)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.nn.max_pool3d(
                x, ksize=kernel_sizes[:], strides=strides[:], padding=pad_type.upper()
            )

        model, inputs, outputs = build_model

        input_values = [random_gen(shape=input_shape, rand_min=-100, rand_max=100)]

        input_dict = dict(zip(inputs, input_values))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )


class TestPrint:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [size for size in range(1, 5)],),
    )
    def test_print(self, use_cpu_only, backend, rank):
        shape = np.random.randint(low=1, high=4, size=rank).astype(np.int32)

        @make_tf_graph([shape])
        def build_model(x):
            print_layer = tf.raw_ops.Print(input=x, data=[])
            res = print_layer + 1
            return res

        model, inputs, outputs = build_model
        input_value = [random_gen(shape=shape, rand_min=-100, rand_max=100)]
        input_dict = dict(zip(inputs, input_value))

        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )


class TestRandom:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, size, rank, constant",
        itertools.product(
            [True, False],
            backends,
            [size for size in range(1, 5)],
            [rank for rank in range(1, 6)],
            [True, False],
        ),
    )
    def test_random_binomial(self, use_cpu_only, backend, size, rank, constant):
        if not constant and backend == "mil_proto":
            return  # TODO: rdar://61948178 (MIL backend Random op does not support dynamic input shape)

        shape = np.random.randint(low=1, high=4, size=rank).astype(np.int32)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=shape)
            if constant:
                ref = tf.add(x, tf.keras.backend.random_binomial(shape=shape, p=1.0))
            else:
                ref = tf.add(
                    x,
                    tf.keras.backend.random_binomial(
                        shape=tf.raw_ops.Shape(input=x), p=1.0
                    ),
                )
            run_compare_tf(
                graph,
                {x: np.random.rand(*shape)},
                ref,
                use_cpu_only=use_cpu_only,
                backend=backend,
            )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, size",
        itertools.product([True, False], backends, [size for size in range(1, 5)]),
    )
    def test_random_categorical(self, use_cpu_only, backend, size):
        # TensorFlow's input is 2-D tensor with shape [batch_size, num_classes].
        shape = np.random.randint(low=1, high=4, size=2)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=shape)
            ref = tf.random.categorical(x, size)
            run_compare_tf(
                graph,
                {x: np.random.rand(*shape)},
                ref,
                use_cpu_only=use_cpu_only,
                validate_shapes_only=True,
                backend=backend,
            )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, mean, rank, constant",
        itertools.product(
            [True, False],
            backends,
            [0.0],
            [rank for rank in range(1, 6)],
            [True, False],
        ),
    )
    def test_random_normal(self, use_cpu_only, backend, mean, rank, constant):
        if not constant and backend == "mil_proto":
            return  # TODO: rdar://61948178 (MIL backend Random op does not support dynamic input shape)

        shape = np.random.randint(low=1, high=4, size=rank).astype(np.int32)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=shape)
            if constant:
                ref = tf.add(x, tf.random.normal(shape=shape, mean=mean, stddev=0.0))
            else:
                ref = tf.add(
                    x,
                    tf.random.normal(
                        shape=tf.raw_ops.Shape(input=x), mean=mean, stddev=0.0
                    ),
                )
            run_compare_tf(
                graph,
                {x: np.random.rand(*shape)},
                ref,
                use_cpu_only=use_cpu_only,
                backend=backend,
            )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, mean, rank, constant",
        itertools.product(
            [True, False],
            backends,
            [0.0],
            [rank for rank in range(1, 6)],
            [True, False],
        ),
    )
    def test_keras_random_normal(self, use_cpu_only, backend, mean, rank, constant):
        if not constant and backend == "mil_proto":
            return  # TODO: rdar://61948178 (MIL backend Random op does not support dynamic input shape)

        shape = np.random.randint(low=1, high=4, size=rank).astype(np.int32)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=shape)
            if constant:
                ref = tf.add(
                    x,
                    tf.keras.backend.random_normal(shape=shape, mean=mean, stddev=0.0),
                )
            else:
                ref = tf.add(
                    x,
                    tf.keras.backend.random_normal(
                        shape=tf.raw_ops.Shape(input=x), mean=mean, stddev=0.0
                    ),
                )
            run_compare_tf(
                graph,
                {x: np.random.rand(*shape)},
                ref,
                use_cpu_only=use_cpu_only,
                backend=backend,
            )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, low, high, rank, constant",
        itertools.product(
            [True, False],
            backends,
            [0.0],
            [0.0],
            [rank for rank in range(1, 2)],
            [True, False],
        ),
    )
    def test_random_uniform(self, use_cpu_only, backend, low, high, rank, constant):
        if not constant and backend == "mil_proto":
            return  # TODO: rdar://61948178 (MIL backend Random op does not support dynamic input shape)

        shape = np.random.randint(low=1, high=4, size=rank).astype(np.int32)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=shape)
            if constant:
                ref = tf.add(x, tf.random.uniform(shape=shape, minval=low, maxval=high))
            else:
                ref = tf.add(
                    x,
                    tf.random.uniform(
                        shape=tf.raw_ops.Shape(input=x), minval=low, maxval=high
                    ),
                )
            run_compare_tf(
                graph,
                {x: np.random.rand(*shape)},
                ref,
                use_cpu_only=use_cpu_only,
                backend=backend,
            )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, low, high, rank, constant",
        itertools.product(
            [True, False],
            backends,
            [1.0],
            [1.0],
            [rank for rank in range(1, 6)],
            [True, False],
        ),
    )
    def test_keras_random_uniform(
        self, use_cpu_only, backend, low, high, rank, constant
    ):
        if not constant and backend == "mil_proto":
            return  # TODO: rdar://61948178 (MIL backend Random op does not support dynamic input shape)
        shape = np.random.randint(low=1, high=4, size=rank).astype(np.int32)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=shape)
            if constant:
                ref = tf.add(
                    x,
                    tf.keras.backend.random_uniform(
                        shape=shape, minval=low, maxval=high
                    ),
                )
            else:
                ref = tf.add(
                    x,
                    tf.keras.backend.random_uniform(
                        shape=tf.raw_ops.Shape(input=x), minval=low, maxval=high
                    ),
                )
            run_compare_tf(
                graph,
                {x: np.random.rand(*shape)},
                ref,
                use_cpu_only=use_cpu_only,
                backend=backend,
            )


class TestReduction:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank_and_axes, keep_dims, tf_op",
        itertools.product(
            [True, False],
            backends,
            [
                (1, (-1,)),
                (2, (0,)),
                (2, (-1, 0)),
                (3, (1, -3)),
                (3, (-2,)),
                (4, (0, 1, 2)),
                (4, (-2, -1, 0)),
                (4, (1, -2)),
                (5, (-3, -1)),
                (5, (0, -1, 1, -2)),
                (3, None),
                (5, None),
                (3, 1),
                (5, -1),
            ],
            [True, False],
            [
                tf.reduce_all,
                tf.math.reduce_euclidean_norm,
                tf.reduce_max,
                tf.reduce_mean,
                tf.reduce_min,
                tf.reduce_prod,
                tf.reduce_sum,
                tf.reduce_any,
                tf.reduce_logsumexp,
                tf.math.argmax,
                tf.math.argmin,
            ],
        ),
    )
    def test_reduction(self, use_cpu_only, backend, rank_and_axes, keep_dims, tf_op):
        rank, axes = rank_and_axes
        shape = np.random.randint(low=1, high=4, size=rank)

        def parse_axes(axes):
            if axes is None:
                axes = 0
            elif isinstance(axes, (tuple, list)):
                axes = axes[0]
            return axes

        def test_tf_argmax():
            with tf.Graph().as_default() as graph:
                x = tf.placeholder(tf.float32, shape=shape)
                ref = tf.math.argmax(x, axis=parse_axes(axes))
                run_compare_tf(
                    graph,
                    {x: random_gen(shape=shape, rand_min=-5.0, rand_max=5.0)},
                    ref,
                    use_cpu_only=use_cpu_only,
                    backend=backend,
                )

        def test_tf_argmin():
            with tf.Graph().as_default() as graph:
                x = tf.placeholder(tf.float32, shape=shape)
                ref = tf.math.argmin(x, axis=parse_axes(axes))
                run_compare_tf(
                    graph,
                    {x: random_gen(shape=shape, rand_min=-5.0, rand_max=5.0)},
                    ref,
                    use_cpu_only=use_cpu_only,
                    backend=backend,
                )

        def test_tf_reduction():
            if isinstance(axes, list) and axes and len(axes) == rank and not keep_dims:
                return  # TODO <rdar://problem/59152311> MIL: Add rank 0 and dim size 0 related tests for every op

            with tf.Graph().as_default() as graph:
                x = tf.placeholder(tf.float32, shape=shape)
                x_val = random_gen(shape=shape, rand_min=-5.0, rand_max=5.0)
                if tf_op in {tf.reduce_all, tf.reduce_any}:
                    x = tf.placeholder(tf.bool, shape=shape)
                    x_val = np.random.randint(low=0, high=2, size=shape).astype(
                        np.float32
                    )
                elif tf_op in {tf.math.reduce_euclidean_norm}:
                    x_val = random_gen(shape=shape, rand_min=0.0, rand_max=10.0)
                elif tf_op in {tf.reduce_prod}:
                    x_val = random_gen(shape=shape, rand_min=1.0, rand_max=1.5)
                elif tf_op in {tf.reduce_logsumexp}:
                    x_val = random_gen(shape=shape, rand_min=-5, rand_max=5)
                ref = tf_op(x, axis=axes, keepdims=keep_dims)

                if tf_op == tf.reduce_any:
                    ref = tf.cast(ref, tf.float32)

                run_compare_tf(
                    graph, {x: x_val}, ref, use_cpu_only=use_cpu_only, backend=backend
                )

        if tf_op in {tf.math.argmax}:
            test_tf_argmax()
        elif tf_op in {tf.math.argmin}:
            test_tf_argmin()
        else:
            test_tf_reduction()


class TestScatterGather:
    # TODO: <rdar://problem/59738824> [MIL] Gather layer with 0-d indices leads to input shape mismatch
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rankX_rankIndices_axis, mode",
        itertools.product(
            [True, False],
            backends,
            [
                (1, 2, -1),
                (2, 1, 0),
                (3, 2, -2),
                (2, 3, 1),
                (2, 2, 1),
                (1, 1, 0),
                (3, 3, -2),
                (3, 3, 2),
                (3, 3, 0),
                (1, 3, -1),
                (3, 1, 2),
                (3, 1, -1),
            ],
            ["Gather", "GatherV2", "gather"],
        ),
    )
    def test_gather_function(self, use_cpu_only, backend, rankX_rankIndices_axis, mode):
        x_rank, indices_rank, axis = rankX_rankIndices_axis
        x_shape = np.random.randint(low=2, high=4, size=x_rank)
        indices_shape = np.random.randint(low=2, high=4, size=indices_rank)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=x_shape)
            indices = tf.placeholder(tf.int32, shape=indices_shape)
            if mode == "Gather":
                res = tf.raw_ops.Gather(params=x, indices=indices)
                axis = 0
            elif mode == "GatherV2":
                res = tf.raw_ops.GatherV2(params=x, indices=indices, axis=axis)
            elif mode == "gather":
                res = tf.gather(x, indices, axis=axis)
            run_compare_tf(
                graph,
                {
                    x: np.random.rand(*x_shape),
                    indices: np.random.randint(
                        0, x_shape[axis], size=indices_shape, dtype=np.int32
                    ),
                },
                res,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rankX_rankIndices",
        itertools.product(
            [True, False],
            backends,
            [
                (1, 2),
                (2, 2),
                (3, 2),
                (2, 3),
                (1, 4),
                (5, 2),
                (2, 5),
                (4, 3),
                (3, 4),
                (2, 4),
                (4, 2),
                (1, 5),
            ],
        ),
    )
    def test_gather_nd(self, use_cpu_only, backend, rankX_rankIndices):
        x_rank, indices_rank = rankX_rankIndices
        x_shape = np.random.randint(low=2, high=4, size=x_rank)
        indices_shape = np.random.randint(low=2, high=4, size=indices_rank)
        indices_shape[-1] = np.random.randint(low=1, high=x_rank + 1)

        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=x_shape)
            indices = tf.placeholder(tf.int32, shape=indices_shape)
            res = tf.gather_nd(x, indices)

            a = np.random.rand(*x_shape)
            indices_list = []
            for i in range(indices_shape[-1]):
                indices_list.append(
                    np.random.randint(0, x_shape[i], size=indices_shape[:-1])
                )

            input_values = {
                x: a,
                indices: np.stack(indices_list, axis=-1).astype(np.float),
            }

            run_compare_tf(
                graph,
                input_values,
                res,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, data_rank, indices_rank",
        itertools.product(
            [True, False], backends, list(range(1, 4)), list(range(2, 4)),
        ),
    )
    def test_scatter_nd_with_zeros(
        self, use_cpu_only, backend, data_rank, indices_rank
    ):

        shape = np.random.randint(low=2, high=4, size=data_rank)
        indices_shape = np.random.randint(low=2, high=4, size=indices_rank)
        indices_shape[-1] = np.random.randint(low=1, high=data_rank + 1)
        updates_shape = list(indices_shape[:-1]) + list(shape[indices_shape[-1] :])

        updates = np.random.rand(*updates_shape)
        indices_list = []
        for i in range(indices_shape[-1]):
            indices_list.append(np.random.randint(0, shape[i], size=indices_shape[:-1]))

        indices = np.stack(indices_list, axis=-1).astype(np.int32)

        @make_tf_graph(
            [list(indices.shape) + [tf.int32], updates_shape, [data_rank, tf.int32]]
        )
        def build_model(indices, updates, shape):
            return tf.raw_ops.ScatterNd(indices=indices, updates=updates, shape=shape)

        model, inputs, outputs = build_model
        input_values = [indices, updates, shape]

        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestSlice:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, masking",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(1, 5)],
            [True, False]
        ),
    )
    def test_slice_by_index(self, use_cpu_only, backend, rank, masking):
        input_shape = np.random.randint(low=2, high=4, size=rank)
        begin_val = np.array(
            [
                np.random.randint(low=-input_shape[i], high=input_shape[i])
                for i in range(rank)
            ]
        ).astype(np.int32)
        end_val = np.array(
            [
                np.random.randint(low=-input_shape[i], high=input_shape[i])
                for i in range(rank)
            ]
        ).astype(np.int32)
        stride_val = np.array(
            [
                np.random.randint(low=-input_shape[i], high=input_shape[i])
                for i in range(rank)
            ]
        ).astype(np.int32)
        if not masking:
            begin_mask = [False] * rank
            end_mask = [False] * rank
            squeeze_mask = [False] * rank
        else:
            begin_mask = np.array(
                [np.random.choice([True, False, False]) for i in range(rank)]
            ).astype(np.bool)
            end_mask = np.array(
                [np.random.choice([True, False, False]) for i in range(rank)]
            ).astype(np.bool)
            squeeze_flag = True
            # We do not squeeze to scalar in nn
            while squeeze_flag and backend == "nn_proto":
                squeeze_mask = np.array(
                    [np.random.choice([True, False]) for i in range(rank)]
                ).astype(np.bool)
                for i in range(rank):
                    if begin_mask[i] or end_mask[i]:
                        squeeze_mask[i] = False
                for s in squeeze_mask:
                    if not s:
                        squeeze_flag = False

        for i in range(rank):
            if begin_mask[i] or end_mask[i]:
                stride = 0
                while stride == 0:
                    stride = np.random.randint(low=-input_shape[i], high=input_shape[i])
                stride_val[i] = stride

                if not end_mask[i]:
                    while True:
                        end = np.random.randint(
                            low=-input_shape[i], high=input_shape[i]
                        )
                        normalized_end = input_shape[i] + end if end < 0 else end
                        if normalized_end == 0 and stride_val[i] > 0:
                            continue
                        elif normalized_end == input_shape[i] - 1 and stride_val[i] < 0:
                            continue
                        else:
                            end_val[i] = end
                            break
                continue
            if squeeze_mask[i]:
                stride_val[i] = 1
            while True:
                end = np.random.randint(low=-input_shape[i], high=input_shape[i])
                normalized_end = input_shape[i] + end if end < 0 else end
                normalized_begin = (
                    input_shape[i] + begin_val[i] if begin_val[i] < 0 else begin_val[i]
                )
                if normalized_end == normalized_begin:
                    continue
                if begin_mask[i] or end_mask[i] or squeeze_mask[i]:
                    stride = 1
                elif normalized_end < normalized_begin:
                    stride = -np.random.randint(low=1, high=input_shape[i])
                else:
                    stride = np.random.randint(low=1, high=input_shape[i])
                end_val[i] = end
                stride_val[i] = stride
                break

        def _mask_to_bit(mask):
            ret = 0
            for x in mask[::-1]:
                ret <<= 1
                if x:
                    ret += 1
            return ret

        @make_tf_graph(
            [
                input_shape,
                list(begin_val.shape) + [tf.int32],
                list(end_val.shape) + [tf.int32],
            ]
        )
        def build_model(x, begin, end):
            return tf.strided_slice(
                x,
                begin,
                end,
                stride_val,
                begin_mask=_mask_to_bit(begin_mask),
                end_mask=_mask_to_bit(end_mask),
                shrink_axis_mask=_mask_to_bit(squeeze_mask),
            )

        model, inputs, outputs = build_model

        input_values = [
            np.array(list(range(np.prod(input_shape))))
            .reshape(input_shape)
            .astype(np.float32),
            begin_val,
            end_val,
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, testcase",
        itertools.product(
            [True, False],
            backends,
            # Change to slice representation for allowing iteration with a non-constant input
            [
                (
                    slice(1, 2),
                    slice(1, 2),
                    slice(1, 2),
                ),  # equivalent to [1:2, 1:2, 1:2]
                (slice(-3, -2), slice(-4, -3), slice(-5, -4)),
                (slice(0, -2), slice(0, -1), slice(-3, -2)),
                (slice(-1, 0, -2), slice(-1, 1, -1), slice(-1, -3, -3)),
                (slice(1, 2), slice(1, 3), slice(1, 4, 2)),
                (slice(None, 2), slice(1, 3), slice(None, 4, 2)),
                (
                    slice(None),
                    slice(1, None),
                    slice(None, 4, 2),
                ),  # equivalent to [:,1:,:4:2]
                (slice(1, None, 1), 1, slice(None, 3, 2)),
                (slice(None), slice(None), slice(None)),
                (slice(1, 2), slice(1, 2), 1),
                (slice(1, 2), slice(None), slice(None)),
                (slice(None), slice(None), slice(None)),
                (slice(1, 2), slice(None), slice(1, 2)),
                (slice(None), slice(None), 1),
                (0, 0, slice(None)),
                (slice(1, 2)),
                (slice(1, 2), slice(1, 2)),
                (1),
                (slice(0, 3)),
                (slice(None)),
                (slice(None), slice(None), slice(None, None, -1)),
            ],
        ),
    )
    def test_slice_by_index_from_scratch(self, use_cpu_only, backend, testcase):
        input_shape = np.array([3, 4, 5])

        @make_tf_graph([input_shape])
        def build_model(x):
            return x[testcase]

        model, inputs, outputs = build_model

        input_values = [
            np.array(list(range(np.prod(input_shape))))
            .reshape(input_shape)
            .astype(np.float32)
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_slice_by_index_smoke(self, use_cpu_only, backend):
        def test_two_slice_ops():
            input_shape = [1, 64, 2]
            x_val = np.random.rand(*input_shape).astype(np.float32)
            y_val = np.random.rand(*input_shape).astype(np.float32)

            @make_tf_graph([input_shape, input_shape])
            def build_model(x, y):
                x_slice = x[:, :, 0]
                y_slice = y[:, :, 0]
                return (x_slice, y_slice)

            model, inputs, outputs = build_model

            input_values = [x_val, y_val]
            input_dict = dict(zip(inputs, input_values))
            run_compare_tf(
                model,
                input_dict,
                outputs,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )

        def test_slice_new_axis():
            input_shape = [4, 5, 64]
            val = np.random.rand(*input_shape).astype(np.float32)
            num_cases = 8

            @make_tf_graph([input_shape] * num_cases)
            def build_model(*args):
                a, b, c, d, e, f, g, h = args
                slice_0 = a[:1, tf.newaxis, :3, :]
                slice_1 = b[:, tf.newaxis]
                slice_2 = c[..., tf.newaxis]
                slice_3 = d[..., tf.newaxis, :, 10]
                slice_4 = e[:, 2, tf.newaxis, ...]
                slice_5 = f[2, ..., :, tf.newaxis]
                slice_6 = g[tf.newaxis, ..., tf.newaxis]
                slice_7 = h[tf.newaxis, 2, tf.newaxis, ...]

                return (
                    slice_0,
                    slice_1,
                    slice_2,
                    slice_3,
                    slice_4,
                    slice_5,
                    slice_6,
                    slice_7,
                )

            model, inputs, outputs = build_model

            input_values = [val] * num_cases
            input_dict = dict(zip(inputs, input_values))
            run_compare_tf(
                model,
                input_dict,
                outputs,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )

        test_two_slice_ops()
        test_slice_new_axis()

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, single_size, dynamic_size",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(1, 5)],
            [True, False],
            [True, False],
        ),
    )
    def test_slice_by_size(
        self, use_cpu_only, backend, rank, single_size, dynamic_size
    ):
        input_shape = np.random.randint(low=2, high=4, size=rank)
        begin_val = np.array(
            [np.random.randint(input_shape[i]) for i in range(rank)]
        ).astype(np.int32)
        size_val = np.array(
            [np.random.randint(input_shape[i] - begin_val[i]) + 1 for i in range(rank)]
        )
        if single_size:
            for r in range(rank):
                size_val_r = np.array(
                    [s if i == r else -1 for i, s in enumerate(size_val)]
                ).astype(np.int32)

                @make_tf_graph([input_shape, list(begin_val.shape) + [tf.int32]])
                def build_model(x, begin):
                    return tf.slice(x, begin, size_val_r)

                @make_tf_graph(
                    [
                        input_shape,
                        list(begin_val.shape) + [tf.int32],
                        list(size_val_r.shape) + [tf.int32],
                    ]
                )
                def build_model_dynamic_size(x, begin, size):
                    return tf.slice(x, begin, size)

                if dynamic_size:
                    model, inputs, outputs = build_model_dynamic_size
                    input_values = [
                        random_gen(input_shape, rand_min=-100, rand_max=100),
                        begin_val,
                        size_val_r,
                    ]
                else:
                    model, inputs, outputs = build_model
                    input_values = [
                        random_gen(input_shape, rand_min=-100, rand_max=100),
                        begin_val,
                    ]

                input_dict = dict(zip(inputs, input_values))
                run_compare_tf(
                    model,
                    input_dict,
                    outputs,
                    use_cpu_only=use_cpu_only,
                    frontend_only=False,
                    backend=backend,
                )
        else:
            size_val = np.array(
                [s if np.random.randint(2) == 0 else -1 for s in size_val]
            ).astype(np.int32)

            @make_tf_graph([input_shape, list(begin_val.shape) + [tf.int32]])
            def build_model(x, begin):
                return tf.slice(x, begin, size_val)

            @make_tf_graph(
                [
                    input_shape,
                    list(begin_val.shape) + [tf.int32],
                    list(size_val.shape) + [tf.int32],
                ]
            )
            def build_model_dynamic_size(x, begin, size):
                return tf.slice(x, begin, size)

            if dynamic_size:
                model, inputs, outputs = build_model_dynamic_size
                input_values = [
                    random_gen(input_shape, rand_min=-100, rand_max=100),
                    begin_val,
                    size_val,
                ]
            else:
                model, inputs, outputs = build_model
                input_values = [
                    random_gen(input_shape, rand_min=-100, rand_max=100),
                    begin_val,
                ]

            input_dict = dict(zip(inputs, input_values))
            run_compare_tf(
                model,
                input_dict,
                outputs,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )


class TestMatrixBandPart:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, lower_and_upper",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(2, 6)],
            [(0, -1), (-1, 0), (0, 0)],
        ),
    )
    def test_matrix_band_part(self, use_cpu_only, backend, rank, lower_and_upper):
        lower, upper = lower_and_upper
        shape = np.random.randint(low=3, high=4, size=rank)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=shape)
            res = tf.matrix_band_part(x, num_lower=lower, num_upper=upper)
            run_compare_tf(
                graph,
                {x: random_gen(shape, rand_min=-100, rand_max=100)},
                res,
                use_cpu_only=use_cpu_only,
                backend=backend,
            )


class TestCumSum:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, reverse, exclusive",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(1, 6)],
            [True, False],
            [True, False],
        ),
    )
    def test_cumsum(self, use_cpu_only, backend, rank, reverse, exclusive):
        input_shape = np.random.randint(low=1, high=4, size=rank)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=input_shape)
            for axis in range(-1, rank, 3):
                res = tf.math.cumsum(x, axis=axis, reverse=reverse, exclusive=exclusive)
                if use_cpu_only:
                    dtype = np.float32
                else:
                    dtype = np.float16
                run_compare_tf(
                    graph,
                    {
                        x: random_gen(
                            input_shape, rand_min=-100, rand_max=100, dtype=dtype
                        )
                    },
                    res,
                    use_cpu_only=use_cpu_only,
                    frontend_only=False,
                    backend=backend,
                )

    @pytest.mark.xfail(reason="Backend doesn't support empty counts", run=False)
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_cumsum_empty_input_tf(self, use_cpu_only, backend):
        empty_inputs = [[], [[]], [[[]]], [[], []], [[[]], [[]]]]
        for input_x in empty_inputs:
            input_x = np.array(input_x)
            with tf.Graph().as_default() as graph:
                x = tf.placeholder(tf.float32, shape=input_x.shape)
                for axis in range(-1, len(input_x.shape), 2):
                    res = tf.math.cumsum(x, axis=axis)
                    run_compare_tf(
                        graph,
                        {x: input_x},
                        res,
                        use_cpu_only=use_cpu_only,
                        frontend_only=False,
                        backend=backend,
                    )


class TestFill:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, value",
        itertools.product(
            [True, False], backends, [rank for rank in range(1, 6)], [-19.0, 0.0, 37.0]
        ),
    )
    def test_fill(self, use_cpu_only, backend, rank, value):
        def test_tf_static():
            shape = np.random.randint(low=1, high=3, size=rank)
            with tf.Graph().as_default() as graph:
                x = tf.placeholder(tf.float32, shape=shape)
                ref = tf.add(
                    x, tf.fill(dims=np.array(shape, dtype=np.float32), value=value)
                )
                run_compare_tf(
                    graph,
                    {x: np.random.rand(*shape)},
                    ref,
                    use_cpu_only=use_cpu_only,
                    backend=backend,
                )

        def test_tf_dynamic():
            shape = np.random.randint(low=1, high=3, size=rank)
            with tf.Graph().as_default() as graph:
                s = tf.placeholder(tf.int32, shape=(len(shape),))
                ref = tf.fill(dims=s, value=value)
                run_compare_tf(
                    graph,
                    {s: np.array(shape, dtype=np.float32)},
                    ref,
                    use_cpu_only=use_cpu_only,
                    backend=backend,
                )

        test_tf_static()
        test_tf_dynamic()


class TestNonMaximumSuppression:
    @pytest.mark.parametrize(
        ",".join(
            [
                "use_cpu_only",
                "backend",
                "num_boxes",
                "max_boxes",
                "iou_threshold",
                "score_threshold",
            ]
        ),
        itertools.product(
            [True, False],
            backends,
            [20, 30, 80],
            [5, 20, 100],
            [1.0, 0.99],
            [float("-inf"), -200.0],
        ),
    )
    def test_non_max_suppression(
        self,
        use_cpu_only,
        backend,
        num_boxes,
        max_boxes,
        iou_threshold,
        score_threshold,
    ):
        boxes_val = random_gen(shape=(num_boxes, 4), rand_min=0, rand_max=32)
        scores_val = random_gen(shape=(num_boxes,), rand_min=-100, rand_max=100)

        with tf.Graph().as_default() as graph:
            boxes = tf.placeholder(tf.float32, shape=boxes_val.shape)
            scores = tf.placeholder(tf.float32, shape=scores_val.shape)
            ref = tf.image.non_max_suppression(
                boxes=boxes,
                scores=scores,
                max_output_size=max_boxes,
                iou_threshold=iou_threshold,
                score_threshold=score_threshold,
            )
            run_compare_tf(
                graph,
                {boxes: boxes_val, scores: scores_val},
                ref,
                use_cpu_only=use_cpu_only,
                backend=backend,
            )


class TestOneHot:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank_and_axis, dynamic",
        itertools.product(
            [True, False],
            backends,
            [
                (2, 0),
                (2, -1),
                (3, 3),
                (3, 0),
                (3, -2),
                (4, -4),
                (4, 1),
                (4, -1),
                (4, -2),
                (4, 3),
            ],
            [True, False],
        ),
    )
    def test_one_hot(self, use_cpu_only, backend, rank_and_axis, dynamic):
        rank, axis = rank_and_axis
        depth, on_value, off_value = 30, 28.0, -4.0
        x_shape = np.random.randint(low=2, high=4, size=rank)

        if not dynamic:
            with tf.Graph().as_default() as graph:
                x = tf.placeholder(tf.int32, shape=x_shape)
                axis = (
                    axis if axis >= -1 else axis + rank + 1
                )  ## TF limitation: Doesn't support axis < -1
                res = tf.one_hot(
                    x, axis=axis, depth=depth, on_value=on_value, off_value=off_value
                )
                run_compare_tf(
                    graph,
                    {x: np.random.randint(0, depth, size=x_shape)},
                    res,
                    use_cpu_only=use_cpu_only,
                    frontend_only=False,
                    backend=backend,
                )
        else:  # Dynamic Case with depth being an input
            with tf.Graph().as_default() as graph:
                x = tf.placeholder(tf.int32, shape=x_shape)
                depth_input = tf.placeholder(tf.int32)
                axis = (
                    axis if axis >= -1 else axis + rank + 1
                )  ## TF limitation: Doesn't support axis < -1
                res = tf.one_hot(
                    x,
                    axis=axis,
                    depth=depth_input,
                    on_value=on_value,
                    off_value=off_value,
                )
                run_compare_tf(
                    graph,
                    {x: np.random.randint(0, depth, size=x_shape), depth_input: depth},
                    res,
                    use_cpu_only=use_cpu_only,
                    frontend_only=False,
                    backend=backend,
                )


class TestPad:
    @pytest.mark.parametrize("use_cpu_only, backend, rank, mode, dynamic, trial",
                             itertools.product(
                                 [True, False],
                                 backends,
                                 [2, 3, 4],
                                 ['constant', 'reflect'],
                                 [True, False],
                                 list(range(10)),
                             )
                             )
    def test(self, use_cpu_only, backend, rank, mode, dynamic, trial):
        if backend == "mil_proto" and dynamic:
            return
        input_shape = np.random.randint(low=2, high=10, size=rank)
        min_input_dim_size = input_shape.min()
        padding_val = np.random.randint(low=0, high=min_input_dim_size, size=(rank, 2), dtype=np.int32)

        # Only constant mode supports padding across all dimensions
        # All other padding modes are only applied on two dimensions.
        perm = list(range(rank))
        import random
        random.shuffle(perm)
        if mode != "constant":
            padding_val[perm[:-2]] = 0
        tf_mode = mode.upper()

        if dynamic:
            if mode != "constant":
                return
            padding_shape = padding_val.shape
            @make_tf_graph([input_shape, list(padding_shape)+[tf.int32]])
            def build_model(x, paddings):
                return tf.pad(x, paddings=paddings, mode=tf_mode)

            model, inputs, outputs = build_model
            input_values = [random_gen(input_shape, rand_min=0.2, rand_max=1000), padding_val]
            input_dict = dict(zip(inputs, input_values))

        else:
            @make_tf_graph([input_shape])
            def build_model(x):
                return tf.pad(x, paddings=padding_val, mode=tf_mode)

            model, inputs, outputs = build_model
            input_values = [random_gen(input_shape, rand_min=0.2, rand_max=1000)]
            input_dict = dict(zip(inputs, input_values))

        run_compare_tf(model, input_dict, outputs,
                       use_cpu_only=use_cpu_only,
                       frontend_only=False, backend=backend)


class TestPadV2:
    @pytest.mark.parametrize("use_cpu_only, backend, rank, constant_values, dynamic, trial",
                             itertools.product(
                                 [True, False],
                                 backends,
                                 list(range(1, 6)),
                                 [0., 10, -1],
                                 [True],
                                 list(range(10))
                             )
                             )
    def test(self, use_cpu_only, backend, rank, constant_values, dynamic, trial):
        if backend == "mil_proto" and dynamic:
            return
        input_shape = np.random.randint(low=2, high=10, size=rank)
        paddings = np.random.randint(low=2, high=5, size=2*rank)
        padding_val = paddings.reshape(-1,2)
        if dynamic:
            padding_shape = padding_val.shape
            @make_tf_graph([input_shape, list(padding_shape)+[tf.int32]])
            def build_model(x, paddings):
                return tf.raw_ops.PadV2(input=x, paddings=paddings, constant_values=constant_values)

            model, inputs, outputs = build_model

            input_values = [random_gen(input_shape, rand_min=0.2, rand_max=1000), padding_val]
            input_dict = dict(zip(inputs, input_values))

        else:
            @make_tf_graph([input_shape])
            def build_model(x):
                return tf.raw_ops.PadV2(input=x, paddings=padding_val, constant_values=constant_values)

            model, inputs, outputs = build_model

            input_values = [random_gen(input_shape, rand_min=0.2, rand_max=1000)]
            input_dict = dict(zip(inputs, input_values))
        run_compare_tf(model, input_dict, outputs,
                       use_cpu_only=use_cpu_only,
                       frontend_only=False, backend=backend)


class TestRange:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, params",
        itertools.product(
            [True, False],
            backends,
            [
                (-10.4, 23, 12.2),
                (0, 10, 1),
                (50.5, 90.5, 1.5),
                (5, 8, 2),
                (5, 8, 98),
                (5, 8, 1.5),
                (10, 5, -0.6),
                (24, -65, -2),
            ],
        ),
    )
    def test_range(self, use_cpu_only, backend, params):
        start, end, step = params
        with tf.Graph().as_default() as graph:
            limit = tf.placeholder(tf.float32)
            res = tf.range(start=start, limit=limit, delta=step)
            run_compare_tf(
                graph,
                {limit: end},
                res,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )

        with tf.Graph().as_default() as graph:
            delta = tf.placeholder(tf.float32)
            res = tf.range(start=start, limit=end, delta=delta)
            run_compare_tf(
                graph,
                {delta: step},
                res,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )

        with tf.Graph().as_default() as graph:
            begin = tf.placeholder(tf.float32)
            res = tf.range(start=begin, limit=end, delta=step)
            run_compare_tf(
                graph,
                {begin: start},
                res,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )


class TestTile:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank_and_reps",
        itertools.product(
            [True, False],
            backends,
            [
                (1, (2,)),
                (2, (1, 2)),
                (2, (2, 2)),
                (3, (3, 2, 1)),
                (3, (2, 1, 3)),
                (3, (2, 1, 1)),
                (4, (1, 3, 2, 1)),
                (4, (2, 1, 1, 2)),
                (5, (2, 1, 1, 3, 2)),
                (5, (1, 1, 2, 3, 2)),
            ],
        ),
    )
    def test_tile(self, use_cpu_only, backend, rank_and_reps):
        rank, reps = rank_and_reps
        x_shape = np.random.randint(low=2, high=4, size=rank)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=x_shape)
            res = tf.tile(x, multiples=reps)
            run_compare_tf(
                graph,
                {x: np.random.rand(*x_shape)},
                res,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )


@pytest.mark.skip(reason="rdar://65198011 (Re-enable Conv3dTranspose and DynamicTile unit tests)")
class TestDynamicTile:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [1, 2, 3, 4, 5]),
    )
    def test_tile(self, use_cpu_only, backend, rank):
        x_shape = np.random.randint(low=2, high=4, size=rank)
        reps_val = np.random.randint(low=1, high=3, size=rank)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=x_shape)
            reps = tf.placeholder(tf.int32, shape=reps_val.shape)
            res = tf.tile(x, multiples=reps)
            run_compare_tf(
                graph,
                {x: np.random.rand(*x_shape), reps: reps_val},
                res,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )


class TestTopK:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, k",
        itertools.product(
            [True, False], backends, [rank for rank in range(1, 6)], [1, 2, 3],
        ),
    )
    def test_top_k(self, use_cpu_only, backend, rank, k):
        # TensorFlow only supports last dimension (axis = -1).
        shape = np.random.randint(low=3, high=4, size=rank)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=shape)
            ref = tf.math.top_k(x, k=k, sorted=True)
            ref = (ref[1], ref[0])
            run_compare_tf(
                graph,
                {x: random_gen(shape, rand_min=-100, rand_max=100)},
                ref,
                use_cpu_only=use_cpu_only,
                backend=backend,
            )


class TestConcat:
    @pytest.mark.parametrize("use_cpu_only, backend, op_version, rank, num_inputs",
                             itertools.product(
                                 [True, False],
                                 ['nn_proto'],
                                 ['v1', 'v2'],
                                 [1,2,3,4,5],
                                 list(range(1, 4)),
                             ))
    def test_concat(self, use_cpu_only, backend, op_version, rank, num_inputs):
        import random
        for axis in range(-rank,rank):
            input_shape = np.random.randint(low=1, high=6, size=rank)
            input_shapes = [input_shape.copy() for _ in range(num_inputs)]
            concat_axis_value = np.random.randint(low=1, high=3, size=num_inputs)
            for i, v in enumerate(concat_axis_value):
                input_shapes[i][axis] = concat_axis_value[i]

            @make_tf_graph(input_shapes)
            def build_model(*inputs):
                # add 5 zero size constants
                zero_shape = input_shape.copy()
                zero_shape[axis] = 0
                const = [tf.constant([], shape=zero_shape) for _ in range(5)]
                values = inputs + tuple(const)
                values = list(values)
                random.shuffle(values)
                values = tuple(values)
                if op_version == 'v1':
                    # Seems like now the tf functions are using concatV2, so create as raw_ops here
                    res = tf.raw_ops.Concat(concat_dim=axis, values=values)
                elif op_version == 'v2':
                    res = tf.raw_ops.ConcatV2(values=values, axis=axis)
                return res

            model, inputs, outputs = build_model

            input_values = [np.random.rand(*shape).astype(np.float32) for shape in input_shapes]
            input_dict = dict(zip(inputs, input_values))

            run_compare_tf(model, input_dict, outputs,
                   use_cpu_only=use_cpu_only,
                   frontend_only=False, backend=backend)


class TestSplit:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, dynamic",
        itertools.product([True, False], backends, [1, 2, 3, 4], [True, False]),
    )
    def test_split(self, use_cpu_only, backend, rank, dynamic):
        input_shape1 = np.random.randint(low=1, high=3, size=rank)
        for axis in range(-rank, rank, 2):
            for split_num in range(1, input_shape1[axis] + 1, 2):
                if input_shape1[axis] % split_num != 0:
                    continue
                tf_input_shape = list(input_shape1)
                if dynamic:
                    axis1 = np.random.randint(low=0, high=rank)
                    tf_input_shape[axis1] = None
                with tf.Graph().as_default() as graph:
                    x = tf.placeholder(tf.float32, shape=tf_input_shape)
                    res = tf.split(x, split_num, axis=axis)
                    # TODO (rdar://60358242) If tf.split output is returned, there's no
                    # get_tuple nodes. Some graph pass is needed. Example:
                    #
                    #    x = tf.placeholder(tf.float32, shape=input_shape1)
                    #    res = tf.split(x, 3, axis=0)
                    #
                    # res are ['split:0', 'split:1', 'split']
                    #
                    # but node.outputs == ['gto_1', 'gto_2', 'gto_3']
                    import random

                    random.shuffle(res)
                    res = tuple(res)
                    inputs = {
                        x: np.random.rand(*input_shape1),
                    }
                    run_compare_tf(
                        graph,
                        inputs,
                        res,
                        use_cpu_only=use_cpu_only,
                        frontend_only=False,
                        backend=backend,
                    )

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_splitv(self, use_cpu_only, backend):
        input_shape1 = [3, 2, 1]
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=input_shape1)
            res = tf.split(x, [1, 2], axis=0)
            res = (res[0], res[1])
            inputs = {
                x: np.random.rand(*input_shape1),
            }
            run_compare_tf(
                graph,
                inputs,
                res,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )


class TestStack:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_stack(self, use_cpu_only, backend):
        input_shape1 = [3, 1, 1]
        input_shape2 = [3, 1, 1]
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=input_shape1)
            y = tf.placeholder(tf.float32, shape=input_shape2)
            res = [tf.stack((x, y), axis=0), tf.stack((x, y), axis=-1)]
            inputs = {
                x: np.random.rand(*input_shape1),
                y: np.random.rand(*input_shape2),
            }
            run_compare_tf(
                graph,
                inputs,
                res,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )


class TestPack:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, num_inputs",
        itertools.product([True, False], backends, list(range(5)), list(range(1, 5))),
    )
    def test_pack(self, use_cpu_only, backend, rank, num_inputs):
        shape = np.random.randint(low=1, high=4, size=rank)
        input_shapes = [shape[:] for _ in range(num_inputs)]

        @make_tf_graph(input_shapes)
        def build_model(*inputs):
            return tf.raw_ops.Pack(values=inputs, axis=0)

        model, inputs, outputs = build_model
        input_values = [
            random_gen(shape, rand_min=-1, rand_max=1) for shape in input_shapes
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestArgSort:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, axis, direction",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(1, 6)],
            [-1, 0],
            ["ascending", "descending"],
        ),
    )
    def test_argsort(self, use_cpu_only, backend, rank, axis, direction):
        shape = np.random.randint(low=1, high=4, size=rank)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=shape)
            ref = tf.argsort(x, axis=axis, direction=direction.upper())
            if use_cpu_only:
                dtype = np.float32
            else:
                dtype = np.float16
            run_compare_tf(
                graph,
                {
                    x: random_gen(
                        shape,
                        rand_min=-100,
                        rand_max=100,
                        allow_duplicate=False,
                        dtype=dtype,
                    )
                },
                ref,
                use_cpu_only=use_cpu_only,
                backend=backend,
            )


class TestDepthToSpace:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, shape, block_size",
        itertools.product(
            [True, False],
            backends,
            [(1, 1, 1, 16), (1, 1, 1, 32), (1, 3, 3, 16)],
            [2, 4],
        ),
    )
    def test_depth_to_space(self, use_cpu_only, backend, shape, block_size):
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=shape)
            ref = tf.depth_to_space(x, block_size)
            run_compare_tf(
                graph,
                {x: np.random.rand(*shape)},
                ref,
                use_cpu_only=use_cpu_only,
                backend=backend,
            )


class TestReshape:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_flatten(self, use_cpu_only, backend):
        shapes = [[2, 2], [3, 2, 1, 2], [2, 1, 4, 3]]

        for input_shape in shapes:

            @make_tf_graph([input_shape])
            def build_model(x):
                return tf.keras.backend.flatten(x)

            model, inputs, outputs = build_model

            input_values = [np.random.rand(*input_shape).astype(np.float32)]
            input_dict = dict(zip(inputs, input_values))
            run_compare_tf(
                model,
                input_dict,
                outputs,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(1, 6)],),
    )
    def test_shape(self, use_cpu_only, backend, rank):
        shape = np.random.randint(low=3, high=4, size=rank)
        shape_holder = [None] * rank

        @make_tf_graph([shape_holder])
        def build_model(x):
            return tf.shape(x)

        model, inputs, outputs = build_model

        input_values = [random_gen(shape, rand_min=-100, rand_max=100)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank_and_axis",
        itertools.product(
            [True, False],
            backends,
            [
                (rank, axis)
                for rank in range(1, 5)
                for axis in range(-rank - 1, rank + 1)
            ],
        ),
    )
    def test_expand_dims(self, use_cpu_only, backend, rank_and_axis):
        rank, axis = rank_and_axis
        input_shape = np.random.randint(low=2, high=4, size=rank)

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.expand_dims(x, axis=axis)

        model, inputs, outputs = build_model

        input_values = [np.random.rand(*input_shape).astype(np.float32)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, input_shape",
        itertools.product(
            [False],
            backends,
            [
                ([10, 10], [5, 20]),
                ([3, 4, 5, 6], [4, 5, 3, 6]),
                ([4, 4, 5, 6], [2, 2, -1]),
            ],
        ),
    )
    def test_reshape_static(self, use_cpu_only, backend, input_shape):
        @make_tf_graph([input_shape[0]])
        def build_model(x):
            return tf.reshape(x, shape=input_shape[1])

        model, inputs, outputs = build_model

        input_values = [np.random.rand(*input_shape[0]).astype(np.float32)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, input_shape",
        itertools.product(
            [False],
            backends,
            [
                ([10, 10], [5, 20]),
                ([3, 4, 5, 6], [4, 5, 3, 6]),
                ([4, 4, 5, 6], [2, 2, -1]),
                ([2, 3, 5, 3], [2, -1]),
            ],
        ),
    )
    def test_reshape_dynamic(self, use_cpu_only, backend, input_shape):
        @make_tf_graph([input_shape[0], (len(input_shape[1]), tf.int32)])
        def build_model(x, y):
            return tf.reshape(x, shape=y)

        model, inputs, outputs = build_model

        input_values = [
            np.random.rand(*input_shape[0]).astype(np.float32),
            np.array(input_shape[1], dtype=np.int32),
        ]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, shape",
        itertools.product([False], backends, [[1], [1, 1], [1, 1, -1], []],),
    )
    def test_reshape_scalar(self, use_cpu_only, backend, shape):
        input_shape = ()

        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.raw_ops.Reshape(tensor=x, shape=shape)

        model, inputs, outputs = build_model

        input_values = [np.random.rand(*input_shape)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

class TestMatrixDiag:
    @pytest.mark.parametrize("use_cpu_only, backend, length, dynamic",
                             itertools.product(
                                 [True, False],
                                 backends,
                                 [length for length in range(1, 5)],
                                 [True, False]))
    def test(self, use_cpu_only, backend, length, dynamic):

        if dynamic:
            return  # FIXME: "rdar://65198011 (Re-enable Conv3dTranspose and DynamicTile unit tests)"
            input_shape = np.random.randint(low=1, high=4, size=length)
            a, b = np.prod(input_shape[:2]), np.prod(input_shape[2:])
            size = np.array([a,b]).astype(np.int32)
            reshape_shape = [2]
            @make_tf_graph([input_shape, reshape_shape+[tf.int32]])
            def build_model(x, reshape):
                x = tf.reshape(x, reshape)
                x = tf.reshape(x, [-1])
                return tf.raw_ops.MatrixDiag(diagonal=x)
            model, inputs, outputs = build_model
            input_values = [random_gen(input_shape, -1, 1), size]
        else:
            input_shape = [length]
            @make_tf_graph([input_shape])
            def build_model(x):
                return tf.raw_ops.MatrixDiag(diagonal=x)
            model, inputs, outputs = build_model
            input_values = [random_gen(input_shape, -1, 1)]

        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(model, input_dict, outputs,
                       use_cpu_only=use_cpu_only,
                       frontend_only=False, backend=backend)

class TestReverse:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank_and_axes",
        itertools.product(
            [True, False],
            backends,
            [
                (1, (-1,)),
                (2, (0,)),
                (2, (-1, 0)),
                (3, (1, -3)),
                (3, (-2,)),
                (3, (0, 1, 2)),
                (4, (-2, -1, 0)),
                (4, (-1, -2)),
                (4, []),
                (5, (-3, -1, 3)),
                (5, (0, -1, 1, -2)),
            ],
        ),
    )
    def test_reverse(self, use_cpu_only, backend, rank_and_axes):
        rank, axes = rank_and_axes
        shape = np.random.randint(low=1, high=4, size=rank)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=shape)
            res = tf.reverse(x, axis=axes)
            run_compare_tf(
                graph,
                {x: np.random.rand(*shape)},
                res,
                use_cpu_only=use_cpu_only,
                backend=backend,
            )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [rank for rank in range(2, 6)]),
    )
    def test_reverse_sequence(self, use_cpu_only, backend, rank):
        shape = np.random.randint(low=1, high=4, size=rank)
        seq_axis = np.random.randint(low=1, high=rank)
        batch_axis = np.random.randint(low=0, high=seq_axis)
        lengths = np.random.randint(low=0, high=shape[seq_axis], size=shape[batch_axis])

        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=shape)
            res = tf.reverse_sequence(
                x, seq_lengths=lengths, seq_axis=seq_axis, batch_axis=batch_axis
            )
            run_compare_tf(
                graph,
                {x: np.random.rand(*shape)},
                res,
                use_cpu_only=use_cpu_only,
                backend=backend,
            )


class TestSpaceToDepth:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, shape, block_size",
        itertools.product(
            [True, False],
            backends,
            [(1, 6, 6, 1), (1, 12, 12, 1), (1, 6, 6, 3)],
            [2, 3],
        ),
    )
    def test_space_to_depth(self, use_cpu_only, backend, shape, block_size):
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=shape)
            ref = tf.space_to_depth(x, block_size)
            run_compare_tf(
                graph,
                {x: np.random.rand(*shape)},
                ref,
                use_cpu_only=use_cpu_only,
                backend=backend,
            )


class TestSqueeze:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank_and_axes",
        itertools.product(
            [True, False],
            backends,
            [
                (2, (1,)),
                (2, (0,)),
                (3, (1,)),
                (3, (0, -1)),
                (3, []),
                (4, (-1, 2, 1)),
                (4, (0, 1)),
                (5, (3, 1, 2)),
                (5, (-1,)),
            ],
        ),
    )
    def test_squeeze(self, use_cpu_only, backend, rank_and_axes):
        rank, axes = rank_and_axes
        x_shape = np.random.randint(low=2, high=4, size=rank)
        for axis in axes:
            x_shape[axis] = 1

        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=x_shape)
            res = tf.squeeze(x, axis=axes)
            run_compare_tf(
                graph,
                {x: np.random.rand(*x_shape)},
                res,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )

        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=x_shape)
            res = tf.squeeze(x, axis=None)
            run_compare_tf(
                graph,
                {x: np.random.rand(*x_shape)},
                res,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )


class TestTranspose:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank_and_perm",
        itertools.product(
            [True, False],
            backends,
            [
                (1, (0,)),
                (2, (1, 0)),
                (2, (0, 1)),
                (3, (0, 2, 1)),
                (3, (2, 1, 0)),
                (3, (2, 0, 1)),
                (4, (0, 3, 2, 1)),
                (4, (3, 0, 1, 2)),
                (5, (2, 3, 1, 0, 4)),
                (5, (3, 1, 0, 4, 2)),
            ],
        ),
    )
    def test_transpose_1(self, use_cpu_only, backend, rank_and_perm):
        rank, perm = rank_and_perm
        x_shape = np.random.randint(low=2, high=4, size=rank)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=x_shape)
            res = tf.transpose(x, perm=perm)
            run_compare_tf(
                graph,
                {x: np.random.rand(*x_shape)},
                res,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank",
        itertools.product([True, False], backends, [1, 2, 3, 4],),
    )
    def test_transpose_2(self, use_cpu_only, backend, rank):
        input_shape = np.random.randint(low=2, high=4, size=rank)
        perm = np.random.permutation(rank).astype(np.float32)

        def static_perm():
            with tf.Graph().as_default() as graph:
                x = tf.placeholder(tf.float32, shape=input_shape)
                res = tf.transpose(x, perm)
                run_compare_tf(
                    graph,
                    {x: np.random.rand(*input_shape)},
                    res,
                    use_cpu_only=use_cpu_only,
                    frontend_only=False,
                    backend=backend,
                )

        def dynamic_perm():
            with tf.Graph().as_default() as graph:
                x = tf.placeholder(tf.float32, shape=input_shape)
                tf_perm = tf.placeholder(tf.int32, shape=[None])
                res = tf.transpose(x, tf_perm)
                run_compare_tf(
                    graph,
                    {x: np.random.rand(*input_shape), tf_perm: perm},
                    res,
                    use_cpu_only=use_cpu_only,
                    frontend_only=False,
                    backend=backend,
                )

        static_perm()
        # Note that TF supports dynamic perm in tf.transpose.
        with pytest.raises(ValueError, match=r".*must be const at compile time.*"):
            dynamic_perm()


class TestSpaceToBatchND:
    # No direct mil smoke test since it's a TF op which is a composite of several ops.
    @pytest.mark.parametrize(
        "use_cpu_only, backend, input_shape, block_shape, paddings",
        itertools.product(
            [True, False],
            backends,
            [(1, 4, 4, 1), (1, 4, 4, 3), (2, 4, 6, 1)],
            [[2, 2]],
            [[[0, 0], [0, 0]], [[1, 1], [0, 2]], [[4, 2], [4, 2]]],
        ),
    )
    def test_smoke(self, use_cpu_only, backend, input_shape, block_shape, paddings):
        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.raw_ops.SpaceToBatchND(
                input=x, block_shape=block_shape, paddings=paddings
            )

        model, inputs, outputs = build_model
        input_values = [random_gen(input_shape)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, input_block_rank, dynamic",
        itertools.product(
            [True, False],
            backends,
            [(3, 1), (3, 2), (4, 1)],
            [True, False],
        ),
    )
    def test_programmatic(
        self, use_cpu_only, backend, input_block_rank, dynamic
    ):

        input_rank, block_rank = input_block_rank

        # generate data
        input_shape = np.random.randint(low=1, high=4, size=input_rank)
        block_shape = np.random.randint(low=1, high=3, size=block_rank)
        paddings = []
        for i in range(block_rank):
            while True:
                temp = np.random.randint(low=0, high=10, size=2)
                if (np.sum(temp) + input_shape[i + 1]) % block_shape[i] == 0:
                    paddings.append(temp)
                    break
        paddings = np.array(paddings)

        if not dynamic:

            @make_tf_graph([input_shape])
            def build_model(x):
                return tf.raw_ops.SpaceToBatchND(
                    input=x, block_shape=block_shape, paddings=paddings
                )

        else:

            @make_tf_graph([[None] * input_rank])
            def build_model(x):
                return tf.raw_ops.SpaceToBatchND(
                    input=x, block_shape=block_shape, paddings=paddings
                )

        model, inputs, outputs = build_model
        input_values = [random_gen(input_shape)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestBatchToSpaceND:
    # No direct mil smoke test since it's a TF op which is a composite of several ops.
    @pytest.mark.parametrize(
        "use_cpu_only, backend, input_shape, block_size, crops",
        itertools.product(
            [True, False],
            backends,
            [(4, 4, 4, 1), (4, 4, 4, 3), (4, 4, 6, 1)],
            [[2, 2]],
            [[[0, 0], [0, 0]], [[1, 1], [0, 2]], [[4, 2], [4, 2]]],
        ),
    )
    def test_smoke(self, use_cpu_only, backend, input_shape, block_size, crops):
        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.raw_ops.BatchToSpaceND(
                input=x, block_shape=block_size, crops=crops
            )

        model, inputs, outputs = build_model
        input_values = [random_gen(input_shape)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, input_block_rank, dynamic",
        itertools.product(
            [True, False],
            backends,
            [(3, 1), (3, 2), (4, 1)],
            [True, False]
        ),
    )
    def test_programmatic(
        self, use_cpu_only, backend, input_block_rank, dynamic):

        input_rank, block_rank = input_block_rank

        # generate data
        input_shape = np.random.randint(low=1, high=4, size=input_rank)
        block_shape = np.random.randint(low=1, high=3, size=block_rank)
        input_shape[0] = input_shape[0] * np.prod(block_shape)
        crops = []
        for i in range(block_rank):
            while True:
                temp = np.random.randint(low=0, high=4, size=2)
                if np.sum(temp) < input_shape[i + 1] * block_shape[i]:
                    crops.append(temp)
                    break
        crops = np.array(crops)

        if not dynamic:

            @make_tf_graph([input_shape])
            def build_model(x):
                return tf.raw_ops.BatchToSpaceND(
                    input=x, block_shape=block_shape, crops=crops
                )

        else:

            @make_tf_graph([[None] * input_rank])
            def build_model(x):
                return tf.raw_ops.BatchToSpaceND(
                    input=x, block_shape=block_shape, crops=crops
                )

        model, inputs, outputs = build_model
        input_values = [random_gen(input_shape)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestTensorArray:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_tf_basic(self, use_cpu_only, backend):
        # TF1: TensorArrayV3, TensorArrayWriteV3, TensorArrayScatterV3,
        #      TensorArraySizeV3, TensorArrayGatherV3
        # TF2: TensorListReserve, TensorListLength, TensorListSetItem,
        #      TensorListScatterIntoExistingList, TensorListStack,
        #      TensorListResize

        elem_shape = (3, 2)

        @make_tf_graph([elem_shape])
        def build_model(x):
            ta = tf.TensorArray(dtype=tf.float32, size=1, dynamic_size=True)

            ta = ta.write(2, x)

            # TensorArray has write-once semantics, and thus we write to a new
            # index
            # (https://www.tensorflow.org/api_docs/python/tf/TensorArray)
            # writing to out of bound index
            ta = ta.scatter([3], tf.expand_dims(x, 0))

            # writing to in-bound index
            ta = ta.scatter([0], tf.expand_dims(x, 0))

            return ta.stack()

        model, inputs, outputs = build_model
        input_values = [random_gen(elem_shape)]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

    @pytest.mark.skip(
        reason="[NNv2 TensorArray scatter returns wrong result](rdar://63345281)"
    )
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_tf_while_loop(self, use_cpu_only, backend):
        @make_tf_graph([(3, 2)])
        def build_model(x):
            def body(i, num_iters, array, update):
                return i + 1, num_iters, array.write(i, update), update

            def cond(i, num_iters, array, update):
                return i < num_iters

            i = 0
            max_iters = 3
            ta = tf.TensorArray(dtype=tf.float32, size=1, dynamic_size=True)
            _, _, new_ta, _ = tf.while_loop(cond, body, [i, max_iters, ta, x])
            new_ta = new_ta.scatter([max_iters], tf.expand_dims(x, 0))

            return new_ta.stack()

        model, inputs, outputs = build_model
        input_values = [random_gen(shape=(3, 2))]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )


class TestBroadcastTo:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, shapes, is_dynamic",
        itertools.product(
            [True, False],
            backends,
            [
                ((2,), (2,)),
                ((1,), (10,)),
                ((3,), (3, 3)),
                ((1, 1), (1, 4)),
                ((1, 1, 5), (3, 4, 4, 4, 5)),
                ((3,), (1, 3, 2, 1, 3)),
                ((3, 5), (2, 3, 5)),
                ((1, 2), (2, 3, 1, 2)),
                ((1, 3, 1, 4), (8, 3, 32, 4)),
                ((2, 16), (3, 1, 4, 2, 16)),
            ],
            [False],
        ),
    )
    def test(self, use_cpu_only, backend, shapes, is_dynamic):
        input_shape, output_shape = shapes

        if is_dynamic is False:

            @make_tf_graph([input_shape])
            def build_model(x):
                return tf.broadcast_to(x, output_shape)

        else:  # output / target shape is an input (placeholder)

            @make_tf_graph([input_shape, (len(output_shape), tf.int32)])
            def build_model(x, shape):
                return tf.broadcast_to(x, shape)

        model, inputs, outputs = build_model
        if is_dynamic is False:
            input_values = [random_gen(input_shape)]
        else:
            input_values = [
                random_gen(input_shape),
                np.array(output_shape, dtype=np.int32),
            ]

        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model, input_dict, outputs, use_cpu_only=use_cpu_only, backend=backend
        )


class TestLSTMBlockCell:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, batch, return_hc_only, has_peephole, has_clip",
        itertools.product(
            [True, False],
            backends,
            [1, 2],
            [True, False],
            [True, False],
            [True, False],
        ),
    )
    def test_tf_no_variable(
        self, use_cpu_only, batch, backend, return_hc_only, has_peephole, has_clip
    ):
        """
        If return_hc_only == True, the op can be mapped to mb.lstm.
        Otherwise it has to be expanded.
        """
        # _lstm_block_cell allows fine-grained control of W, peephole etc
        from tensorflow.contrib.rnn.python.ops.lstm_ops import _lstm_block_cell

        actual_len, padded_len = 3, 4
        input_dim, hidden_dim = 2, 3
        x_shape = (batch, input_dim)
        init_h = np.random.rand(batch, hidden_dim).astype(np.float32)
        init_c = np.random.rand(batch, hidden_dim).astype(np.float32)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=x_shape)
            res = _lstm_block_cell(
                x,
                tf.constant(init_c),
                tf.constant(init_h),
                w=tf.constant(
                    np.random.rand(input_dim + hidden_dim, 4 * hidden_dim).astype(
                        np.float32
                    )
                ),
                b=tf.constant(np.random.rand(4 * hidden_dim).astype(np.float32)),
                use_peephole=has_peephole,
                wci=tf.constant(np.random.rand(hidden_dim).astype(np.float32)),
                wcf=tf.constant(np.random.rand(hidden_dim).astype(np.float32)),
                wco=tf.constant(np.random.rand(hidden_dim).astype(np.float32)),
                forget_bias=np.random.rand(),
                cell_clip=np.random.rand() if has_clip else -1,
            )
            if return_hc_only:
                # All other outputs aren't supported by mb.lstm.
                res = res[1], res[6]

            run_compare_tf(
                graph,
                {x: np.random.rand(*x_shape).astype(np.float32),},
                res,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )

    @pytest.mark.xfail(
        reason="Revert the assumption of invoking set_global before get_global: <rdar://problem/63326545>",
        run=False,
    )
    @pytest.mark.parametrize(
        "use_cpu_only, backend, batch",
        itertools.product([True, False], backends, [1, 2],),
    )
    def test_tf_lstm_block_cell(self, use_cpu_only, backend, batch):
        actual_len, padded_len = 3, 4
        input_dim, hidden_dim = 2, 3
        # [timelen, batch_size, num_inputs]
        x_shape = (batch, input_dim)
        init_h = np.random.rand(batch, hidden_dim).astype(np.float32)
        init_c = np.random.rand(batch, hidden_dim).astype(np.float32)
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=x_shape)
            rnn_cell = tf.contrib.rnn.LSTMBlockCell(
                hidden_dim, use_peephole=True, forget_bias=np.random.rand()
            )
            res = rnn_cell(x, (init_h, init_c))
            cs_new, h_new = res[1][0], res[1][1]
            res = [h_new, cs_new]

            run_compare_tf(
                graph,
                {x: np.random.rand(*x_shape).astype(np.float32),},
                res,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
                # variable needs to be frozen
                freeze_graph=True,
            )


class TestVariable:
    @pytest.mark.xfail(reason="Investigate get_global <rdar://62623458>", run=False)
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True], backends,)
    )
    def test_tf_no_variable(self, use_cpu_only, backend):
        with tf.Graph().as_default() as graph:
            x = tf.placeholder(tf.float32, shape=[1,], name="input")
            y = tf.Variable([1.0], dtype=tf.float32, name="y")

            # We set our assign op
            assign_op = tf.assign(y, y + 10)

            with tf.control_dependencies([assign_op]):
                res = tf.multiply(x, y, name="output")

            run_compare_tf(
                graph,
                {x: np.random.rand(1).astype(np.float32),},
                res,
                use_cpu_only=use_cpu_only,
                frontend_only=False,
                backend=backend,
            )


class TestZerosLike:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, dynamic",
        itertools.product(
            [True, False], backends, [rank for rank in range(5)], [True, False],
        ),
    )
    def test(self, use_cpu_only, backend, rank, dynamic):
        input_shape = np.random.randint(low=2, high=4, size=rank)
        input_value = random_gen(input_shape, rand_min=-1, rand_max=1)
        if dynamic:
            a, b = np.prod(input_shape[:2]), np.prod(input_shape[2:])
            reshape_vals = np.array([a, b], dtype=np.int32)
            reshape_input_shape = np.array([2], dtype=np.int32)

            @make_tf_graph([input_shape, list(reshape_input_shape) + [tf.int32]])
            def build_model(x, reshape):
                x = tf.reshape(x, shape=reshape)
                return tf.raw_ops.ZerosLike(x=x)

            model, inputs, outputs = build_model
            input_values = [input_value, reshape_vals]
        else:

            @make_tf_graph([input_shape])
            def build_model(x):
                return tf.raw_ops.ZerosLike(x=x)

            model, inputs, outputs = build_model
            input_values = [input_value]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestIsFinite:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, dynamic",
        itertools.product(
            [True, False], backends, [rank for rank in range(5)], [True, False]
        ),
    )
    def test(self, use_cpu_only, backend, rank, dynamic):
        def _generate_num_with_inf(input_shape):
            res = random_gen(input_shape, rand_min=-1, rand_max=1)
            random_map = np.random.choice([np.inf, -np.inf, 0], size=input_shape)
            if len(input_shape) == 0:
                return random_map.astype(np.float32)
            res[np.where(random_map == np.inf)] = np.inf
            res[np.where(random_map == -np.inf)] = -np.inf
            return res.astype(np.float32)

        input_shape = np.random.randint(low=2, high=4, size=rank)
        input_value = _generate_num_with_inf(input_shape)
        if dynamic:
            reshape_shape = [2, tf.int32]

            if len(input_shape) == 0:
                reshape_value = np.array([1, 1], dtype=np.int32)
            else:
                reshape_value = np.array(
                    [input_shape[0], np.prod(input_shape[1:])], dtype=np.int32
                )

            @make_tf_graph([input_shape, reshape_shape])
            def build_model(x, reshape):
                x = tf.reshape(x, reshape)
                x = tf.raw_ops.IsFinite(x=x)
                return tf.raw_ops.Cast(x=x, DstT=tf.float32)

            model, inputs, outputs = build_model
            input_values = [input_value, reshape_value]

        else:

            @make_tf_graph([input_shape])
            def build_model(x):
                x = tf.raw_ops.IsFinite(x=x)
                return tf.raw_ops.Cast(x=x, DstT=tf.float32)

            model, inputs, outputs = build_model
            input_values = [input_value]

        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

class TestLogSoftMax:
    @pytest.mark.parametrize('use_cpu_only, backend',
                             itertools.product(
                                 [True, False],
                                 backends,
                             ))
    def test(self, use_cpu_only, backend):
        input_shape = (5, 20)
        input_value = random_gen(input_shape, rand_min=-10, rand_max=10)
        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.math.log_softmax(x)

        model, inputs, outputs = build_model
        input_values = [input_value]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(model, input_dict, outputs,
                       use_cpu_only=use_cpu_only,
                       frontend_only=False, backend=backend)


class TestClipByValue:
    @pytest.mark.parametrize('use_cpu_only, backend, rank, min_and_max',
                             itertools.product(
                                 [True, False],
                                 backends,
                                 [rank for rank in range(5)],
                                 [(-1,1),(-1,-1),(1,2),(-3,-2)],
                             ))
    def test(self, use_cpu_only, backend, rank, min_and_max):
        input_shape = np.random.randint(low=2, high=4, size=rank)
        min_val, max_val = min_and_max
        input_value = random_gen(input_shape, rand_min=min_val-1, rand_max=max_val+1)
        @make_tf_graph([input_shape])
        def build_model(x):
            return tf.raw_ops.ClipByValue(t=x, clip_value_min=min_val, clip_value_max=max_val)

        model, inputs, outputs = build_model
        input_values = [input_value]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(model, input_dict, outputs,
                       use_cpu_only=use_cpu_only,
                       frontend_only=False, backend=backend)


class TestSize:
    @pytest.mark.parametrize('use_cpu_only, backend, rank, dynamic',
                             itertools.product(
                                 [True, False],
                                 backends,
                                 [rank for rank in range(5)],
                                 [True, False],
                             ))
    def test(self, use_cpu_only, backend, rank, dynamic):
        input_shape = np.random.randint(low=2, high=4, size=rank)
        input_value = random_gen(input_shape, rand_min=-1, rand_max=1)
        if dynamic:
            a, b = np.prod(input_shape[:2]), np.prod(input_shape[2:])
            reshape_vals = np.array([a,b], dtype=np.int32)
            reshape_input_shape = np.array([2], dtype=np.int32)

            @make_tf_graph([input_shape, list(reshape_input_shape)+[tf.int32]])
            def build_model(x, reshape):
                x = tf.reshape(x, shape=reshape)
                return tf.raw_ops.Size(input=x)

            model, inputs, outputs = build_model
            input_values = [input_value, reshape_vals]
        else:
            @make_tf_graph([input_shape])
            def build_model(x):
                return tf.raw_ops.Size(input=x)

            model, inputs, outputs = build_model
            input_values = [input_value]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(model, input_dict, outputs,
                       use_cpu_only=use_cpu_only,
                       frontend_only=False, backend=backend)
