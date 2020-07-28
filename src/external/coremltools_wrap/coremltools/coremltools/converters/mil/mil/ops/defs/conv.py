#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil.mil.ops.defs._utils import spatial_dimensions_out_shape
from ._op_reqs import *


@register_op(doc_str="")
class conv(Operation):
    """
    Perform convolution over input, currently supporting only 1D and 2D
    convolution.

    Parameters
    ----------
    x: tensor<[n, C_in, *d_in], T> (Required)

        * ``d_in`` are (possibly runtime-determined) spatial dimensions (e.g., ``d_in = [224, 224]`` for 2D convolution).  ``1 <= len(d_in) <= 2`` (i.e., only 1D and 2D convolution) .
        * ``C_in`` is the number of input channels or depth dimensions.
        * ``n``  is the batch dimension

    weight: tensor<[C_out, C_in/groups, *K], T> (Required)

        * Filter weights. ``C_in`` is the number of input channels. ``C_in`` must be divisible by ``groups``
	* ``K`` are kernel sizes (e.g. ``K = [KH, KW]`` for 2D conv).
        * When ``dilations`` is not all 1, ``weight`` has to be const at compile time

    strides: const tensor<[S], i32> (Optional. Default to one vector of length equals to the number of spatial dimensions)
	* Strides along each of the spatial dimensions
	* ``S == len(d_in)``

    pad_type: const str (Required)

	* Must be one of the followings

            * ``valid``: No padding. This is equivalent to custom pad with ``pad[2*i] == pad[2*i+1] == 0, for i=0,...,len(d_in)-1``

            * ``same``: input is padded such that out spatial shapes are ``d_out[i] = ceil(d_in[i] / strides[i])``. Specifically, for ``i = 0,..,,len(d_in)-1``, the equivalent paddings are

		* When dilated kernel is even (i.e.  ``(K[i]-1)*dilations[i]+1)``
		    *  ``pad[2*i] = ceil[((K[i]-1)*dilations[i]+1)/2]``
		    *  ``pad[2*i+1] = floor[((K[i]-1)*dilations[i]+1)/2]``
		* Otherwise,  ``pad[2*i] = pad[2*i+1] = (K[i]-1) * dilations[i] / 2``
	    * custom: Specify custom padding in the parameter ``pad``

    pad: const tensor<[P], i32> (Optional. Default to all zeros)
	* ``len(P) = 2 * len(d_in)``
	* ``pad`` should be specified if and only if ``pad_type == custom``.  Errors otherwise.
        *  ``pad`` represents the number of elements to pad before and after each dimension. Specifically, ``pad[0], pad[1]`` are the pad size before / after spatial dimension 0, ``pad[2], pad[3]`` are the pad size before / after spatial dimension 1, etc.

    dilations: const tensor<[S], i32> (Optional. Default to all 1s)
	* Dilation value along each spatial dimension in ``d_in``. See `visualization <https://github.com/vdumoulin/conv_arithmetic/blob/master/README.md>`_
	* ``S == len(d_in)``

    groups: const tensor<[], i32> (Optional. Default to 1)
	* Input and output channels are split by ``groups``.

        * ``C_in`` must be divisible by ``groups``. Maximum value for group is ``C_in``, in which case it is a depthwise convolution.

	* For examples (assuming ``C_in = 16, C_out = 32``):

            * ``groups == 1``, ``weight`` has shape ``[32, 16, KH, KW]``: all input channels are convolved with ``weight`` kernel to produce all output channels

            * ``groups == 2``, ``weight`` has shape ``[32, 8, KH, KW]``: Input channel 0~7 are convolved with half of ``weight`` kernel to produce output channel 0~15. Similarly, input channel 8~15 are convolved with the other half of ``weight`` to product output channel 16~31.

            * ``groups == C_in``, ``weight`` has shape ``[32, 1, KH, KW]``: each input channel is convolved with its own set of filters and each produce ``C_out / C_in = 2`` channels. This is equivalent to depthwise convolution.

    bias: const tensor<[C_out],T> (Optional. Default to all 0)
	* Bias along output channels


    Returns
    -------
    tensor<[n, C_out, *d_out], T>
        * Output activation has the same rank and spatial dimension as the input (i.e., ``len(d_out) == len(d_in)``)
	* For ``i=0,..,len(d_in)-1, d_out[i] = floor [(D_in[i] + pad[2*i] + pad[2*i+1] - (K[i]-1)*dilations[i] - 1) / strides[i] ] + 1``

    Attributes
    ----------
    T: fp32

    See Also
    --------
    conv_transpose
    """

    input_spec = InputSpec(
        x=TensorInputType(),
        weight=TensorInputType(),
        bias=TensorInputType(const=True, optional=True, default=None),
        strides=IntTensorInputType(const=True, optional=True, default=None),
        pad_type=StringInputType(const=True, optional=True, default="valid"),
        pad=IntTensorInputType(const=True, optional=True, default=None),
        dilations=IntTensorInputType(const=True, optional=True, default=None),
        groups=IntInputType(const=True, optional=True, default=1),
    )

    def __init__(self, **kwargs):
        super(conv, self).__init__(**kwargs)

    def type_inference(self):
        inshape = self.x.shape
        f_shape = self.weight.shape
        kernel_shape = f_shape[2:]
        num_dims = len(inshape) - 2
        C_out = f_shape[0]
        C_in = self.x.shape[1]
        groups = self.groups.val

        if self.bias is not None and self.bias.val.shape[0] != C_out:
            msg = "# of bias values {} not equal to # output channels {}"
        if C_in % groups != 0:
            msg = "# of input channels {} not divisible by groups {}"
            raise ValueError(msg.format(C_in, groups))
        if C_in // groups != self.weight.shape[1]:
            msg = "C_in / groups = {}/{} != weight[1] ({})"
            raise ValueError(msg.format(C_in, groups, self.weight.shape[1]))

        strides = [1] * num_dims if self.strides is None else self.strides.val
        dilations = [1] * num_dims if self.dilations is None else self.dilations.val
        custom_pad = None if self.pad is None else self.pad.val
        N = inshape[0]
        C_out = f_shape[0]
        # spatial dimensions
        d_out_shape = spatial_dimensions_out_shape(
            pad_type=self.pad_type.val,
            input_shape=inshape[2:],
            kernel_shape=kernel_shape,
            strides=strides,
            dilations=dilations,
            custom_pad=custom_pad,
        )
        retshape = [N, C_out] + d_out_shape
        return types.tensor(self.x.dtype, tuple(retshape))


