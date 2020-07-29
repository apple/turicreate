#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.mil import get_new_symbol
from coremltools.converters.mil.testing_reqs import *

from .testing_utils import run_compare_builder

backends = testing_reqs.backends


@pytest.mark.skip("Broken for mil backend")
class TestResizeBilinear:
    def test_builder_to_backend_smoke(self, use_cpu_only=True, backend="nn_proto"):
        x = np.array([0, 1], dtype=np.float32).reshape(1, 1, 2)
        input_placeholder_dict = {"x": mb.placeholder(shape=x.shape)}
        input_value_dict = {"x": x}

        def build_mode_0(x):
            return mb.resize_bilinear(
                x=x,
                target_size_height=1,
                target_size_width=5,
                sampling_mode="STRICT_ALIGN_CORNERS",
            )

        expected_output_type = (1, 1, 5, types.fp32)
        expected_output = np.array([0, 0.25, 0.5, 0.75, 1], dtype=np.float32).reshape(
            1, 1, 5
        )

        run_compare_builder(
            build_mode_0,
            input_placeholder_dict,
            input_value_dict,
            expected_output_type,
            expected_output,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

        def build_mode_2(x):
            return mb.resize_bilinear(
                x=x, target_size_height=1, target_size_width=5, sampling_mode="DEFAULT"
            )

        expected_output = np.array([0, 0.4, 0.8, 1, 1], dtype=np.float32).reshape(
            1, 1, 5
        )

        run_compare_builder(
            build_mode_2,
            input_placeholder_dict,
            input_value_dict,
            expected_output_type,
            expected_output,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

        def build_mode_3(x):
            return mb.resize_bilinear(
                x=x,
                target_size_height=1,
                target_size_width=5,
                sampling_mode="OFFSET_CORNERS",
            )

        expected_output = np.array([0.1, 0.3, 0.5, 0.7, 0.9], dtype=np.float32).reshape(
            1, 1, 5
        )

        run_compare_builder(
            build_mode_3,
            input_placeholder_dict,
            input_value_dict,
            expected_output_type,
            expected_output,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


@pytest.mark.skip("Broken for nn backend")
class TestUpsampleBilinear:
    def test_builder_to_backend_smoke(self, use_cpu_only=True, backend="nn_proto"):
        x = np.array([0, 1], dtype=np.float32).reshape(1, 1, 2)
        input_placeholder_dict = {"x": mb.placeholder(shape=x.shape)}
        input_value_dict = {"x": x}

        def build_upsample_integer(x):
            return mb.upsample_bilinear(
                x=x, scale_factor_height=1, scale_factor_width=3
            )

        expected_output_type = (1, 1, 6, types.fp32)
        expected_output = np.array(
            [0, 0.2, 0.4, 0.6, 0.8, 1], dtype=np.float32
        ).reshape(1, 1, 6)

        run_compare_builder(
            build_upsample_integer,
            input_placeholder_dict,
            input_value_dict,
            expected_output_type,
            expected_output,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

        def build_upsample_fractional(x):
            return mb.upsample_bilinear(
                x=x, scale_factor_height=1, scale_factor_width=2.6, align_corners=False
            )

        expected_output_type = (1, 1, 5, types.fp32)
        expected_output = np.array([0, 0.1, 0.5, 0.9, 1], dtype=np.float32).reshape(
            1, 1, 5
        )

        run_compare_builder(
            build_upsample_fractional,
            input_placeholder_dict,
            input_value_dict,
            expected_output_type,
            expected_output,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

    # TODO: enable GPU test: rdar://problem/60309338
    @pytest.mark.skipif(not testing_reqs._HAS_TORCH, reason="PyTorch not installed.")
    @pytest.mark.parametrize(
        "use_cpu_only, backend, input_shape, scale_factor, align_corners",
        itertools.product(
            [True],
            backends,
            [(2, 5, 10, 22)],
            [(3, 4), (2.5, 2), (0.5, 0.75)],
            [True, False],
        ),
    )
    def test_builder_to_backend_stress(
        self, use_cpu_only, backend, input_shape, scale_factor, align_corners
    ):
        def _get_torch_upsample_prediction(x, scale_factor=(2, 2), align_corners=False):
            x = torch.from_numpy(x)
            m = torch.nn.Upsample(
                scale_factor=scale_factor, mode="bilinear", align_corners=align_corners
            )
            out = m(x)
            return out.numpy()

        x = random_gen(input_shape, rand_min=-100, rand_max=100)
        torch_pred = _get_torch_upsample_prediction(
            x, scale_factor=scale_factor, align_corners=align_corners
        )

        input_placeholder_dict = {"x": mb.placeholder(shape=x.shape)}
        input_value_dict = {"x": x}

        def build_upsample(x):
            return mb.upsample_bilinear(
                x=x,
                scale_factor_height=scale_factor[0],
                scale_factor_width=scale_factor[1],
                align_corners=align_corners,
            )

        expected_output_type = torch_pred.shape + (types.fp32,)
        run_compare_builder(
            build_upsample,
            input_placeholder_dict,
            input_value_dict,
            expected_output_type,
            torch_pred,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestUpsampleNearestNeighbor:
    def test_builder_to_backend_smoke(self, use_cpu_only=True, backend="nn_proto"):
        x = np.array([1.5, 2.5, 3.5], dtype=np.float32).reshape(1, 1, 1, 3)
        input_placeholder_dict = {"x": mb.placeholder(shape=x.shape)}
        input_value_dict = {"x": x}

        def build(x):
            return mb.upsample_nearest_neighbor(
                x=x, upscale_factor_height=1, upscale_factor_width=2
            )

        expected_output_type = (1, 1, 1, 6, types.fp32)
        expected_output = np.array(
            [1.5, 1.5, 2.5, 2.5, 3.5, 3.5], dtype=np.float32
        ).reshape(1, 1, 1, 6)

        run_compare_builder(
            build,
            input_placeholder_dict,
            input_value_dict,
            expected_output_type,
            expected_output,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestCrop:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, is_symbolic",
        itertools.product([True, False], backends, [True, False]),
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend, is_symbolic):
        x = np.array(
            [[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12], [13, 14, 15, 16]],
            dtype=np.float32,
        ).reshape(1, 1, 4, 4)

        input_shape = list(x.shape)
        placeholder_input_shape = input_shape
        if is_symbolic:
            # set batch and channel dimension symbolic
            placeholder_input_shape[0] = get_new_symbol()
            placeholder_input_shape[1] = get_new_symbol()

        input_placeholder_dict = {"x": mb.placeholder(shape=placeholder_input_shape)}
        input_value_dict = {"x": x}

        def build(x):
            return mb.crop(x=x, crop_height=[0, 1], crop_width=[1, 1])

        expected_output_type = (
            placeholder_input_shape[0],
            placeholder_input_shape[1],
            3,
            2,
            types.fp32,
        )
        expected_output = (
            np.array([2, 3, 6, 7, 10, 11], dtype=np.float32).reshape(1, 1, 3, 2),
        )

        run_compare_builder(
            build,
            input_placeholder_dict,
            input_value_dict,
            expected_output_type,
            expected_output,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )

    @pytest.mark.parametrize(
        "use_cpu_only, backend, C, H, W",
        itertools.product(
            [True, False],
            backends,
            [x for x in range(1, 4)],
            [x for x in range(5, 10)],
            [x for x in range(5, 10)],
        ),
    )
    def test_builder_to_backend_stress(self, use_cpu_only, backend, C, H, W):
        input_shape = (1, C, H, W)
        x = np.random.random(input_shape)

        crop_h = [np.random.randint(H)]
        crop_h.append(np.random.randint(H - crop_h[0]))
        crop_w = [np.random.randint(W)]
        crop_w.append(np.random.randint(W - crop_w[0]))

        input_placeholder_dict = {"x": mb.placeholder(shape=input_shape)}
        input_value_dict = {"x": x}

        def build(x):
            return mb.crop(x=x, crop_height=crop_h, crop_width=crop_w)

        expected_output_type = (
            1,
            C,
            H - crop_h[0] - crop_h[1],
            W - crop_w[0] - crop_w[1],
            types.fp32,
        )
        expected_output = x[:, :, crop_h[0] : H - crop_h[1], crop_w[0] : W - crop_w[1]]

        run_compare_builder(
            build,
            input_placeholder_dict,
            input_value_dict,
            expected_output_type,
            expected_output,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestCropResize:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, is_symbolic",
        itertools.product([True, False], backends, [True, False]),
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend, is_symbolic):
        x = np.array(
            [[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12], [13, 14, 15, 16]],
            dtype=np.float32,
        ).reshape(1, 1, 4, 4)

        input_shape = list(x.shape)
        placeholder_input_shape = input_shape
        if is_symbolic:
            # set batch and channel dimension symbolic
            placeholder_input_shape[0] = get_new_symbol()
            placeholder_input_shape[1] = get_new_symbol()

        input_placeholder_dict = {"x": mb.placeholder(shape=placeholder_input_shape)}
        input_value_dict = {"x": x}
        N = 1
        roi = np.array([[1, 1, 2, 2]], dtype=np.float32).reshape(1, 1, 4, 1, 1)
        roi_normalized = np.array(
            [[0, 0.0, 0.0, 1.0 / 3, 1.0 / 3]], dtype=np.float32
        ).reshape(1, 1, 5, 1, 1)
        roi_invert = np.array([[2, 2, 1, 1]], dtype=np.float32).reshape(1, 1, 4, 1, 1)

        def build(x):
            return [
                mb.crop_resize(
                    x=x,
                    roi=roi,
                    target_width=2,
                    target_height=2,
                    normalized_coordinates=False,
                    box_coordinate_mode="CORNERS_HEIGHT_FIRST",
                    sampling_mode="ALIGN_CORNERS",
                ),
                mb.crop_resize(
                    x=x,
                    roi=roi,
                    target_width=4,
                    target_height=4,
                    normalized_coordinates=False,
                    box_coordinate_mode="CORNERS_HEIGHT_FIRST",
                    sampling_mode="ALIGN_CORNERS",
                ),
                mb.crop_resize(
                    x=x,
                    roi=roi,
                    target_width=1,
                    target_height=1,
                    normalized_coordinates=False,
                    box_coordinate_mode="CORNERS_HEIGHT_FIRST",
                    sampling_mode="ALIGN_CORNERS",
                ),
                mb.crop_resize(
                    x=x,
                    roi=roi_normalized,
                    target_width=2,
                    target_height=2,
                    normalized_coordinates=True,
                    box_coordinate_mode="CORNERS_HEIGHT_FIRST",
                    sampling_mode="ALIGN_CORNERS",
                ),
                mb.crop_resize(
                    x=x,
                    roi=roi_invert,
                    target_width=2,
                    target_height=2,
                    normalized_coordinates=False,
                    box_coordinate_mode="CORNERS_HEIGHT_FIRST",
                    sampling_mode="ALIGN_CORNERS",
                ),
            ]

        expected_output_type = [
            (
                N,
                placeholder_input_shape[0],
                placeholder_input_shape[1],
                2,
                2,
                types.fp32,
            ),
            (
                N,
                placeholder_input_shape[0],
                placeholder_input_shape[1],
                4,
                4,
                types.fp32,
            ),
            (
                N,
                placeholder_input_shape[0],
                placeholder_input_shape[1],
                1,
                1,
                types.fp32,
            ),
            (
                N,
                placeholder_input_shape[0],
                placeholder_input_shape[1],
                2,
                2,
                types.fp32,
            ),
            (
                N,
                placeholder_input_shape[0],
                placeholder_input_shape[1],
                2,
                2,
                types.fp32,
            ),
        ]
        expected_output = [
            np.array([6, 7, 10, 11], dtype=np.float32).reshape(1, 1, 1, 2, 2),
            np.array(
                [
                    [6, 6.333333, 6.66666, 7],
                    [7.333333, 7.666666, 8, 8.333333],
                    [8.666666, 9, 9.3333333, 9.666666],
                    [10, 10.333333, 10.666666, 11],
                ],
                dtype=np.float32,
            ).reshape(1, 1, 1, 4, 4),
            np.array([8.5], dtype=np.float32).reshape(1, 1, 1, 1, 1),
            np.array([1, 2, 5, 6], dtype=np.float32).reshape(1, 1, 1, 2, 2),
            np.array([11, 10, 7, 6], dtype=np.float32).reshape(1, 1, 1, 2, 2),
        ]

        run_compare_builder(
            build,
            input_placeholder_dict,
            input_value_dict,
            expected_output_type,
            expected_output,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )
