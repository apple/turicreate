#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.mil import get_new_symbol, get_new_variadic_symbol
from coremltools.converters.mil.testing_reqs import *

from .testing_utils import UNK_SYM, UNK_VARIADIC, run_compare_builder

backends = testing_reqs.backends


class TestDepthToSpace:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        # original input type is (1, 4, 1, 1, fp32)
        val = np.array([[[[9.0]], [[5.0]], [[1.0]], [[3.0]]]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=val.shape)}
        input_values = {"x": val}

        def build(x):
            return [mb.depth_to_space(x=x, block_size=2)]

        expected_output_types = (1, 1, 2, 2, types.fp32)
        expected_outputs = np.array([[[[9.0, 5.0], [1.0, 3.0]]]], dtype=np.float32)

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


class TestExpandDims:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            return [
                mb.expand_dims(x=x, axes=[0]),
                mb.expand_dims(x=x, axes=[1]),
                mb.expand_dims(x=x, axes=[2]),
                mb.expand_dims(x=x, axes=[-1]),
                mb.expand_dims(x=x, axes=[0, 1]),
                mb.expand_dims(x=x, axes=[-2, -1]),
            ]

        expected_output_types = [
            (1, 2, 3, types.fp32),
            (2, 1, 3, types.fp32),
            (2, 3, 1, types.fp32),
            (2, 3, 1, types.fp32),
            (1, 1, 2, 3, types.fp32),
            (2, 3, 1, 1, types.fp32),
        ]
        expected_outputs = [
            np.array([[[1, 2, 3], [4, 5, 6]]], dtype=np.float32),
            np.array([[[1, 2, 3]], [[4, 5, 6]]], dtype=np.float32),
            np.array([[[1], [2], [3]], [[4], [5], [6]]], dtype=np.float32),
            np.array([[[1], [2], [3]], [[4], [5], [6]]], dtype=np.float32),
            np.array([[[[1, 2, 3], [4, 5, 6]]]], dtype=np.float32),
            np.array([[[[1]], [[2]], [[3]]], [[[4]], [[5]], [[6]]]], dtype=np.float32),
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

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_symbolic(self, use_cpu_only, backend):
        s0 = get_new_symbol()

        input_placeholders = {
            "x": mb.placeholder(shape=(2, s0)),
        }

        def build(x):
            return [
                mb.expand_dims(x=x, axes=[-1]),
                mb.expand_dims(x=x, axes=[1]),
            ]

        expected_output_types = [
            (2, s0, 1, types.fp32),
            (2, 1, s0, types.fp32),
        ]
        expected_outputs = [
            np.array([[[1], [2], [3]], [[4], [5], [6]]], dtype=np.float32),
            np.array([[[1, 2, 3]], [[4, 5, 6]]], dtype=np.float32),
        ]

        input_values = {
            "x": np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32),
        }
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
        x_val = np.random.rand(1, 6)
        v1 = mb.expand_dims(x=x_val, axes=[2])
        assert is_close(np.expand_dims(x_val, 2), v1.val)

        v2 = mb.expand_dims(x=x_val, axes=[-1])
        assert is_close(np.expand_dims(x_val, -1), v2.val)

        v3 = mb.expand_dims(x=x_val, axes=[-1, -2])
        ref = np.expand_dims(np.expand_dims(x_val, -1), -1)
        assert is_close(ref, v3.val)

        v4 = mb.expand_dims(x=x_val, axes=[0, -1, -2])
        assert is_close(np.reshape(x_val, (1, 1, 6, 1, 1)), v4.val)

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
    def test_builder_to_backend_programmatic_one_axis(
        self, use_cpu_only, backend, rank_and_axis
    ):
        rank, axis = rank_and_axis
        x_shape = np.random.randint(low=2, high=6, size=rank)
        input_placeholders = {"x": mb.placeholder(shape=x_shape)}
        input_values = {"x": np.random.sample(x_shape).astype(np.float32)}

        def build(x):
            return mb.expand_dims(x=x, axes=[axis])

        adjusted_axis = axis if axis >= 0 else rank + axis + 1
        x_shape = list(x_shape)
        out_shape = x_shape[:adjusted_axis] + [1] + x_shape[adjusted_axis:]
        expected_output_types = tuple(out_shape[:]) + (types.fp32,)

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            np.expand_dims(input_values["x"], axis),
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank_and_axes",
        itertools.product(
            [True, False],
            backends,
            [
                (3, [0, 1]),
                (3, [1, 0]),
                (3, [-2, -1]),
                (3, [-1, -2]),
                (2, [-3, -1]),
                (2, [-3, 1, -1]),
                (2, [-2, 0]),
                (1, [-1, -2, -3, -4]),
                (1, [0, -1]),
                (1, [0, 1, -2, -1]),
            ],
        ),
    )
    def test_builder_to_backend_programmatic_multiple_axes(
        self, use_cpu_only, backend, rank_and_axes
    ):
        rank, axes = rank_and_axes
        x_shape = np.random.randint(low=1, high=6, size=rank)
        input_placeholders = {"x": mb.placeholder(shape=x_shape)}
        input_values = {"x": np.random.sample(x_shape).astype(np.float32)}

        def build(x):
            return mb.expand_dims(x=x, axes=axes)

        out_shape = list(x_shape)
        out_rank = rank + len(axes)
        pos_axes = sorted([out_rank + axis if axis < 0 else axis for axis in axes])
        for axis in pos_axes:
            out_shape.insert(axis, 1)

        expected_outputs = np.reshape(input_values["x"], out_shape)
        expected_output_types = tuple(out_shape) + (types.fp32,)

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


