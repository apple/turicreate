#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from ._op_reqs import *


@register_op(
    doc_str="TODO (rdar://58622145), https://quip-apple.com/1mNfAW4JhWR9#PDe9CARK6vT"
)
class upsample_nearest_neighbor(Operation):
    input_spec = InputSpec(
        x=TensorInputType(),
        upscale_factor_height=IntInputType(const=True, default=1),
        upscale_factor_width=IntInputType(const=True, default=1),
    )

    def __init__(self, **kwargs):
        super(upsample_nearest_neighbor, self).__init__(**kwargs)

    def type_inference(self):
        if self.x.rank < 3:
            raise ValueError(
                'input to the "upsample_nearest_neighbor" op must have rank at least 3'
            )

        ret_shape = list(self.x.shape)
        ret_shape[-1] *= self.upscale_factor_width.val
        ret_shape[-2] *= self.upscale_factor_height.val
        return types.tensor(self.x.dtype, ret_shape)


@register_op(
    doc_str="TODO (rdar://58622145), https://quip-apple.com/1mNfAW4JhWR9#PDe9CA9aGcP"
)
class upsample_bilinear(Operation):
    input_spec = InputSpec(
        x=TensorInputType(),
        scale_factor_height=IntOrFloatInputType(const=True, default=1),
        scale_factor_width=IntOrFloatInputType(const=True, default=1),
        align_corners=BoolInputType(const=True, default=True),
    )

    def __init__(self, **kwargs):
        super(upsample_bilinear, self).__init__(**kwargs)

    def type_inference(self):
        if self.x.rank < 3:
            raise ValueError(
                'input to the "upsample_bilinear" op must have rank at least 3'
            )

        ret_shape = list(self.x.shape)
        ret_shape[-1] = np.floor(self.scale_factor_width.val * ret_shape[-1])
        ret_shape[-2] = np.floor(self.scale_factor_height.val * ret_shape[-2])
        return types.tensor(self.x.dtype, ret_shape)


@register_op(
    doc_str="TODO (rdar://58622145), https://quip-apple.com/1mNfAW4JhWR9#PDe9CAaiOQP"
)
class resize_bilinear(Operation):
    input_spec = InputSpec(
        x=TensorInputType(),
        target_size_height=IntInputType(const=True, default=1),
        target_size_width=IntInputType(const=True, default=1),
        sampling_mode=StringInputType(const=True, default="DEFAULT"),
    )

    def __init__(self, **kwargs):
        super(resize_bilinear, self).__init__(**kwargs)

    def type_inference(self):
        if self.x.rank < 3:
            raise ValueError(
                'input to the "resize_bilinear" op must have rank at least 3'
            )

        if self.sampling_mode.val not in {
            "STRICT_ALIGN_CORNERS",
            "ALIGN_CORNERS",
            "DEFAULT",
            "OFFSET_CORNERS",
        }:
            raise ValueError(
                '"resize_bilinear" op: unrecognized sampling mode "{}"'.format(
                    self.sampling_mode.val
                )
            )

        ret_shape = list(self.x.shape)
        ret_shape[-1] = self.target_size_width.val
        ret_shape[-2] = self.target_size_height.val
        return types.tensor(self.x.dtype, ret_shape)


@register_op(doc_str="https://quip-apple.com/1mNfAW4JhWR9#PDe9CAHTGW7")
class crop_resize(Operation):
    input_spec = InputSpec(
        x=TensorInputType(),
        roi=TensorInputType(),
        target_height=IntInputType(const=True, default=1),
        target_width=IntInputType(const=True, default=1),
        normalized_coordinates=BoolInputType(const=True, default=False),
        spatial_scale=FloatInputType(const=True, default=1.0),
        box_coordinate_mode=StringInputType(
            const=True, default="CONRNERS_HEIGHT_FIRST"
        ),
        sampling_mode=StringInputType(const=True, default="STRICT_ALIGN_CORNERS"),
    )

    def __init__(self, **kwargs):
        super(crop_resize, self).__init__(**kwargs)

    def type_inference(self):
        if self.x.rank != 4:
            raise ValueError(
                'input to the "crop_resize" op must be of rank 4. Provided {}'.format(
                    self.x.rank
                )
            )

        if self.roi.rank != 5:
            raise ValueError(
                'ROI input to the "crop_resize" op must be of rank 5, provided {}'.format(
                    self.roi.rank
                )
            )

        if self.sampling_mode.val not in {
            "STRICT_ALIGN_CORNERS",
            "ALIGN_CORNERS",
            "DEFAULT",
            "OFFSET_CORNERS",
        }:
            raise ValueError(
                '"crop_resize" op: unrecognized sampling mode "{}"'.format(
                    self.sampling_mode
                )
            )

        # ret_shape: [N] + [B, C, h_out, w_out]
        N, B, C = self.roi.shape[0], self.x.shape[0], self.x.shape[1]
        ret_shape = [N, B, C, self.target_height.val, self.target_width.val]
        return types.tensor(self.x.dtype, ret_shape)


@register_op(doc_str="TODO")
class crop(Operation):
    input_spec = InputSpec(
        x=TensorInputType(),
        crop_height=IntTensorInputType(const=True),
        crop_width=IntTensorInputType(const=True),
    )

    def __init__(self, **kwargs):
        super(crop, self).__init__(**kwargs)

    def type_inference(self):
        if self.x.rank < 2:
            raise ValueError(
                'input to the "crop" op must at least be of rank 2. Provided {}'.format(
                    self.x.rank
                )
            )

        crop_height = self.crop_height.val
        crop_width = self.crop_width.val

        if len(crop_height.flatten()) != 2:
            raise ValueError(
                "crop_height must have 2 elements. Provided {}".format(
                    len(crop_height.flatten())
                )
            )

        if len(crop_width.flatten()) != 2:
            raise ValueError(
                "crop_width must have 2 elements. Provided {}".format(
                    len(crop_width.flatten())
                )
            )

        input_shape = list(self.x.shape)
        ret_shape = (
            input_shape[:-2]
            + [input_shape[-2] - crop_height[0] - crop_height[1]]
            + [input_shape[-1] - crop_width[0] - crop_width[1]]
        )
        return types.tensor(self.x.dtype, ret_shape)
