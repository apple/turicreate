#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.mil import get_new_symbol
from coremltools.converters.mil.testing_reqs import *

from .testing_utils import UNK_SYM, UNK_VARIADIC, run_compare_builder

backends = testing_reqs.backends


class TestBandPart:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array(
            [
                [3.0, 3.0, 5.0, 1.0],
                [5.0, 6.0, 3.0, 8.0],
                [7.0, 2.0, 7.0, 2.0],
                [6.0, 7.0, 7.0, 1.0],
            ],
            dtype=np.float32,
        )
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [
                mb.band_part(x=x),
                mb.band_part(x=x, lower=0, upper=-1),
                mb.band_part(x=x, lower=-1, upper=0),
                mb.band_part(x=x, lower=0, upper=0),
            ]

        expected_output_types = [
            (4, 4, types.fp32),
            (4, 4, types.fp32),
            (4, 4, types.fp32),
            (4, 4, types.fp32),
        ]

        expected_outputs = [
            np.array(
                [
                    [3.0, 3.0, 5.0, 1.0],
                    [5.0, 6.0, 3.0, 8.0],
                    [7.0, 2.0, 7.0, 2.0],
                    [6.0, 7.0, 7.0, 1.0],
                ],
                dtype=np.float32,
            ),
            np.array(
                [
                    [3.0, 3.0, 5.0, 1.0],
                    [0.0, 6.0, 3.0, 8.0],
                    [0.0, 0.0, 7.0, 2.0],
                    [0.0, 0.0, 0.0, 1.0],
                ],
                dtype=np.float32,
            ),
            np.array(
                [
                    [3.0, 0.0, 0.0, 0.0],
                    [5.0, 6.0, 0.0, 0.0],
                    [7.0, 2.0, 7.0, 0.0],
                    [6.0, 7.0, 7.0, 1.0],
                ],
                dtype=np.float32,
            ),
            np.array(
                [
                    [3.0, 0.0, 0.0, 0.0],
                    [0.0, 6.0, 0.0, 0.0],
                    [0.0, 0.0, 7.0, 0.0],
                    [0.0, 0.0, 0.0, 1.0],
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
            frontend_only=False,
            backend=backend,
        )


class TestCumSum:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            return mb.cumsum(x=x, axis=0, reverse=True, exclusive=False)

        expected_output_types = (2, 3, types.fp32)
        expected_outputs = np.array([[5, 7, 9], [4, 5, 6]], dtype=np.float32)

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
        x_val = random_gen(shape=(1, 2, 3, 4, 5), rand_min=-100, rand_max=100)
        v = mb.cumsum(x=x_val)
        assert is_close(np.cumsum(x_val, axis=0), v.val)

    @ssa_fn
    def test_invalid_axis1(self):
        x_val = random_gen(shape=(1, 2, 3, 4, 5), rand_min=-100, rand_max=100)
        with pytest.raises(ValueError):
            mb.cumsum(x=x_val, axis=-2)

    @ssa_fn
    def test_invalid_axis2(self):
        x_val = random_gen(shape=(1, 2, 3, 4, 5), rand_min=-100, rand_max=100)
        with pytest.raises(ValueError):
            mb.cumsum(x=x_val, axis=len(x_val.shape))

    @ssa_fn
    def test_invalid_axis3(self):
        x_val = random_gen(shape=(1, 2, 3, 4, 5), rand_min=-100, rand_max=100)
        with pytest.raises(TypeError):
            mb.cumsum(x=x_val, axis="")

    @ssa_fn
    def test_invalid_reverse1(self):
        x_val = random_gen(shape=(1, 2, 3, 4, 5), rand_min=-100, rand_max=100)
        with pytest.raises(TypeError):
            mb.cumsum(x=x_val, reverse="")

    @ssa_fn
    def test_invalid_reverse2(self):
        x_val = random_gen(shape=(1, 2, 3, 4, 5), rand_min=-100, rand_max=100)
        with pytest.raises(TypeError):
            pred = mb.cumsum(x=x_val, reverse=0)

    @ssa_fn
    def test_invalid_reverse3(self):
        x_val = random_gen(shape=(1, 2, 3, 4, 5), rand_min=-100, rand_max=100)
        with pytest.raises(TypeError):
            pred = mb.cumsum(x=x_val, reverse=1)

    @ssa_fn
    def test_invalid_exclusive1(self):
        x_val = random_gen(shape=(1, 2, 3, 4, 5), rand_min=-100, rand_max=100)
        with pytest.raises(TypeError):
            pred = mb.cumsum(x=x_val, exclusive="")

    @ssa_fn
    def test_invalid_exclusive2(self):
        x_val = random_gen(shape=(1, 2, 3, 4, 5), rand_min=-100, rand_max=100)
        with pytest.raises(TypeError):
            pred = mb.cumsum(x=x_val, exclusive=0)

    @ssa_fn
    def test_invalid_exclusive3(self):
        x_val = random_gen(shape=(1, 2, 3, 4, 5), rand_min=-100, rand_max=100)
        with pytest.raises(TypeError):
            pred = mb.cumsum(x=x_val, exclusive=1)

    @ssa_fn
    def test_invalid_input1(self):
        x_val = 1
        with pytest.raises(TypeError):
            pred = mb.cumsum(x=x_val)

    @ssa_fn
    def test_invalid_input2(self):
        x_val = ["1"]
        with pytest.raises(TypeError):
            pred = mb.cumsum(x=x_val)


class TestFill:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        shape = (2, 1, 3)
        x_val = np.zeros(shape=shape, dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}

        input_values = {"x": x_val}

        def build(x):
            return mb.add(x=x, y=mb.fill(shape=shape, value=1.0))

        expected_output_types = [(2, 1, 3, types.fp32)]
        expected_outputs = [np.full(shape=shape, fill_value=1.0)]

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
        shape = np.random.randint(low=1, high=3, size=5).astype(np.int32)
        res = mb.fill(shape=shape, value=1991.0).val
        assert is_close(np.full(shape, fill_value=1991.0), res)

    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, value",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(1, 6)],
            [-1917.0, 0.0, 2048.0],
        ),
    )
    def test_builder_to_backend_stress(self, use_cpu_only, backend, rank, value):
        shape = np.random.randint(low=1, high=4, size=rank).astype(np.int32)
        x_val = np.zeros(shape=shape, dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return mb.add(x=x, y=mb.fill(shape=shape, value=value))

        expected_outputs = [np.full(shape=shape, fill_value=value)]
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

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_symbolic(self, use_cpu_only, backend):
        # Test variadic (rdar://59559656)

        s_len = get_new_symbol()
        input_placeholders = {
            "shape": mb.placeholder(shape=(s_len,), dtype=types.int32),
        }

        def build(shape):
            return [mb.fill(shape=shape)]

        expected_output_types = [(UNK_VARIADIC, types.fp32)]
        expected_outputs = [np.zeros(shape=(2, 1, 3), dtype=np.float32)]
        input_values = {"shape": np.array([2, 1, 3], dtype=np.float32)}

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs,
            use_cpu_only=use_cpu_only,
            backend=backend,
        )


class TestNonMaximumSuppression:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        boxes_val = np.array(
            [
                [
                    [0.0, 0.0, 0.0, 0.0],
                    [1.0, 1.0, 1.0, 1.0],
                    [2.0, 2.0, 2.0, 2.0],
                    [3.0, 3.0, 3.0, 3.0],
                ]
            ],
            dtype=np.float32,
        )
        scores_val = np.array([[[-3.5], [9.4], [2.3], [0.7]]], dtype=np.float32)
        input_placeholders = {
            "boxes": mb.placeholder(shape=(1, 4, 4)),
            "scores": mb.placeholder(shape=(1, 4, 1)),
        }
        input_values = {"boxes": boxes_val, "scores": scores_val}

        expected_output_types = [
            (1, 2, 4, types.fp32),
            (1, 2, 1, types.fp32),
            (1, 2, types.int32),
            (1, types.int32),
        ]
        expected_outputs = [
            np.array([[[1.0, 1.0, 1.0, 1.0], [2.0, 2.0, 2.0, 2.0]]], dtype=np.float32),
            np.array([[[9.4], [2.3]]], dtype=np.float32),
            np.array([[1, 2]], dtype=np.int32),
            np.array([2], dtype=np.int32),
        ]

        def build(boxes, scores):
            return mb.non_maximum_suppression(
                boxes=boxes,
                scores=scores,
                iou_threshold=0.2,
                score_threshold=0.4,
                max_boxes=2,
                per_class_suppression=True,
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

    @staticmethod
    def _compute_iou_matrix(boxes):
        # input is (N, 4), in order [center_w, center_h, width, height]
        boxes = boxes.astype(np.float)
        center_w, center_h, width, height = np.split(boxes, 4, axis=1)
        top = center_h + 0.5 * height
        bottom = center_h - 0.5 * height
        left = center_w - 0.5 * width
        right = center_w + 0.5 * width
        area = width * height

        h_b = np.minimum(top, np.transpose(top))
        w_b = np.minimum(right, np.transpose(right))
        h_a = np.maximum(bottom, np.transpose(bottom))
        w_a = np.maximum(left, np.transpose(left))

        intersection_area = np.maximum(0, h_b - h_a) * np.maximum(0, w_b - w_a)
        union_area = area + np.transpose(area) - intersection_area
        return intersection_area / union_area

    @staticmethod
    def _ref_non_maximum_suppression(
        boxes, scores, iou_threshold, score_threshold, max_boxes, per_class_suppression
    ):
        """
        Reference implementation of Core ML's NMS op using TensorFlow.
        boxes of shape (n_batch, n_box, 4), [center_w, center_h, width, height]
        scores of shape (n_batch, n_box, n_score)
        output shapes [
           (n_batch, max_boxes, 4),
           (n_batch, max_boxes, n_score),
           (n_batch, max_boxes),
           (n_batch,)
        ]
        """
        n_batch, n_box, n_score = scores.shape

        iou_threshold = iou_threshold.astype(np.float32)
        score_threshold = score_threshold.astype(np.float32)

        # convert box ids to TF style
        center_w, center_h, width, height = np.split(
            boxes, 4, axis=-1
        )  # (n_batch,n_box,1)
        y1 = center_h - 0.5 * height
        y2 = center_h + 0.5 * height
        x1 = center_w - 0.5 * width
        x2 = center_w + 0.5 * width
        boxes_tf = np.concatenate((y1, x1, y2, x2), axis=-1)  # (n_batch,n_box,4)

        out1 = np.zeros((n_batch, max_boxes, 4))
        out2 = np.zeros((n_batch, max_boxes, n_score))
        out3 = -1 * np.ones((n_batch, max_boxes))
        out4 = np.zeros((n_batch,))

        for b in range(n_batch):
            box_coord_matrix = boxes_tf[b, :, :]  # (n_box,4)
            score_vector = np.max(scores[b, :, :], axis=-1)  # (n_box,)
            if not per_class_suppression:
                # this is the simple case as TF directly supports it
                with tf.Graph().as_default(), tf.Session() as sess:
                    box_coord_matrix_pl = tf.placeholder(
                        tf.float32, shape=box_coord_matrix.shape
                    )
                    score_vector_pl = tf.placeholder(
                        tf.float32, shape=score_vector.shape
                    )
                    ids_g = tf.image.non_max_suppression(
                        box_coord_matrix_pl,
                        score_vector_pl,
                        max_output_size=max_boxes,
                        iou_threshold=iou_threshold,
                        score_threshold=score_threshold,
                    )
                    ids = sess.run(
                        ids_g,
                        feed_dict={
                            box_coord_matrix_pl: box_coord_matrix,
                            score_vector_pl: score_vector,
                        },
                    )
            else:
                # this is slightly complicated as TF does not directly support it
                class_ids = np.argmax(scores[b, :, :], axis=-1)  # (n_box,)
                sorted_score_ids = np.argsort(-score_vector)
                box_coord_matrix2 = np.take(box_coord_matrix, sorted_score_ids, axis=0)
                score_vector2 = np.take(score_vector, sorted_score_ids)
                class_ids = np.take(class_ids, sorted_score_ids)
                classes_seen = dict()
                ids_intermediate = np.array([], dtype=np.int)
                for n in range(n_box):
                    if class_ids[n] in classes_seen:
                        continue
                    c = class_ids[n]
                    classes_seen[c] = True
                    current_class_ids = np.where(class_ids == c)[0]
                    if len(current_class_ids) > 0:
                        feed_in1 = np.take(box_coord_matrix2, current_class_ids, axis=0)
                        feed_in2 = np.take(score_vector2, current_class_ids)

                        with tf.Graph().as_default(), tf.Session() as sess:
                            box_coord_matrix_pl = tf.placeholder(
                                tf.float32, shape=feed_in1.shape
                            )
                            score_vector_pl = tf.placeholder(
                                tf.float32, shape=feed_in2.shape
                            )
                            cur_ids_g = tf.image.non_max_suppression(
                                box_coord_matrix_pl,
                                score_vector_pl,
                                max_output_size=max_boxes,
                                iou_threshold=iou_threshold,
                                score_threshold=score_threshold,
                            )
                            cur_ids = sess.run(
                                cur_ids_g,
                                feed_dict={
                                    box_coord_matrix_pl: feed_in1,
                                    score_vector_pl: feed_in2,
                                },
                            )

                        from_sort_ids = np.take(current_class_ids, cur_ids)
                        ids_intermediate = np.append(ids_intermediate, from_sort_ids)
                ids_intermediate.sort()
                ids = np.take(sorted_score_ids, ids_intermediate)

            xx = len(ids)
            if xx == 0:
                ids = np.array([np.argmax(score_vector)])
                xx = 1
            if xx > max_boxes:
                ids = ids[:max_boxes]
                xx = len(ids)
            out1[b, :xx, :] = np.take(boxes[b, :, :], ids, axis=0)
            out2[b, :xx, :] = np.take(scores[b, :, :], ids, axis=0)
            out3[b, :xx] = ids
            out4[b] = xx

        return out1, out2, out3, out4

    @pytest.mark.xfail(reason="rdar://60390856", run=False)
    @pytest.mark.parametrize(
        ",".join(
            [
                "use_cpu_only",
                "backend",
                "iou_threshold_percentile",
                "score_threshold_percentile",
                "n_boxes",
                "n_batch",
                "n_score",
                "per_class_suppression",
            ]
        ),
        itertools.product(
            [True, False],
            backends,
            [0, 30, 80, 100],
            [0, 40, 100],
            [(10, 7), (30, 37), (100, 64)],
            [1],
            [1, 4, 7],
            [True, False],
        ),
    )
    def test_builder_to_backend_stress(
        self,
        use_cpu_only,
        backend,
        iou_threshold_percentile,
        score_threshold_percentile,
        n_boxes,
        n_batch,
        n_score,
        per_class_suppression,
    ):
        n_boxes_in, n_boxes_out = n_boxes
        boxes_val = random_gen((n_batch, n_boxes_in, 4), 0, 100)
        scores_val = random_gen((n_batch, n_boxes_in, n_score), -100, 100)

        iou_matrix = self._compute_iou_matrix(boxes_val[0, :, :])
        iou_matrix = iou_matrix[~np.eye(iou_matrix.shape[0], dtype=bool)].reshape(
            iou_matrix.shape[0], -1
        )

        if score_threshold_percentile == 0:
            score_threshold = np.min(scores_val) - 1
        elif score_threshold_percentile == 100:
            score_threshold = np.max(scores_val) + 1
        else:
            score_threshold = (
                np.percentile(scores_val, score_threshold_percentile) + 0.01
            )

        if iou_threshold_percentile == 0:
            iou_threshold = np.maximum(np.min(iou_matrix) - 0.01, 0.0)
        else:
            iou_threshold = np.percentile(iou_matrix, iou_threshold_percentile) + 0.01

        (
            tf_boxes,
            tf_scores,
            tf_indices,
            tf_num_boxes,
        ) = self._ref_non_maximum_suppression(
            boxes_val,
            scores_val,
            iou_threshold,
            score_threshold,
            n_boxes_out,
            per_class_suppression,
        )
        expected_outputs = [tf_boxes, tf_scores, tf_indices, tf_num_boxes]
        expected_output_types = [
            tf_boxes.shape[:] + (types.fp32,),
            tf_scores.shape[:] + (types.fp32,),
            tf_indices.shape[:] + (types.int32,),
            tf_num_boxes.shape[:] + (types.int32,),
        ]

        input_placeholders = {
            "boxes": mb.placeholder(shape=(n_batch, n_boxes_in, 4)),
            "scores": mb.placeholder(shape=(n_batch, n_boxes_in, n_score)),
        }
        input_values = {"boxes": boxes_val, "scores": scores_val}

        def build(boxes, scores):
            return mb.non_maximum_suppression(
                boxes=boxes,
                scores=scores,
                iou_threshold=iou_threshold,
                score_threshold=score_threshold,
                max_boxes=n_boxes_out,
                per_class_suppression=per_class_suppression,
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


class TestNonZero:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], ["nn_proto"])
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array([[3, 0, 0], [0, 4, 0], [5, 6, 0]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [mb.non_zero(x=x)]

        expected_output_types = [(UNK_SYM, 2, types.int)]
        expected_outputs = [np.array(np.transpose(np.nonzero(x_val)))]

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
        x_val = np.random.randint(low=-1, high=2, size=(6, 1, 7))
        res = mb.non_zero(x=x_val)
        assert is_close(np.transpose(np.nonzero(x_val)), res.val)


class TestOneHot:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x = np.array([1, 0], dtype=np.int32)
        depth = 4

        input_placeholders = {
            "x": mb.placeholder(shape=x.shape, dtype=types.int32),
            "y": mb.placeholder(shape=(), dtype=types.int32),
        }

        input_values = {"x": x, "y": depth}

        def build(x, y):
            return [
                mb.one_hot(indices=x, one_hot_vector_size=4),
                mb.one_hot(indices=x, one_hot_vector_size=4, axis=0),
                mb.one_hot(
                    indices=x, one_hot_vector_size=4, on_value=1.0, off_value=0.0
                ),
                mb.one_hot(
                    indices=x, one_hot_vector_size=y, on_value=1.0, off_value=0.0
                ),
            ]

        expected_output_types = [
            (2, 4, types.int32),
            (4, 2, types.int32),
            (2, 4, types.fp32),
            (2, UNK_SYM, types.fp32),
        ]

        expected_outputs = [
            np.array([[0, 1, 0, 0], [1, 0, 0, 0]], dtype=np.float32),
            np.array([[0, 1], [1, 0], [0, 0], [0, 0]], dtype=np.float32),
            np.array([[0, 1, 0, 0], [1, 0, 0, 0]], dtype=np.float32),
            np.array([[0, 1, 0, 0], [1, 0, 0, 0]], dtype=np.float32),
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


class TestPad:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        def test_constant_mode():
            t = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
            pad = np.array([1, 1, 2, 2], dtype=np.int32)
            input_placeholders = {"x": mb.placeholder(shape=t.shape)}
            input_values = {"x": t}

            def build(x):
                return mb.pad(x=x, pad=pad, mode="constant", constant_val=0.0)

            expected_output_types = (4, 7, types.fp32)
            expected_outputs = np.array(
                [
                    [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
                    [0.0, 0.0, 1.0, 2.0, 3.0, 0.0, 0.0],
                    [0.0, 0.0, 4.0, 5.0, 6.0, 0.0, 0.0],
                    [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
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

        def test_constant_mode_constant_val():
            t = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
            pad = np.array([1, 1, 2, 2], dtype=np.int32)
            input_placeholders = {"x": mb.placeholder(shape=t.shape)}
            input_values = {"x": t}

            def build(x):
                return mb.pad(x=x, pad=pad, mode="constant", constant_val=0.5)

            expected_output_types = (4, 7, types.fp32)
            expected_outputs = np.array(
                [
                    [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5],
                    [0.5, 0.5, 1.0, 2.0, 3.0, 0.5, 0.5],
                    [0.5, 0.5, 4.0, 5.0, 6.0, 0.5, 0.5],
                    [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5],
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

        def test_reflect_mode():
            t = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
            pad = np.array([1, 1, 2, 2], dtype=np.int32)
            input_placeholders = {"x": mb.placeholder(shape=t.shape)}
            input_values = {"x": t}

            def build(x):
                return mb.pad(x=x, pad=pad, mode="reflect")

            expected_output_types = (4, 7, types.fp32)
            expected_outputs = np.array(
                [
                    [6.0, 5.0, 4.0, 5.0, 6.0, 5.0, 4.0],
                    [3.0, 2.0, 1.0, 2.0, 3.0, 2.0, 1.0],
                    [6.0, 5.0, 4.0, 5.0, 6.0, 5.0, 4.0],
                    [3.0, 2.0, 1.0, 2.0, 3.0, 2.0, 1.0],
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

        def test_replicate_mode():
            t = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
            pad = np.array([1, 1, 2, 2], dtype=np.int32)
            input_placeholders = {"x": mb.placeholder(shape=t.shape)}
            input_values = {"x": t}

            def build(x):
                return mb.pad(x=x, pad=pad, mode="replicate")

            expected_output_types = (4, 7, types.fp32)
            expected_outputs = np.array(
                [
                    [1.0, 1.0, 1.0, 2.0, 3.0, 3.0, 3.0],
                    [1.0, 1.0, 1.0, 2.0, 3.0, 3.0, 3.0],
                    [4.0, 4.0, 4.0, 5.0, 6.0, 6.0, 6.0],
                    [4.0, 4.0, 4.0, 5.0, 6.0, 6.0, 6.0],
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

        def test_constant_general():
            t = np.arange(12, dtype=np.float32).reshape([2, 2, 3])
            pad = np.array([[1, 1], [2, 2], [1, 1]], dtype=np.int32)
            input_placeholders = {"x": mb.placeholder(shape=t.shape)}
            input_values = {"x": t}

            def build(x):
                return mb.pad(
                    x=x, pad=pad.reshape(-1), mode="constant", constant_val=0.0
                )

            expected_output_types = (4, 6, 5, types.fp32)
            expected_outputs = np.pad(t, pad, mode="constant")

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

        # Test different modes
        test_constant_mode()
        test_constant_mode_constant_val()
        test_reflect_mode()
        test_replicate_mode()
        test_constant_general()

    @ssa_fn
    def test_builder_eval(self):
        def test_constant_mode():
            x_val = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
            v = mb.pad(
                x=x_val,
                pad=np.array([1, 1, 2, 2], dtype=np.int32),
                mode="constant",
                constant_val=0.0,
            )
            expected_outputs = np.array(
                [
                    [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
                    [0.0, 0.0, 1.0, 2.0, 3.0, 0.0, 0.0],
                    [0.0, 0.0, 4.0, 5.0, 6.0, 0.0, 0.0],
                    [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
                ],
                dtype=np.float32,
            )
            assert is_close(expected_outputs, v.val)

        def test_reflect_mode():
            x_val = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
            v = mb.pad(
                x=x_val, pad=np.array([1, 1, 2, 2], dtype=np.int32), mode="reflect"
            )
            expected_outputs = np.array(
                [
                    [6.0, 5.0, 4.0, 5.0, 6.0, 5.0, 4.0],
                    [3.0, 2.0, 1.0, 2.0, 3.0, 2.0, 1.0],
                    [6.0, 5.0, 4.0, 5.0, 6.0, 5.0, 4.0],
                    [3.0, 2.0, 1.0, 2.0, 3.0, 2.0, 1.0],
                ],
                dtype=np.float32,
            )
            assert is_close(expected_outputs, v.val)

        def test_replicate_mode():
            x_val = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
            v = mb.pad(
                x=x_val, pad=np.array([1, 1, 2, 2], dtype=np.int32), mode="replicate"
            )
            expected_outputs = np.array(
                [
                    [1.0, 1.0, 1.0, 2.0, 3.0, 3.0, 3.0],
                    [1.0, 1.0, 1.0, 2.0, 3.0, 3.0, 3.0],
                    [4.0, 4.0, 4.0, 5.0, 6.0, 6.0, 6.0],
                    [4.0, 4.0, 4.0, 5.0, 6.0, 6.0, 6.0],
                ],
                dtype=np.float32,
            )
            assert is_close(expected_outputs, v.val)

        def test_constant_general():
            x_val = np.arange(12, dtype=np.float32).reshape([2, 2, 3])
            pad = np.array([[1, 1], [2, 2], [1, 1]], dtype=np.int32)
            v = mb.pad(x=x_val, pad=pad.reshape(-1), mode="constant", constant_val=0.0)
            expected_outputs = np.pad(x_val, pad, mode="constant")
            assert is_close(expected_outputs, v.val)

        # Test different modes
        test_constant_mode()
        test_reflect_mode()
        test_replicate_mode()
        test_constant_general()


class TestRange1d:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x = np.array([15.0], dtype=np.float32)
        y = 5.0
        z = 2.0
        input_placeholders = {
            "x": mb.placeholder(shape=x.shape),
            "y": mb.placeholder(shape=()),
            "z": mb.placeholder(shape=()),
        }
        input_values = {"x": x, "y": y, "z": z}

        def build(x, y, z):
            return [
                mb.mul(x=x, y=x),
                mb.range_1d(start=y, end=15.0, step=2.0),
                mb.range_1d(start=y, end=15.0, step=z),
                mb.range_1d(start=y, end=x, step=2.0),
                mb.range_1d(start=y, end=x, step=z),
                mb.range_1d(start=5.0, end=15.0, step=z),
                mb.range_1d(start=5.0, end=x, step=2.0),
                mb.range_1d(start=5.0, end=x, step=z),
            ]

        expected_output_types = [
            (1, types.fp32),
            (UNK_SYM, types.fp32),
            (UNK_SYM, types.fp32),
            (UNK_SYM, types.fp32),
            (UNK_SYM, types.fp32),
            (UNK_SYM, types.fp32),
            (UNK_SYM, types.fp32),
            (UNK_SYM, types.fp32),
        ]

        expected_outputs = [
            np.array([225.0], dtype=np.float32),
            np.array([5, 7, 9, 11, 13], dtype=np.float32),
            np.array([5, 7, 9, 11, 13], dtype=np.float32),
            np.array([5, 7, 9, 11, 13], dtype=np.float32),
            np.array([5, 7, 9, 11, 13], dtype=np.float32),
            np.array([5, 7, 9, 11, 13], dtype=np.float32),
            np.array([5, 7, 9, 11, 13], dtype=np.float32),
            np.array([5, 7, 9, 11, 13], dtype=np.float32),
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
        v = mb.range_1d(start=5, end=15, step=2)
        assert is_close(np.arange(5, 15, 2), v.val)


class TestTile:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=x.shape)}

        input_values = {"x": x}

        def build(x):
            return [
                mb.tile(x=x, reps=(1, 1)),
                mb.tile(x=x, reps=(2,)),
                mb.tile(x=x, reps=(2, 1)),
            ]

        expected_output_types = [
            (2, 3, types.fp32),
            (2, 6, types.fp32),
            (4, 3, types.fp32),
        ]

        expected_outputs = [
            x,
            np.array([[1, 2, 3, 1, 2, 3], [4, 5, 6, 4, 5, 6]], dtype=np.float32),
            np.array([[1, 2, 3], [4, 5, 6], [1, 2, 3], [4, 5, 6]], dtype=np.float32),
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
        v = mb.tile(x=x, reps=(2,))
        assert is_close(np.tile(x, reps=(2,)), v.val)


@pytest.mark.skip(reason="rdar://65198011 (Re-enable Conv3dTranspose and DynamicTile unit tests)")
class TestDynamicTile:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        rep1 = np.array([1, 1]).astype(np.int32)
        rep2 = np.array([2, 1]).astype(np.int32)
        rep3 = np.array([2, 3]).astype(np.int32)
        input_placeholders = {
            "x": mb.placeholder(shape=x.shape),
            "reps1": mb.placeholder(shape=rep1.shape),
            "reps2": mb.placeholder(shape=rep2.shape),
            "reps3": mb.placeholder(shape=rep3.shape),
        }

        input_values = {"x": x, "reps1": rep1, "reps2": rep2, "reps3": rep3}

        def build(x, reps1, reps2, reps3):
            return [
                mb.tile(x=x, reps=reps1),
                mb.tile(x=x, reps=reps2),
                mb.tile(x=x, reps=reps3),
            ]

        expected_output_types = [
            (UNK_SYM, UNK_SYM, types.fp32),
            (UNK_SYM, UNK_SYM, types.fp32),
            (UNK_SYM, UNK_SYM, types.fp32),
        ]

        expected_outputs = [
            x,
            np.array([[1, 2, 3], [4, 5, 6], [1, 2, 3], [4, 5, 6]], dtype=np.float32),
            np.array(
                [
                    [1, 2, 3, 1, 2, 3, 1, 2, 3],
                    [4, 5, 6, 4, 5, 6, 4, 5, 6],
                    [1, 2, 3, 1, 2, 3, 1, 2, 3],
                    [4, 5, 6, 4, 5, 6, 4, 5, 6],
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
            frontend_only=False,
            backend=backend,
        )


class TestTopK:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        val = np.array([[-1.0, 2.0, -3.0], [4.0, -5.0, 6.0]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=val.shape)}
        input_values = {"x": val}

        def build(x):
            return mb.topk(x=x, k=2, axis=1)

        expected_output_types = [
            (2, 2, types.fp32),
            (2, 2, types.int32),
        ]
        expected_outputs = [
            np.array([[2.0, -1.0], [6.0, 4.0]], dtype=np.float32),
            np.array([[1, 0], [2, 0]], dtype=np.float32),
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
        def np_topk(x, k, axis, ascending=False):
            indices = np.argsort(x, axis=axis)
            if not ascending:
                indices = np.argsort(-x, axis=axis)
            slc = [slice(None)] * len(x.shape)
            slc[axis] = slice(0, k)
            indices = indices[tuple(slc)]
            values = np.take_along_axis(x, indices, axis=axis)
            return values, indices

        val = np.array([[-1.0, 7.0, -3.0], [4.0, -5.0, 8.0]], dtype=np.float32)
        res_values, res_indices = mb.topk(x=val, k=1, axis=0)
        ref_values, ref_indices = np_topk(x=val, k=1, axis=0)
        assert is_close(ref_values, res_values.val)
        assert is_close(ref_indices, res_indices.val)
        res_values, res_indices = mb.topk(x=val, k=2, axis=-1, ascending=True)
        ref_values, ref_indices = np_topk(x=val, k=2, axis=-1, ascending=True)
        assert is_close(ref_values, res_values.val)
        assert is_close(ref_indices, res_indices.val)

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_symbolic(self, use_cpu_only, backend):
        s0 = get_new_symbol()

        val = np.array([[1.0, 2.0, -3.0], [4.0, -5.0, 6.0]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=(s0, 3))}
        input_values = {"x": val}

        def build(x):
            return mb.topk(x=x, k=2, axis=-1, ascending=True)

        expected_output_types = [
            (s0, 2, types.fp32),
            (s0, 2, types.int32),
        ]
        expected_outputs = [
            np.array([[-3.0, 1.0], [-5.0, 4.0]], dtype=np.float32),
            np.array([[2, 0], [1, 0]], dtype=np.float32),
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


class TestFlatten:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array(
            [[[1, 2, 3], [4, 5, 6]], [[-1, -2, -3], [-4, -5, -6]]], dtype=np.float32
        )
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            return [mb.flatten(x=x)]

        expected_output_types = [
            (2, 6, types.fp32),
        ]
        expected_outputs = [
            np.array([[1, 2, 3, 4, 5, 6], [-1, -2, -3, -4, -5, -6]], dtype=np.float32),
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
        t = np.array([[[1, 2, 3], [4, 5, 6]]], dtype=np.float32)
        f = mb.flatten(x=t)
        expected_f = np.array([[1, 2, 3, 4, 5, 6]], dtype=np.float32)
        assert is_close(expected_f, f.val)

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_symbolic(self, use_cpu_only, backend):
        s0 = get_new_symbol()

        # Test variadic (rdar://59559656)
        input_placeholders = {
            "x": mb.placeholder(shape=(s0, 4, 5, 6)),
        }

        def build(x):
            return [mb.flatten(x=x)]

        input = np.random.rand(10, 4, 5, 6)
        output = input.reshape(10, -1)

        expected_output_types = (s0, 120, types.fp32)
        expected_outputs = [output]

        input_values = {"x": input}
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


class TestShape:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float32)
        pad = np.array([1, 1, 2, 2], dtype=np.int32)
        input_placeholders = {"x": mb.placeholder(shape=t.shape)}
        input_values = {"x": t}

        def build(x):
            x = mb.pad(x=x, pad=pad, mode="constant", constant_val=0.0)
            return mb.shape(x=x)

        expected_output_types = (2, types.int32)
        expected_outputs = [
            np.array([4, 7], dtype=np.int32),
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
        t = np.array([[[1, 2, 3], [4, 5, 6]]], dtype=np.float32)
        f = mb.shape(x=t)
        expected_f = np.array([1, 2, 3], dtype=np.float32)
        assert is_close(expected_f, f.val)

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_symbolic(self, use_cpu_only, backend):
        s0 = get_new_symbol()

        # Test variadic (rdar://59559656)
        input_placeholders = {
            "x": mb.placeholder(shape=(s0, 4, 5, 6)),
        }

        def build(x):
            return [mb.shape(x=x)]

        input = np.random.rand(10, 4, 5, 6)
        output = np.array([10, 4, 5, 6], dtype=np.float32)

        expected_output_types = (4, types.int32)
        expected_outputs = [output]

        input_values = {"x": input}
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


class TestConcat:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t1 = np.array([[1, 2], [4, 5]], dtype=np.float32)
        t2 = np.array([[7, 8]], dtype=np.float32)

        input_placeholders = {
            "x": mb.placeholder(shape=t1.shape),
            "y": mb.placeholder(shape=t2.shape),
        }
        input_values = {"x": t1, "y": t2}

        def build(x, y):
            return (mb.concat(values=(x, y), axis=0),)

        expected_output_types = [
            (3, 2, types.fp32),
        ]
        expected_outputs = [
            np.array([[1, 2], [4, 5], [7, 8]], dtype=np.float32),
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
    def test_builder_to_backend_type_promotion(self, use_cpu_only, backend):
        t1 = np.array([[1, 2], [4, 5]], dtype=np.float32)
        t2 = np.array([[7, 8]], dtype=np.float32)

        input_placeholders = {
            "x": mb.placeholder(shape=t1.shape),
        }
        input_values = {"x": t1}

        def build(x):
            t2 = np.array([[7, 8]], dtype=np.int32)
            return (mb.concat(values=(x, t2), axis=0),)

        expected_output_types = [
            # np.int32 should be promoted to fp32
            (3, 2, types.fp32),
        ]
        expected_outputs = [
            np.array([[1, 2], [4, 5], [7, 8]], dtype=np.float32),
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
        values = [
            np.random.rand(1, 1, 6, 2),
            np.random.rand(1, 1, 3, 2),
        ]
        v = mb.concat(values=values, axis=2)
        assert is_close(np.concatenate(values, 2), v.val)

    @ssa_fn
    def test_builder_eval_failure(self):
        values = [
            np.random.rand(1, 1, 6, 2),
            np.random.rand(1, 1, 3, 1),
        ]
        with pytest.raises(ValueError):
            v = mb.concat(values=values, axis=2)


class TestSplit:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t = np.array([[1, 2], [3, 4], [5, 6]], dtype=np.float32)

        input_placeholders = {
            "x": mb.placeholder(shape=t.shape),
        }
        input_values = {"x": t}

        def build(x):
            return mb.split(x=x, num_splits=2, axis=1) + mb.split(
                x=x, split_sizes=[1, 2], axis=0
            )

        expected_output_types = [
            (3, 1, types.fp32),
            (3, 1, types.fp32),
            (1, 2, types.fp32),
            (2, 2, types.fp32),
        ]
        expected_outputs = [
            np.array([[1], [3], [5]], dtype=np.float32),
            np.array([[2], [4], [6]], dtype=np.float32),
            np.array([[1, 2]], dtype=np.float32),
            np.array([[3, 4], [5, 6]], dtype=np.float32),
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
        t = np.array([[1, 2], [3, 4], [5, 6]], dtype=np.float32)
        vs = mb.split(x=t, num_splits=3, axis=0)
        es = np.split(t, [1, 2, 3], axis=0)
        for v, e in zip(vs, es):
            assert is_close(e, v.val)


class TestStack:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        t1 = np.array([1, 2, 3], dtype=np.float32)
        t2 = np.array([7, 8, 9], dtype=np.float32)

        input_placeholders = {
            "x": mb.placeholder(shape=t1.shape),
            "y": mb.placeholder(shape=t2.shape),
        }
        input_values = {"x": t1, "y": t2}

        def build(x, y):
            return [mb.stack(values=(x, y), axis=0), mb.stack(values=(x, y), axis=1)]

        expected_output_types = [
            (2, 3, types.fp32),
            (3, 2, types.fp32),
        ]
        expected_outputs = [
            np.array([[1, 2, 3], [7, 8, 9]], dtype=np.float32),
            np.array([[1, 7], [2, 8], [3, 9]], dtype=np.float32),
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
        values = [
            np.random.rand(1, 1, 3, 2).astype(np.float32),
            np.random.rand(1, 1, 3, 2).astype(np.float32),
        ]
        v = mb.stack(values=values, axis=2)
        assert is_close(np.stack(values, 2), v.val)


class TestArgSort:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        val = np.array([[-1.0, 2.0, -3.0], [4.0, -5.0, 6.0]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=val.shape)}
        input_values = {"x": val}

        def build(x):
            return [mb.argsort(x=x), mb.argsort(x=x, axis=0, ascending=True)]

        expected_output_types = [
            (2, 3, types.int32),
            (2, 3, types.int32),
        ]
        expected_outputs = [
            np.array([[1, 0, 2], [2, 0, 1]], dtype=np.int32),
            np.array([[0, 1, 0], [1, 0, 1]], dtype=np.int32),
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
        x_val = random_gen(shape=(1, 3, 2, 2), rand_min=-100, rand_max=100)
        res = mb.argsort(x=x_val, axis=-3)
        assert is_close(np.argsort(x_val, axis=-3), res.val)


class TestIsFinite:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        val = np.array([[np.inf, -np.inf, 0], [-np.inf, 5, 6]], dtype=np.float32)
        input_placeholders = {"x": mb.placeholder(shape=val.shape)}
        input_values = {"x": val}

        def build(x):
            return [mb.isfinite(x=x)]

        expected_output_types = [(2, 3, types.bool)]
        expected_outputs = [
            np.array([[False, False, True], [False, True, True]], dtype=np.bool)
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
        shape = (3, 3, 3, 3)
        x_val = random_gen(shape=shape, rand_min=-1, rand_max=1)
        random_map = np.random.choice([np.inf, -np.inf, 0], size=shape)
        x_val[np.where(random_map == np.inf)] = np.inf
        x_val[np.where(random_map == -np.inf)] = -np.inf
        res = mb.isfinite(x=x_val)
        assert is_close(np.isfinite(x_val), res.val)