class TestReshape:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            return [
                mb.reshape(x=x, shape=[3, 2]),
                mb.reshape(x=x, shape=[2, -1]),
                mb.reshape(x=x, shape=[2, 1, 1, 3]),
            ]

        expected_output_types = [
            (3, 2, types.fp32),
            (2, 3, types.fp32),
            (2, 1, 1, 3, types.fp32),
        ]
        expected_outputs = [
            np.array([[1, 2], [3, 4], [5, 6]], dtype=np.float32),
            np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32),
            np.array([[[[1.0, 2.0, 3.0]]], [[[4.0, 5.0, 6.0]]]], dtype=np.float32),
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
        t = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        r = mb.reshape(x=t, shape=[3, 2])
        expected_r = np.array([[1, 2], [3, 4], [5, 6]], dtype=np.float32)
        assert is_close(expected_r, r.val)
        r2 = mb.reshape(x=t, shape=[2, -1])
        expected_r2 = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        assert is_close(expected_r2, r2.val)

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_symbolic(self, use_cpu_only, backend):
        s0 = get_new_symbol()
        s_len = get_new_symbol()
        s1 = get_new_variadic_symbol()

        # Test variadic (rdar://59559656)
        input_placeholders = {
            "x": mb.placeholder(shape=(2, s0)),
            # TODO: variadic (rdar://59559656)
            # "x2": mb.placeholder(shape=(s1, 2)),
            "shape": mb.placeholder(shape=(3,), dtype=types.int32),
            "shape2": mb.placeholder(shape=(s_len,), dtype=types.int32),
        }

        def build(x, shape, shape2):
            return [
                mb.reshape(x=x, shape=[2, -1]),
                mb.reshape(x=x, shape=[1, -1]),
                mb.reshape(x=x, shape=[2, 1, 1, -1]),
                # TODO: variadic (rdar://59559656)
                # mb.reshape(x=x2, shape=[2, 1, 1]),
                mb.reshape(x=x, shape=shape),
                mb.reshape(x=x, shape=shape2),
            ]

        expected_output_types = [
            (2, s0, types.fp32),
            (1, 2 * s0, types.fp32),
            (2, 1, 1, s0, types.fp32),
            # TODO: variadic (rdar://59559656)
            # (2, 1, 1, types.fp32),
            (UNK_SYM, UNK_SYM, UNK_SYM, types.fp32),
            (UNK_VARIADIC, types.fp32),
        ]
        expected_outputs = [
            np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32),
            np.array([[1, 2, 3, 4, 5, 6]], dtype=np.float32),
            np.array([[[[1.0, 2.0, 3.0]]], [[[4.0, 5.0, 6.0]]]], dtype=np.float32),
            # TODO: variadic (rdar://59559656)
            # np.array([[1, 2, 3],
            #          [4, 5, 6]], dtype=np.float32),
            np.array([[[1, 2, 3]], [[4, 5, 6]]], dtype=np.float32),
            np.array([[[1, 2, 3]], [[4, 5, 6]]], dtype=np.float32),
        ]

        input_values = {
            "x": np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32),
            # TODO: variadic (rdar://59559656)
            # "x2": np.array([[[1, 2, 3],[4, 5, 6]]], dtype=np.float32),
            "shape": np.array([2, 1, 3], dtype=np.float32),
            "shape2": np.array([2, 1, 3], dtype=np.float32),
        }
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


