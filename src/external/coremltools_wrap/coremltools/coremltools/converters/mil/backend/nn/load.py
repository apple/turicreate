# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import logging
from collections import defaultdict

from coremltools.converters.mil.input_types import (
    ClassifierConfig,
    ImageType,
    EnumeratedShapes,
    Shape,
    RangeDim,
)
from coremltools.models import MLModel
from coremltools.models import neural_network as neural_network
import coremltools.models.datatypes as datatypes
from coremltools.converters.mil.mil import types
from coremltools.converters.mil.mil.types.symbolic import (
    any_symbolic,
    any_variadic,
    is_symbolic,
)
from .op_mapping import convert_ops
from coremltools.models.neural_network import flexible_shape_utils
from coremltools.models.neural_network.flexible_shape_utils import (
    update_image_size_range,
    add_enumerated_image_sizes,
    set_multiarray_ndshape_range,
    add_multiarray_ndshape_enumeration,
)
from .passes.nn_passes import nn_backend_passes
from coremltools.converters._profile_utils import _profile


def _convert_to_image_input(proto, inputs):
    tmp_model = MLModel(proto)
    for input_type in inputs:
        if isinstance(input_type, ImageType):
            if input_type.color_layout == "G":
                gray_bias = input_type.bias
                red_bias, green_bias, blue_bias = 0.0, 0.0, 0.0
            elif input_type.color_layout == "RGB":
                gray_bias = 0.0
                red_bias, green_bias, blue_bias = input_type.bias
            elif input_type.color_layout == "BGR":
                gray_bias = 0.0
                blue_bias, green_bias, red_bias = input_type.bias
            tmp_model = neural_network.utils.make_image_input(
                tmp_model,
                input_type.name,
                is_bgr=input_type.color_layout == "BGR",
                image_format="NCHW" if input_type.channel_first else "NHWC",
                red_bias=red_bias,
                green_bias=green_bias,
                blue_bias=blue_bias,
                gray_bias=gray_bias,
                scale=input_type.scale,
            )
    return tmp_model.get_spec()


def _convert_to_classifier(proto, classifier_config):
    tmp_model = MLModel(proto)
    tmp_model = neural_network.utils.make_nn_classifier(
        tmp_model,
        classifier_config.class_labels,
        classifier_config.predicted_feature_name,
        classifier_config.predicted_probabilities_output,
    )
    return tmp_model.get_spec()


def _set_user_inputs(proto, inputs):
    for input_type in inputs:
        shape = input_type.shape
        if isinstance(shape, EnumeratedShapes):
            if isinstance(input_type, ImageType):
                image_sizes = []
                if input_type.image_config.channel_first:
                    for s in shape.shapes:
                        image_sizes.append(
                            flexible_shape_utils.NeuralNetworkImageSize(
                                s.shape[-2], s.shape[-1]
                            )
                        )
                else:
                    for s in shape.shapes:
                        image_sizes.append(
                            flexible_shape_utils.NeuralNetworkImageSize(
                                s.shape[-3], s.shape[-2]
                            )
                        )
                add_enumerated_image_sizes(
                    proto, input_type.name, image_sizes=image_sizes
                )
            else:
                add_multiarray_ndshape_enumeration(
                    proto, input_type.name, [tuple(s.shape) for s in shape.shapes]
                )
        elif isinstance(shape, Shape):
            shape = shape.shape  # This is shape in Shape
            if all(
                [
                    not isinstance(s, RangeDim) and not is_symbolic(s) and s > 0
                    for s in shape
                ]
            ):
                continue
            if isinstance(input_type, ImageType):
                img_range = flexible_shape_utils.NeuralNetworkImageSizeRange()
                if input_type.channel_first:
                    H = shape[-2]
                    W = shape[-1]
                else:
                    H = shape[-3]
                    W = shape[-2]

                if isinstance(H, RangeDim):
                    img_range.add_height_range((H.lower_bound, H.upper_bound))
                elif is_symbolic(H):
                    img_range.add_height_range((1, -1))
                else:
                    img_range.add_height_range((H, H))
                if isinstance(W, RangeDim):
                    img_range.add_width_range((W.lower_bound, W.upper_bound))
                elif is_symbolic(W):
                    img_range.add_width_range((1, -1))
                else:
                    img_range.add_width_range((W, W))

                flexible_shape_utils.update_image_size_range(
                    proto, input_type.name, img_range
                )
            else:
                lb = []
                ub = []
                for s in shape:
                    if isinstance(s, RangeDim):
                        lb.append(s.lower_bound)
                        ub.append(s.upper_bound)
                    elif is_symbolic(s):
                        lb.append(1)
                        ub.append(-1)
                    else:
                        lb.append(s)
                        ub.append(s)
                set_multiarray_ndshape_range(
                    proto, input_type.name, lower_bounds=lb, upper_bounds=ub
                )


