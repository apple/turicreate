#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.testing_reqs import *

from .testing_utils import run_compare_builder

backends = testing_reqs.backends


class TestScatter:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        data = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        indices = np.array([1, 0], dtype=np.int32)
        updates = np.array([[5, 6, 7], [8, 9, 10]], dtype=np.float32)
        input_placeholders = {
            "data": mb.placeholder(shape=data.shape),
            "indices": mb.placeholder(shape=indices.shape, dtype=types.int32),
            "updates": mb.placeholder(shape=updates.shape),
        }

        input_values = {"data": data, "indices": indices, "updates": updates}

        def build(data, indices, updates):
            return (mb.scatter(data=data, indices=indices, updates=updates),)

        expected_output_types = (2, 3, types.fp32)

        expected_outputs = np.array([[9, 11, 13], [9, 11, 13]], dtype=np.float32)

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

    @pytest.mark.skipif(not testing_reqs._HAS_TF_1, reason=MSG_TF1_NOT_FOUND)
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rankData_rankIndices, accumulate_mode",
        itertools.product(
            [True, False],
            backends,
            [
                (1, 2),
                (2, 1),
                (3, 2),
                (2, 3),
                (2, 2),
                (1, 1),
                (3, 3),
                (3, 3),
                (3, 3),
                (1, 3),
                (3, 1),
                (3, 1),
            ],
            ["update", "add", "sub", "mul", "div", "max", "min"],
        ),
    )
    def test_builder_to_backend_programmatic(
        self, use_cpu_only, backend, rankData_rankIndices, accumulate_mode
    ):
        data_rank, indices_rank = rankData_rankIndices
        data_shape = np.random.randint(low=2, high=5, size=data_rank)
        indices_shape = np.random.randint(low=2, high=5, size=indices_rank)
        updates_shape = list(indices_shape) + list(data_shape[1:])

        data = np.random.rand(*data_shape).astype(np.float32)
        updates = np.random.rand(*updates_shape).astype(np.float32)
        indices = np.random.randint(0, data_shape[0], size=indices_shape).astype(
            np.int32
        )

        def build(data, indices, updates):
            return mb.scatter(
                data=data, indices=indices, updates=updates, mode=accumulate_mode
            )

        with tf.Graph().as_default(), tf.Session() as sess:
            tf_output = tf.Variable(data)
            sess.run(tf.global_variables_initializer())
            if accumulate_mode == "update":
                sess.run(tf.scatter_update(tf_output, indices, updates))
            if accumulate_mode == "add":
                sess.run(tf.scatter_add(tf_output, indices, updates))
            if accumulate_mode == "sub":
                sess.run(tf.scatter_sub(tf_output, indices, updates))
            if accumulate_mode == "mul":
                sess.run(tf.scatter_mul(tf_output, indices, updates))
            if accumulate_mode == "div":
                sess.run(tf.scatter_div(tf_output, indices, updates))
            if accumulate_mode == "max":
                sess.run(tf.scatter_max(tf_output, indices, updates))
            if accumulate_mode == "min":
                sess.run(tf.scatter_min(tf_output, indices, updates))
            expected_output = sess.run(tf_output)

        input_placeholders = {
            "data": mb.placeholder(shape=data.shape),
            "indices": mb.placeholder(shape=indices.shape, dtype=types.int32),
            "updates": mb.placeholder(shape=updates.shape),
        }

        input_values = {"data": data, "indices": indices, "updates": updates}

        expected_output_types = tuple(data_shape[:]) + (types.fp32,)
        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_output,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestScatterAlongAxis:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        data = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        indices = np.array([[1, 0, 1], [1, 1, 0]], dtype=np.int32)
        updates = np.array([[5, 6, 7], [8, 9, 10]], dtype=np.float32)
        input_placeholders = {
            "data": mb.placeholder(shape=data.shape),
            "indices": mb.placeholder(shape=indices.shape, dtype=types.int32),
            "updates": mb.placeholder(shape=updates.shape),
        }

        input_values = {"data": data, "indices": indices, "updates": updates}

        def build(data, indices, updates):
            return mb.scatter_along_axis(
                data=data, indices=indices, updates=updates, axis=0, mode="update"
            )

        expected_output_types = (2, 3, types.fp32)

        expected_outputs = np.array([[1, 6, 10], [8, 9, 7]], dtype=np.float32)

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
        indices = np.array([[1, 0, 1], [1, 1, 0]], dtype=np.int32)
        updates = np.array([[5, 6, 7], [8, 9, 10]], dtype=np.float32)
        v = mb.scatter_along_axis(
            data=x, indices=indices, updates=updates, axis=0, mode="update"
        )
        assert is_close(np.array([[1, 6, 10], [8, 9, 7]], dtype=np.float32), v.val)

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank_axis",
        itertools.product(
            [True, False],
            backends,
            [(rank, axis) for rank in range(1, 5) for axis in range(-rank, rank)],
        ),
    )
    def test_builder_to_backend_programmatic(self, use_cpu_only, backend, rank_axis):
        rank, axis = rank_axis
        data_shape = np.random.randint(low=2, high=8, size=rank)
        indices_shape = np.copy(data_shape)
        indices_shape[axis] = np.random.randint(low=1, high=8)
        updates_shape = indices_shape

        data = np.random.rand(*data_shape).astype(np.float32)
        updates = np.random.rand(*updates_shape).astype(np.float32)
        indices = np.random.randint(
            -data_shape[axis], data_shape[axis], size=indices_shape
        ).astype(np.int32)

        def build(data, indices, updates):
            return mb.scatter_along_axis(
                data=data, indices=indices, updates=updates, axis=axis, mode="update"
            )

        input_placeholders = {
            "data": mb.placeholder(shape=data.shape),
            "indices": mb.placeholder(shape=indices.shape, dtype=types.int32),
            "updates": mb.placeholder(shape=updates.shape),
        }

        input_values = {"data": data, "indices": indices, "updates": updates}

        expected_output_types = tuple(data_shape[:]) + (types.fp32,)

        np_output = np.copy(data)
        np.put_along_axis(np_output, indices, updates, axis=axis)

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            np_output,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestScatterNd:
    # TODO: <rdar://problem/59737282> [MIL] Scatter and ScatterNd in tensoflow
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        data = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        indices = np.array([[1, 0], [0, 2]], dtype=np.int32)
        updates = np.array([5, 10], dtype=np.float32)
        input_placeholders = {
            "data": mb.placeholder(shape=data.shape),
            "indices": mb.placeholder(shape=indices.shape, dtype=types.int32),
            "updates": mb.placeholder(shape=updates.shape),
        }

        input_values = {"data": data, "indices": indices, "updates": updates}

        def build(data, indices, updates):
            return (mb.scatter_nd(data=data, indices=indices, updates=updates),)

        expected_output_types = (2, 3, types.fp32)

        expected_outputs = np.array([[1, 2, 13], [9, 5, 6]], dtype=np.float32)

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

    @pytest.mark.skipif(not testing_reqs._HAS_TF_1, reason=MSG_TF1_NOT_FOUND)
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rankData_rankIndices, accumulate_mode",
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
            ["update", "add", "sub"],
        ),
    )
    def test_builder_to_backend_programmatic(
        self, use_cpu_only, backend, rankData_rankIndices, accumulate_mode
    ):
        data_rank, indices_rank = rankData_rankIndices
        data_shape = np.random.randint(low=2, high=5, size=data_rank)
        indices_shape = np.random.randint(low=2, high=5, size=indices_rank)
        indices_shape[-1] = np.random.randint(low=1, high=data_rank + 1)
        updates_shape = list(indices_shape[:-1]) + list(data_shape[indices_shape[-1] :])

        data = np.random.rand(*data_shape).astype(np.float32)
        updates = np.random.rand(*updates_shape).astype(np.float32)
        indices_list = []
        for i in range(indices_shape[-1]):
            indices_list.append(
                np.random.randint(0, data_shape[i], size=indices_shape[:-1])
            )

        indices = np.stack(indices_list, axis=-1).astype(np.int32)

        def build(data, indices, updates):
            return mb.scatter_nd(
                data=data, indices=indices, updates=updates, mode=accumulate_mode
            )

        with tf.Graph().as_default(), tf.Session() as sess:
            tf_output = tf.Variable(data)
            sess.run(tf.global_variables_initializer())
            if accumulate_mode == "update":
                sess.run(tf.scatter_nd_update(tf_output, indices, updates))
            if accumulate_mode == "add":
                sess.run(tf.scatter_nd_add(tf_output, indices, updates))
            if accumulate_mode == "sub":
                sess.run(tf.scatter_nd_sub(tf_output, indices, updates))
            expected_output = sess.run(tf_output)

        input_placeholders = {
            "data": mb.placeholder(shape=data.shape),
            "indices": mb.placeholder(shape=indices.shape, dtype=types.int32),
            "updates": mb.placeholder(shape=updates.shape),
        }

        input_values = {"data": data, "indices": indices, "updates": updates}

        expected_output_types = tuple(data_shape[:]) + (types.fp32,)
        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_output,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestGather:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        indices = np.array([1, 0], dtype=np.int32)
        input_placeholders = {
            "x": mb.placeholder(shape=x.shape),
            "indices": mb.placeholder(shape=indices.shape, dtype=types.int32),
        }

        input_values = {"x": x, "indices": indices}

        def build(x, indices):
            return [
                mb.gather(x=x, indices=indices, axis=0),
                mb.gather(x=x, indices=indices, axis=1),
                mb.gather(x=x, indices=indices, axis=-2),
                mb.gather(x=x, indices=indices, axis=-1),
                mb.gather(x=x, indices=indices),
            ]

        expected_output_types = [
            (2, 3, types.fp32),
            (2, 2, types.fp32),
            (2, 3, types.fp32),
            (2, 2, types.fp32),
            (2, 3, types.fp32),
        ]

        expected_outputs = [
            np.array([[4, 5, 6], [1, 2, 3]], dtype=np.float32),
            np.array([[2, 1], [5, 4]], dtype=np.float32),
            np.array([[4, 5, 6], [1, 2, 3]], dtype=np.float32),
            np.array([[2, 1], [5, 4]], dtype=np.float32),
            np.array([[4, 5, 6], [1, 2, 3]], dtype=np.float32),
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
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        indices = np.array([1, 0], dtype=np.int32)
        v = mb.gather(x=x, indices=indices, axis=-1)
        assert is_close(np.array([[2, 1], [5, 4]], dtype=np.float32), v.val)


class TestGatherAlongAxis:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        indices = np.array([[1, 0, 1], [1, 1, 0]], dtype=np.int32)
        input_placeholders = {
            "x": mb.placeholder(shape=x.shape),
            "indices": mb.placeholder(shape=indices.shape, dtype=types.int32),
        }

        input_values = {"x": x, "indices": indices}

        def build(x, indices):
            return [
                mb.gather_along_axis(x=x, indices=indices, axis=0),
                mb.gather_along_axis(x=x, indices=indices, axis=1),
                mb.gather_along_axis(x=x, indices=indices, axis=-2),
                mb.gather_along_axis(x=x, indices=indices, axis=-1),
                mb.gather_along_axis(x=x, indices=indices),
            ]

        expected_output_types = [
            (2, 3, types.fp32),
            (2, 3, types.fp32),
            (2, 3, types.fp32),
            (2, 3, types.fp32),
            (2, 3, types.fp32),
        ]

        expected_outputs = [
            np.array([[4, 2, 6], [4, 5, 3]], dtype=np.float32),
            np.array([[2, 1, 2], [5, 5, 4]], dtype=np.float32),
            np.array([[4, 2, 6], [4, 5, 3]], dtype=np.float32),
            np.array([[2, 1, 2], [5, 5, 4]], dtype=np.float32),
            np.array([[4, 2, 6], [4, 5, 3]], dtype=np.float32),
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
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        indices = np.array([[1, 0, 1], [0, 0, 1]], dtype=np.int32)
        v = mb.gather_along_axis(x=x, indices=indices, axis=0)
        assert is_close(np.array([[4, 2, 6], [1, 2, 6]], dtype=np.float32), v.val)

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank_axis",
        itertools.product(
            [True, False],
            backends,
            [(rank, axis) for rank in range(1, 5) for axis in range(-rank, rank)],
        ),
    )
    def test_builder_to_backend_programmatic(self, use_cpu_only, backend, rank_axis):
        rank, axis = rank_axis
        x_shape = np.random.randint(low=2, high=8, size=rank)
        indices_shape = np.copy(x_shape)
        indices_shape[axis] = np.random.randint(low=1, high=8)

        x = np.random.rand(*x_shape).astype(np.float32)
        indices = np.random.randint(
            -x_shape[axis], x_shape[axis], size=indices_shape
        ).astype(np.int32)

        def build(x, indices):
            return mb.gather_along_axis(x=x, indices=indices, axis=axis)

        input_placeholders = {
            "x": mb.placeholder(shape=x.shape),
            "indices": mb.placeholder(shape=indices.shape, dtype=types.int32),
        }

        input_values = {"x": x, "indices": indices}

        expected_output_types = tuple(indices_shape[:]) + (types.fp32,)
        expected_output = np.take_along_axis(x, indices, axis=axis)

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_output,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestGatherNd:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        indices = np.array([[1, 0], [0, 2]], dtype=np.int32)
        input_placeholders = {
            "x": mb.placeholder(shape=x.shape),
            "indices": mb.placeholder(shape=indices.shape, dtype=types.int32),
        }

        input_values = {"x": x, "indices": indices}

        def build(x, indices):
            return (mb.gather_nd(x=x, indices=indices),)

        expected_output_types = (2, types.fp32)
        expected_outputs = np.array([4, 3], dtype=np.float32)

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