class TestReverse:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        val = np.array([[-1.0, 2.0, -3.0], [4.0, -5.0, 6.0]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=val.shape)}
        input_values = {"x": val}

        def build(x):
            return [mb.reverse(x=x), mb.reverse(x=x, axes=[0])]

        expected_output_types = [(2, 3, types.fp32), (2, 3, types.fp32)]
        expected_outputs = [
            np.array([[6.0, -5.0, 4.0], [-3.0, 2.0, -1.0]], dtype=np.float32),
            np.array([[4.0, -5.0, 6.0], [-1.0, 2.0, -3.0]], dtype=np.float32),
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
        val = np.array([[-1.0, 7.0, -3.0], [4.0, -5.0, 8.0]], dtype=np.float32)
        res = mb.reverse(x=val, axes=[0])
        assert is_close(np.flip(val, axis=0), res.val)

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_symbolic(self, use_cpu_only, backend):
        s0 = get_new_symbol()

        val = np.array([[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=(s0, 3))}
        input_values = {"x": val}

        def build(x):
            return [
                mb.reverse(x=x, axes=[1]),
                mb.reverse(x=x, axes=[0]),
            ]

        expected_output_types = [
            (s0, 3, types.fp32),
            (s0, 3, types.fp32),
        ]
        expected_outputs = [
            np.array([[3.0, 2.0, 1.0], [6.0, 5.0, 4.0]], dtype=np.float32),
            np.array([[4.0, 5.0, 6.0], [1.0, 2.0, 3.0]], dtype=np.float32),
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


class TestReverseSequence:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array(
            [
                [1, 2, 3, 4, 5, 0, 0, 0],
                [1, 2, 0, 0, 0, 0, 0, 0],
                [1, 2, 3, 4, 0, 0, 0, 0],
                [1, 2, 3, 4, 5, 6, 7, 8],
            ],
            dtype=np.float32,
        )
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [
                mb.reverse_sequence(
                    x=x, lengths=[7, 2, 3, 5], seq_axis=1, batch_axis=0
                ),
            ]

        expected_output_types = [
            (4, 8, types.fp32),
        ]
        expected_outputs = [
            np.array(
                [
                    [0, 0, 5, 4, 3, 2, 1, 0],
                    [2, 1, 0, 0, 0, 0, 0, 0],
                    [3, 2, 1, 4, 0, 0, 0, 0],
                    [5, 4, 3, 2, 1, 6, 7, 8],
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

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_symbolic(self, use_cpu_only, backend):
        s0 = get_new_symbol()

        x_val = np.array(
            [
                [1, 2, 3, 4, 5, 0, 0, 0],
                [1, 2, 0, 0, 0, 0, 0, 0],
                [1, 2, 3, 4, 0, 0, 0, 0],
                [1, 2, 3, 4, 5, 6, 7, 8],
            ],
            dtype=np.float32,
        )
        input_placeholders = {"x": mb.placeholder(shape=(4, s0))}
        input_values = {"x": x_val}

        def build(x):
            return [
                mb.reverse_sequence(
                    x=x, lengths=[7, 2, 3, 5], seq_axis=1, batch_axis=0
                ),
            ]

        expected_output_types = [
            (4, s0, types.fp32),
        ]
        expected_outputs = [
            np.array(
                [
                    [0, 0, 5, 4, 3, 2, 1, 0],
                    [2, 1, 0, 0, 0, 0, 0, 0],
                    [3, 2, 1, 4, 0, 0, 0, 0],
                    [5, 4, 3, 2, 1, 6, 7, 8],
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


class TestSliceBySize:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array(list(range(24))).reshape((2, 3, 4)).astype(np.float32)
        begin_val = np.array([1, 1, 1], dtype=np.int32)
        input_placeholders = {
            "x": mb.placeholder(shape=x_val.shape),
            "begin": mb.placeholder(shape=begin_val.shape, dtype=types.int32),
        }
        input_values = {"x": x_val, "begin": begin_val}

        def build_non_single(x, begin):
            return [
                mb.slice_by_size(x=x, begin=begin, size=[1, 2, 3]),
            ]

        def build_single(x, begin):
            return [
                mb.slice_by_size(x=x, begin=begin, size=[-1, 2, -1]),
            ]

        expected_output_types = [(1, 2, 3, types.fp32)]
        expected_outputs = [np.array([[[17, 18, 19], [21, 22, 23]]], dtype=np.float32)]
        run_compare_builder(
            build_non_single,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

        expected_output_types = [(UNK_SYM, 2, UNK_SYM, types.fp32)]
        run_compare_builder(
            build_single,
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
        x = np.array(list(range(24))).reshape(2, 3, 4)
        v_1 = mb.slice_by_size(x=x, begin=(0, 1, 0), size=(-1, -1, -1))
        v_2 = mb.slice_by_size(x=x, begin=(0, 1, 0), size=(-1, -1, 3))
        v_3 = mb.slice_by_size(x=x, begin=(0, -2, 0), size=(-1, -1, 3))
        assert is_close(x[:, 1:, :], v_1.val)
        assert is_close(x[:, 1:, :3], v_2.val)
        assert is_close(x[:, -2:, :3], v_3.val)


class TestSpaceToDepth:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        # original input type is (1, 1, 2, 2, fp32)
        val = np.array([[[[7.0, 9.0], [4.0, 6.0]]]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=val.shape)}
        input_values = {"x": val}

        def build(x):
            return [mb.space_to_depth(x=x, block_size=2)]

        expected_output_types = (1, 4, 1, 1, types.fp32)
        expected_outputs = np.array(
            [[[[7.0]], [[9.0]], [[4.0]], [[6.0]]]], dtype=np.float32
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


class TestSqueeze:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x = np.array([[[[1], [2], [3]]]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=x.shape)}

        input_values = {"x": x}

        def build(x):
            return [
                mb.squeeze(x=x, axes=(-1,)),
                mb.squeeze(x=x, axes=(-3, 0)),
                mb.squeeze(x=x, axes=(0, 1, 3)),
                mb.squeeze(x=x),
            ]

        expected_output_types = [
            (1, 1, 3, types.fp32),
            (3, 1, types.fp32),
            (3, types.fp32),
            (3, types.fp32),
        ]

        expected_outputs = [
            np.array([[[1, 2, 3]]], dtype=np.float32),
            np.array([[1], [2], [3]], dtype=np.float32),
            np.array([1, 2, 3], dtype=np.float32),
            np.array([1, 2, 3], dtype=np.float32),
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
        x = np.array([[[[1], [2], [3]], [[4], [5], [6]]]], dtype=np.float32)
        v = mb.squeeze(x=x, axes=(-4, 3))
        assert is_close(np.squeeze(x, axis=(-4, 3)), v.val)


class TestTranspose:
    @pytest.mark.parametrize(
        argnames=["use_cpu_only", "backend", "is_symbolic"],
        argvalues=itertools.product([True, False], backends, [True, False],),
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend, is_symbolic):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)

        input_shape = x.shape
        # is_symbolic = False
        if is_symbolic:
            input_shape = [get_new_symbol(), get_new_symbol()]

        input_placeholders = {"x": mb.placeholder(shape=input_shape)}

        input_values = {"x": x}

        def build(x):
            return [
                mb.transpose(x=x, perm=(0, 1)),
                mb.transpose(x=x, perm=(1, 0)),
                mb.transpose(x=x, perm=(-1, 0)),
                mb.transpose(x=x, perm=(-2, -1)),
            ]

        d0 = input_shape[0]
        d1 = input_shape[1]
        expected_output_types = [
            (d0, d1, types.fp32),
            (d1, d0, types.fp32),
            (d1, d0, types.fp32),
            (d0, d1, types.fp32),
        ]

        expected_outputs = [x, x.T, x.T, x]

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
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        v = mb.transpose(x=x, perm=(1, 0))
        assert is_close(x.T, v.val)

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_symbolic(self, use_cpu_only, backend):
        s0 = get_new_symbol()

        # Test variadic (rdar://59559656)
        input_placeholders = {
            "x": mb.placeholder(shape=(2, s0)),
        }

        def build(x):
            return [
                mb.transpose(x=x, perm=[1, 0]),
            ]

        expected_output_types = [
            (s0, 2, types.fp32),
        ]
        expected_outputs = [
            np.array([[1, 4], [2, 5], [3, 6]], dtype=np.float32),
        ]

        input_values = {
            "x": np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32),
        }
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


class TestPixelShuffle:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        # original input type is (1, 4, 1, 1, fp32)
        val = np.array([[[[9.0]], [[5.0]], [[1.0]], [[3.0]]]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=val.shape)}
        input_values = {"x": val}

        def build(x):
            return [mb.pixel_shuffle(x=x, upscale_factor=2)]

        expected_output_types = (1, 1, 2, 2, types.fp32)
        expected_outputs = np.array([[[[9.0, 5.0], [1.0, 3.0]]]], dtype=np.float32)

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
        "use_cpu_only, backend, shape, upscale_factor",
        itertools.product(
            [True, False],
            backends,
            [(1, 16, 1, 1), (2, 16, 3, 3), (1, 32, 1, 1)],
            [2, 4],
        ),
    )
    def test_builder_to_backend_stress(
        self, use_cpu_only, backend, shape, upscale_factor
    ):
        val = np.random.rand(*shape)
        input_placeholders = {"x": mb.placeholder(shape=val.shape)}
        input_values = {"x": val}

        def build(x):
            return [mb.pixel_shuffle(x=x, upscale_factor=upscale_factor)]

        torch_pixel_shuffle = torch.nn.PixelShuffle(upscale_factor)
        expected_outputs = [torch_pixel_shuffle(torch.Tensor(val)).numpy()]
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


class TestSlidingWindows:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        # original input type is (1, 4, 1, 1, fp32)
        val = np.array([[[[9.0]], [[5.0]], [[1.0]], [[3.0]]]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=val.shape)}
        input_values = {"x": val}

        def build(x):
            return [mb.sliding_windows(x=x, axis=1, size=2)]

        expected_output_types = (1, 3, 2, 1, 1, types.fp32)
        expected_outputs = np.array(
            [[[[[9.0]], [[5.0]]], [[[5.0]], [[1.0]]], [[[1.0]], [[3.0]]]]],
            dtype=np.float32,
        )

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
        "use_cpu_only, backend, rank_and_axis, size, stride",
        itertools.product(
            [True, False],
            backends,
            [(rank, axis) for rank in range(1, 5) for axis in range(-rank, rank)],
            [1, 2],
            [1, 2],
        ),
    )
    def test_builder_to_backend_stress(
        self, use_cpu_only, backend, rank_and_axis, size, stride
    ):
        def np_sliding_windows(a, np_axis, np_size, np_stride):
            n = (a.shape[np_axis] - np_size) // np_stride + 1
            x_shape = list(a.shape)
            x_shape[np_axis] = n
            if np_axis < 0:
                np_axis += len(x_shape)
            x_shape.insert(np_axis + 1, np_size)
            strides = list(a.strides)
            eff_stride = strides[np_axis] * np_stride
            strides.insert(np_axis, eff_stride)
            return np.lib.stride_tricks.as_strided(a, x_shape, strides)

        rank, axis = rank_and_axis
        shape = np.random.randint(low=2, high=5, size=rank)
        val = np.random.rand(*shape)
        input_placeholders = {"x": mb.placeholder(shape=val.shape)}
        input_values = {"x": val}

        def build(x):
            return [mb.sliding_windows(x=x, axis=axis, size=size, stride=stride)]

        expected_outputs = [
            np_sliding_windows(val, np_axis=axis, np_size=size, np_stride=stride)
        ]
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