@register_op(doc_str="")
class conv_transpose(Operation):
    """
    Perform transposed convolution (aka deconvolution, fractionally stride
    convolution) over input. ``conv_transpose`` can also be used to compute
    gradient of conv. Currently support 1D and 2D only.

    Parameters
    ----------

    x: tensor<[n,C_in,*D_in],T>  (Required)
	* Input data. ``D_in`` are spatial dimensions. ``1 <= len(D_in) <= 2``
        and ``C_in`` is the number of input channels.

    weight: const tensor<[C_out,C_in/groups,*D_in], T> (Required)
	* Filter weights. ``C_in, C_out`` are the number of input and output channels respectively.
	* ``D_in`` are spatial dimensions. ``1 <= len(D_in) <= 2``

    bias: const tensor<[C_out],T> (Optional. Default to all 0)
	* Bias added along output channels

    pad: const tensor<[P],i32> (Optional. Default to all 0s)
	* Number of elements to pad before and after each dimension
	* ``P == 2 * len(D_in)``
	* ``pad[2*i], pad[2*i+1]`` are pad sizes before and after dimension ``i``, where ``0 <= i < len(D_in)``

    output_shape: const tensor<[P],i32> (Optional. Default None)
	* Expected output shape
	* Padding is computed using pad_type and output_shape if provided

    pad_type: const tensor<[P],i32> (Optional. Default valid)
	* One of ``same``, ``valid``, or ``custom``

    strides: const tensor<[S],i32> (Optional. Default to all 1s)
	* Stride along each of the spatial dimensions. ``S == len(D_in)``

    dilations: const tensor<[S],i32> (Optional. Default to all 1s)
	* Dilation value along each spatial dimension in ``d_in``. See `conv`.
        * ``S == len(D_in)``

    groups: const tensor<[], i32> (Optional. Default to 1)
	* Input and output channels are separated into ``groups``. ``C_in`` and ``C_out`` must be divisible by the number of groups. See ``conv`` for examples.

    Returns
    -------
    tensor<[n,C_out,*D_out],T>
	* ``D_out[i] = (D_in[i]-1) * strides[i] + pad[2*i] + pad[2*i+1] - (K[i] - 1) * dilations[i] + 1)] for i = 0, 1``

    Attributes
    ----------
    T: fp32

    See Also
    --------
    conv
    """

    input_spec = InputSpec(
        x=TensorInputType(),  # [n, C_in, spatial_dims]
        weight=TensorInputType(const=True),  # [C_out, C_in, spatial_dims]
        bias=TensorInputType(const=True, optional=True, default=None),
        pad=IntTensorInputType(const=True, optional=True, default=None),
        output_shape=IntTensorInputType(const=True, optional=True, default=None),
        pad_type=StringInputType(const=True, optional=True, default="valid"),
        strides=TensorInputType(const=True, optional=True, default=None),
        dilations=TensorInputType(const=True, optional=True, default=None),
        groups=IntInputType(const=True, optional=True, default=1),
    )

    def __init__(self, **kwargs):
        super(conv_transpose, self).__init__(**kwargs)

    def type_inference(self):
        # Input shape is [n, C_in, spatial_dims]
        in_shape = self.x.shape
        # Weight shape is [C_out, C_in, spatial_dims]
        f_shape = self.weight.shape
        kernel_shape = f_shape[2:]
        spatial_dim_rank = len(in_shape) - 2
        N = in_shape[0]
        C_in = self.x.shape[1]
        groups = self.groups.val
        C_out = f_shape[0] * groups

        if self.bias is not None and self.bias.val.shape[0] != C_out:
            msg = "# of bias values {} not equal to # output channels {}"
            raise ValueError(msg.format(self.bias.val.shape[0], C_out))
        if C_out % groups != 0:
            msg = "# of input channels {} not divisible by groups {}"
            raise ValueError(msg.format(C_in, groups))

        # If output shape is given, return it
        if self.output_shape is not None:
            return types.tensor(
                self.x.dtype, tuple([N, C_out] + list(self.output_shape.val))
            )

        strides = [1] * spatial_dim_rank if self.strides is None else self.strides.val
        dilations = (
            [1] * spatial_dim_rank if self.dilations is None else self.dilations.val
        )
        kernel_shape = [
            (kernel_shape[r] - 1) * dilations[r] + 1 for r in range(spatial_dim_rank)
        ]

        D_in = in_shape[2:]  # spatial dimensions

        # Deconv's output shape is non-deterministic, we follow TF shape logic here.
        if self.pad_type.val == "same":
            d_out_shape = [strides[r] * D_in[r] for r in range(spatial_dim_rank)]
        elif self.pad_type.val == "valid":
            d_out_shape = [
                strides[r] * D_in[r] + kernel_shape[r] - 1
                for r in range(spatial_dim_rank)
            ]
        elif self.pad_type.val == "custom":
            if self.pad is None:
                raise ValueError("self.pad must exist if pad_type is custom")
            pad = self.pad.val
            d_out_shape = [
                strides[r] * (D_in[r] - 1)
                + kernel_shape[r]
                - pad[2 * r]
                - pad[2 * r + 1]
                for r in range(spatial_dim_rank)
            ]

        retshape = [N, C_out] + d_out_shape
        return types.tensor(self.x.dtype, tuple(retshape))