def _set_symbolic_inputs(proto, symbolic_inputs):
    # Set symbolic input shapes by -1 infered from graph
    for input_name, shape in symbolic_inputs.items():
        lb = [1 if is_symbolic(d) else d for d in shape]
        ub = [-1 if is_symbolic(d) else d for d in shape]
        set_multiarray_ndshape_range(
            proto, input_name, lower_bounds=lb, upper_bounds=ub
        )

def _set_optional_inputs(proto, input_types):
    # Set default values for optional input_types
    default_map = {}
    for input_type in input_types:
        if isinstance(input_type, ImageType):
            continue
        is_optional = input_type.is_optional
        optional_value = input_type.optional_value
        shape = input_type.shape
        if not is_optional:
            continue
        msg = "Not support optional inputs for flexible input shape."
        if not isinstance(shape, Shape):
            raise NotImplementedError(msg)
        if any([isinstance(s, RangeDim) for s in shape.shape]):
            raise NotImplementedError(msg)

        default_map[input_type.name] = optional_value

    for idx, input in enumerate(proto.description.input):
        name = proto.description.input[idx].name
        if name in default_map:
            proto.description.input[idx].type.isOptional = True
            default_value = default_map[name] if default_map[name] is not None else 0.
            proto.description.input[idx].type.multiArrayType.floatDefaultValue = default_value


@_profile
def load(prog, **kwargs):
    if "main" not in prog.functions:
        msg = "main function not found in program {}"
        raise ValueError(msg.format(prog))
    if len(prog.functions) != 1:
        msg = (
            "Program must have exactly one `main` function to "
            "convert to NN. Program: {}"
        )
        raise ValueError(msg.format(prog))

    nn_backend_passes(prog)
    input_types = prog.main_input_types

    v1_inputs = []
    symbolic_inputs = {}
    for name, var in prog.functions["main"].inputs.items():
        if types.is_tensor(var.sym_type):
            sym_shape = var.sym_type.get_shape()
            if any_variadic(sym_shape):
                # TODO: rdar://59559656
                raise NotImplementedError("Variadic rank is not supported")
            if any_symbolic(sym_shape):
                user_specified = False
                for input_type in input_types:
                    if name == input_type.name:
                        sym_shape = input_type.shape.default
                        user_specified = True
                        break
                # Use dummy static shape, and will set it later.
                shape = [1 if is_symbolic(d) else d for d in sym_shape]
                if not user_specified:
                    symbolic_inputs[name] = sym_shape
            else:
                shape = sym_shape
            v1_inputs.append((name, datatypes.Array(*shape)))
        elif types.is_scalar(var.sym_type):
            v1_inputs.append((name, datatypes.Array(1)))
        else:
            raise NotImplementedError()

    v1_outputs = []
    for var in prog.functions["main"].outputs:
        if types.is_tensor(var.sym_type) or types.is_primitive(var.sym_type):
            # Disregard the output types
            v1_outputs.append((var.name, None))
        else:
            raise NotImplementedError()

    # create neural network builder
    builder = neural_network.NeuralNetworkBuilder(
        v1_inputs,
        v1_outputs,
        disable_rank5_shape_mapping=True,
        use_float_arraytype=True,
    )

    # const in V2 are added lazily to V1 by each op whenever needed.
    # `const_context` stores the const names we've added so far and avoid
    # adding a const more than once.
    # const_context: list[set of str] (const name for v1 & v2
    # (the same)). Note that in NN in outer layer is visible from the inner
    # layer, so the const_context is simply a stack of set.
    const_context = []
    # Iterate through ops and add to builder
    convert_ops(
        const_context,
        builder,
        prog.functions["main"].operations,
        prog.functions["main"].outputs,
    )

    # Replace model outputs's name with v1_outputs
    output_names = [x[0] for x in v1_outputs]
    for i, spec_layer in enumerate(builder.nn_spec.layers):
        for j, name in enumerate(spec_layer.output):
            for output_name in output_names:
                if output_name.split(":")[0] == name:
                    spec_layer.output[j] = output_name

    proto = builder.spec
    # image input
    has_image_input = any([isinstance(s, ImageType) for s in input_types])
    if has_image_input:
        proto = _convert_to_image_input(proto, input_types)

    # classifier flag
    classifier_config = kwargs.get("classifier_config", None)
    if classifier_config is not None:
        proto = _convert_to_classifier(proto, classifier_config)

    _set_user_inputs(proto, input_types)
    _set_symbolic_inputs(proto, symbolic_inputs)
    _set_optional_inputs(proto, input_types)

    return proto
