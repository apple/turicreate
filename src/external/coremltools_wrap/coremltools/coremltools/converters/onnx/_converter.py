from __future__ import absolute_import as _
from __future__ import division as _
from __future__ import print_function as _
from __future__ import unicode_literals as _
from typing import Text, Union, Optional, Dict, Any, Iterable, Sequence, Callable, List

import numpy as np

from coremltools._deps import _HAS_ONNX

if _HAS_ONNX:
    import onnx
    from onnx import shape_inference
    from onnx import TensorProto

from coremltools.models.neural_network import NeuralNetworkBuilder  # type: ignore
from coremltools.models import datatypes, MLModel  # type: ignore
from coremltools.proto import FeatureTypes_pb2 as ft  # type: ignore
from coremltools import (
    _MINIMUM_CUSTOM_LAYER_SPEC_VERSION as IOS_11_2_SPEC_VERSION,
)  # iOS 11.2
from coremltools import (
    _MINIMUM_CUSTOM_MODEL_SPEC_VERSION as IOS_12_SPEC_VERSION,
)  # iOS 12.0
from coremltools import _MINIMUM_NDARRAY_SPEC_VERSION as IOS_13_SPEC_VERSION  # iOS 13.0
from coremltools import __version__ as ct_version
from coremltools.models import _METADATA_VERSION, _METADATA_SOURCE
from typing import Tuple

from ._operators import (
    _convert_node,
    _SEQUENCE_LAYERS_REGISTRY,
    _ONNX_NODE_REGISTRY,
    _add_const_inputs_if_required,
)
from ._operators_nd import _ONNX_NODE_REGISTRY_ND, _convert_node_nd

from ._graph import Graph, EdgeInfo, Transformer

from ._transformers import (
    ConvAddFuser,
    DropoutRemover,
    ReshapeInitTensorFuser,
    BNBroadcastedMulFuser,
    BNBroadcastedAddFuser,
    PixelShuffleFuser,
    OutputRenamer,
    AddModelInputsOutputs,
    ConstantsToInitializers,
    ImageScalerRemover,
    ShapeOpRemover,
    ConstantRemover,
    ConstantFillToInitializers,
    ReshapeTransposeReshape_pattern1,
    CastOpRemover,
    DeadCodeElimination,
    PaddingOpRemover,
)

# ML model passes
from coremltools.converters.mil.backend.nn.passes.mlmodel_passes import (
    remove_disconnected_layers,
    transform_conv_crop,
)

from ._error_utils import ErrorHandling
from ._graph_viz import plot_graph  # type: ignore

USE_SHAPE_MAPPING = True

DEBUG = False


class SupportedVersion:
    # Supported iOS Version
    # New OS Version must be added at the end to maintain backward version index
    supported_ios_version = ["11.2", "12", "13"]
    IOS_13_VERSION = supported_ios_version.index("13")
    ND_ARRARY_SUPPORT = IOS_13_VERSION

    @staticmethod
    def ios_support_check(minimum_ios_deployment_target):
        return minimum_ios_deployment_target in SupportedVersion.supported_ios_version

    @staticmethod
    def is_nd_array_supported(minimum_ios_deployment_target):
        if not SupportedVersion.ios_support_check(minimum_ios_deployment_target):
            raise TypeError(
                "{} not supported. Please provide one of target iOS: {}",
                minimum_ios_deployment_target,
                SupportedVersion.supported_ios_version,
            )

        minimum_ios_deployment_target_index = SupportedVersion.supported_ios_version.index(
            minimum_ios_deployment_target
        )
        return SupportedVersion.ND_ARRARY_SUPPORT <= minimum_ios_deployment_target_index

    @staticmethod
    def get_supported_ios():
        return SupportedVersion.supported_ios_version

    @staticmethod
    def get_specification_version(minimum_ios_deployment_target):
        if not SupportedVersion.ios_support_check(minimum_ios_deployment_target):
            raise TypeError(
                "{} not supported. Please provide one of target iOS: {}",
                minimum_ios_deployment_target,
                SupportedVersion.supported_ios_version,
            )

        if minimum_ios_deployment_target == "11.2":
            return IOS_11_2_SPEC_VERSION
        elif minimum_ios_deployment_target == "12":
            return IOS_12_SPEC_VERSION
        else:
            return IOS_13_SPEC_VERSION


"""
inputs: list of tuples.
      [Tuple]: [(name, type, shape)]
"""


def _make_coreml_input_features(
    graph, onnx_coreml_input_shape_map, disable_coreml_rank5_mapping=False
):  # type: (...) -> Sequence[Tuple[Text, datatypes.Array]]
    """
    If "disable_coreml_rank5_mapping" is False, then:

    ONNX shapes to CoreML static shapes mapping
    length==1: [C]
    length==2: [B,C]
    length==3: [C,H,W] or [Seq,B,C]
    length==4: [B,C,H,W]

    If "disable_coreml_rank5_mapping" is True, then
    onnx shapes are mapped "as is" to CoreML.
    """
    inputs = graph.inputs
    op_types = graph.blob_to_op_type
    features = []
    for input_ in inputs:
        shape = input_[2]
        if disable_coreml_rank5_mapping:
            if len(shape) > 5:
                raise ValueError(
                    "ONNX input %s has a rank greater than 5, which is not supported in CoreML framework"
                    % str(input_[0])
                )
            else:
                features.append((str(input_[0]), datatypes.Array(*shape)))
            continue

        if USE_SHAPE_MAPPING and input_[0] in onnx_coreml_input_shape_map:
            mapp = onnx_coreml_input_shape_map[input_[0]]
            if len(mapp) != len(shape):
                raise ValueError(
                    "Incorrect value in onnx_coreml_input_shape_map argument"
                )
            graph.onnx_coreml_shape_mapping[input_[0]] = mapp
            coreml_shape = [1, 1, 1]
            for i in range(3):
                if (i + 2) in mapp:
                    coreml_shape[i] = shape[mapp.index(i + 2)]
            shape = coreml_shape
        else:
            if len(shape) == 0:
                shape = [1, 1, 1]
            elif len(shape) == 1:
                # assume [C]
                if USE_SHAPE_MAPPING:
                    graph.onnx_coreml_shape_mapping[input_[0]] = [2]
            elif len(shape) == 2:
                # assume [Batch,C]
                shape = [shape[1]]
                if USE_SHAPE_MAPPING:
                    graph.onnx_coreml_shape_mapping[input_[0]] = [1, 2]
            elif len(shape) == 3:
                # assume [C,H,W] unless its connected an op that bestows another mapping
                if input_[0] in op_types and len(op_types[input_[0]]) == 1:
                    if str(op_types[input_[0]][0]) in _SEQUENCE_LAYERS_REGISTRY:
                        # (Seq,B,C)
                        shape = [shape[2]]
                        if USE_SHAPE_MAPPING:
                            graph.onnx_coreml_shape_mapping[input_[0]] = [0, 1, 2]
                    elif str(op_types[input_[0]][0]) in [
                        "MaxPool",
                        "AveragePool",
                        "BatchNormalization",
                        "GlobalAveragePool",
                        "GlobalLpPool",
                        "GlobalMaxPool",
                        "InstanceNormalization",
                        "LRN",
                        "LpPool",
                        "Conv",
                        "ConvTranspose",
                    ]:
                        # (B,C,W)
                        shape = [shape[1], 1, shape[2]]
                        if USE_SHAPE_MAPPING:
                            graph.onnx_coreml_shape_mapping[input_[0]] = [1, 2, 4]
                    else:
                        if USE_SHAPE_MAPPING:
                            graph.onnx_coreml_shape_mapping[input_[0]] = [2, 3, 4]
                else:
                    if USE_SHAPE_MAPPING:
                        graph.onnx_coreml_shape_mapping[input_[0]] = [2, 3, 4]
            elif len(shape) == 4:  # (B,C,H,W) --> (C,H,W)
                shape = shape[1:]
                if USE_SHAPE_MAPPING:
                    graph.onnx_coreml_shape_mapping[input_[0]] = [1, 2, 3, 4]
            else:
                raise ValueError(
                    "CoreML input cannot be more than rank 4. Input shape: %s, input: '%s' "
                    % (str(shape), str(input_[0]))
                )
        features.append((str(input_[0]), datatypes.Array(*shape)))
    return features


"""
outputs: list of tuples.
      [Tuple]: [(name, type, shape)]
"""


def _make_coreml_output_features(
    graph, forceShape=False, disable_coreml_rank5_mapping=False
):  # type: (...) -> Sequence[Tuple[Text, datatypes.Array]]
    features = []
    outputs = graph.outputs
    op_types = graph.blob_from_op_type
    ops_allowing_zerod_output = {"Size"}

    for output_ in outputs:
        if op_types[output_[0]] in ops_allowing_zerod_output and len(output_[2]) == 0:
            output_ = list(output_)
            output_[2] = (1,)

        if disable_coreml_rank5_mapping:
            shape = output_[2]
            if len(shape) > 5:
                raise ValueError(
                    "ONNX output %s has a rank greater than 5, which is not supported in CoreML framework"
                    % str(output_[0])
                )
            else:
                features.append((str(output_[0]), datatypes.Array(*shape)))
            continue

        if not forceShape:
            features.append((str(output_[0]), None))
        else:
            shape = output_[2]
            if len(shape) == 0:
                shape = [1, 1, 1]
            elif len(shape) == 1:
                pass
            elif len(shape) == 3:
                if (
                    output_[0] in op_types
                    and str(op_types[output_[0]]) in _SEQUENCE_LAYERS_REGISTRY
                ):
                    # onnx shape: (Seq,B,C)
                    shape = [shape[2]]
            elif len(shape) == 4:  # (B,C,H,W) --> (C,H,W)
                shape = shape[1:]
            else:
                shape = None  # output shape need not be specified for CoreML.
            if shape is None:
                features.append((str(output_[0]), shape))
            else:
                features.append((str(output_[0]), datatypes.Array(*shape)))
    return features


def _check_unsupported_ops(
    nodes, disable_coreml_rank5_mapping=False
):  # type: (...) -> None
    unsupported_op_types = []  # type: List[Text]
    for node in nodes:

        if disable_coreml_rank5_mapping:
            if (
                node.op_type not in _ONNX_NODE_REGISTRY_ND
                and node.op_type not in unsupported_op_types
            ):
                unsupported_op_types.append(node.op_type)
            continue

        if (
            node.op_type not in _ONNX_NODE_REGISTRY
            and node.op_type not in unsupported_op_types
        ):
            unsupported_op_types.append(node.op_type)

    coreml_3_rerun_message = ""
    if not disable_coreml_rank5_mapping:
        coreml_3_rerun_message = (
            "\nPlease try converting again by providing the additonal argument, "
            "minimum_ios_deployment_target=13"
            " and making sure you have the latest coremltools package"
        )
    if len(unsupported_op_types) > 0:
        raise NotImplementedError(
            "Unsupported ONNX ops of type: %s %s"
            % (",".join(unsupported_op_types), coreml_3_rerun_message)
        )


def _update_multiarray_to_float32(
    feature,  # type: Any
):  # type : (...) -> None
    if feature.type.HasField("multiArrayType"):
        feature.type.multiArrayType.dataType = ft.ArrayFeatureType.FLOAT32


def _update_multiarray_to_int32(
    feature,  # type: Any
):  # type : (...) -> None
    if feature.type.HasField("multiArrayType"):
        feature.type.multiArrayType.dataType = ft.ArrayFeatureType.INT32


def _transform_coreml_dtypes(
    builder,  # type : NeuralNetworkBuilder
    inputs,  # type: List[EdgeInfo]
    outputs,  # type: List[EdgeInfo]
):
    # type: (...) -> None

    """ Make sure ONNX input/output data types are mapped to the equivalent CoreML types
    """
    for i, input_ in enumerate(inputs):
        onnx_type = input_[1]
        if onnx_type == TensorProto.FLOAT:
            _update_multiarray_to_float32(builder.spec.description.input[i])
        elif onnx_type == TensorProto.DOUBLE:
            continue
        elif onnx_type == TensorProto.INT32 or onnx_type == TensorProto.INT64:
            _update_multiarray_to_int32(builder.spec.description.input[i])
        elif onnx_type == TensorProto.BOOL:
            _update_multiarray_to_float32(builder.spec.description.input[i])
        else:
            raise TypeError("Input must be of of type FLOAT, DOUBLE, INT32 or INT64")

    for i, output_ in enumerate(outputs):
        onnx_type = output_[1]
        if onnx_type == TensorProto.FLOAT:
            _update_multiarray_to_float32(builder.spec.description.output[i])
        elif onnx_type == TensorProto.DOUBLE:
            continue
        elif onnx_type == TensorProto.INT32 or onnx_type == TensorProto.INT64:
            _update_multiarray_to_int32(builder.spec.description.output[i])
        elif onnx_type == TensorProto.BOOL:
            _update_multiarray_to_float32(builder.spec.description.output[i])
        else:
            raise TypeError("Output must be of of type FLOAT, DOUBLE, INT32 or INT64")


def _convert_multiarray_output_to_image(
    spec,  # type: Any
    feature_name,  # type: Text
    is_bgr=False,  # type: bool
):
    # type: (...) -> None
    for output in spec.description.output:
        if output.name != feature_name:
            continue
        if output.type.WhichOneof("Type") != "multiArrayType":
            raise ValueError("{} is not a multiarray type".format(output.name,))
        array_shape = tuple(output.type.multiArrayType.shape)
        if len(array_shape) == 2:
            height, width = array_shape
            output.type.imageType.colorSpace = ft.ImageFeatureType.ColorSpace.Value(
                "GRAYSCALE"
            )
        else:
            if len(array_shape) == 4:
                if array_shape[0] != 1:
                    raise ValueError(
                        "Shape {} is not supported for image output".format(
                            array_shape,
                        )
                    )
                array_shape = array_shape[1:]

            channels, height, width = array_shape

            if channels == 1:
                output.type.imageType.colorSpace = ft.ImageFeatureType.ColorSpace.Value(
                    "GRAYSCALE"
                )
            elif channels == 3:
                if is_bgr:
                    output.type.imageType.colorSpace = ft.ImageFeatureType.ColorSpace.Value(
                        "BGR"
                    )
                else:
                    output.type.imageType.colorSpace = ft.ImageFeatureType.ColorSpace.Value(
                        "RGB"
                    )
            else:
                raise ValueError(
                    "Channel Value {} is not supported for image output".format(
                        channels,
                    )
                )

        output.type.imageType.width = width
        output.type.imageType.height = height


def _set_deprocessing(
    is_grayscale,  # type: bool
    builder,  # type: NeuralNetworkBuilder
    deprocessing_args,  # type: Dict[Text, Any]
    input_name,  # type: Text
    output_name,  # type: Text
):
    # type: (...) -> None
    is_bgr = deprocessing_args.get("is_bgr", False)

    image_scale = deprocessing_args.get("image_scale", 1.0)

    if is_grayscale:
        gray_bias = deprocessing_args.get("gray_bias", 0.0)
        W = np.array([image_scale])
        b = np.array([gray_bias])
    else:
        W = np.array([image_scale, image_scale, image_scale])

        red_bias = deprocessing_args.get("red_bias", 0.0)
        green_bias = deprocessing_args.get("green_bias", 0.0)
        blue_bias = deprocessing_args.get("blue_bias", 0.0)

        if not is_bgr:
            b = np.array([red_bias, green_bias, blue_bias,])
        else:
            b = np.array([blue_bias, green_bias, red_bias,])
    builder.add_scale(
        name=input_name,
        W=W,
        b=b,
        has_bias=True,
        shape_scale=W.shape,
        shape_bias=b.shape,
        input_name=input_name,
        output_name=output_name,
    )


def _prepare_onnx_graph(
    graph, transformers, onnx_ir_version
):  # type: (Graph, Iterable[Transformer]) -> Graph
    graph_ = Graph.from_onnx(graph, onnx_ir_version)
    if DEBUG:
        plot_graph(graph_, graph_img_path="/tmp/graph_raw.pdf")
    graph_ = graph_.transformed(transformers)
    if DEBUG:
        plot_graph(graph_, graph_img_path="/tmp/graph_opt.pdf")
    return graph_


def convert(
    model,  # type: Union[onnx.ModelProto, Text]
    mode=None,  # type: Optional[Text]
    image_input_names=[],  # type: Sequence[Text]
    preprocessing_args={},  # type: Dict[Text, Any]
    image_output_names=[],  # type: Sequence[Text]
    deprocessing_args={},  # type: Dict[Text, Any]
    class_labels=None,  # type: Union[Text, Iterable[Text], None]
    predicted_feature_name="classLabel",  # type: Text
    add_custom_layers=False,  # type: bool
    custom_conversion_functions={},  # type: Dict[Text, Any]
    onnx_coreml_input_shape_map={},  # type: Dict[Text, List[int,...]]
    minimum_ios_deployment_target="12",
):
    # type: (...) -> MLModel
    """
    Convert ONNX model to CoreML.
    Parameters
    ----------
    model:
        An ONNX model with parameters loaded in onnx package or path to file
        with models.
    mode: 'classifier', 'regressor' or None
        Mode of the converted coreml model:
        'classifier', a NeuralNetworkClassifier spec will be constructed.
        'regressor', a NeuralNetworkRegressor spec will be constructed.
    preprocessing_args:
        'is_bgr', 'red_bias', 'green_bias', 'blue_bias', 'gray_bias',
        'image_scale' keys with the same meaning as
        https://apple.github.io/coremltools/generated/coremltools.models.neural_network.html#coremltools.models.neural_network.NeuralNetworkBuilder.set_pre_processing_parameters
    deprocessing_args:
        Same as 'preprocessing_args' but for deprocessing.
    class_labels:
        As a string it represents the name of the file which contains
        the classification labels (one per line).
        As a list of strings it represents a list of categories that map
        the index of the output of a neural network to labels in a classifier.
    predicted_feature_name:
        Name of the output feature for the class labels exposed in the Core ML
        model (applies to classifiers only). Defaults to 'classLabel'
    add_custom_layers: bool
        Flag to turn on addition of custom CoreML layers for unsupported ONNX ops or attributes within
        a supported op.
    custom_conversion_functions: dict()
        A dictionary with keys corresponding to the names/types of onnx ops and values as functions taking
        an object of class coreml-tools's 'NeuralNetworkBuilder', Graph' (see onnx-coreml/_graph.Graph),
        'Node' (see onnx-coreml/_graph.Node), ErrorHandling (see onnx-coreml/_error_utils.ErrorHandling).
        This custom conversion function gets full control and responsibility for converting given onnx op.
        This function returns nothing and is responsible for adding a equivalent CoreML layer via 'NeuralNetworkBuilder'
    onnx_coreml_input_shape_map: dict()
        (Optional) A dictionary with keys corresponding to the model input names. Values are a list of integers that specify
        how the shape of the input is mapped to CoreML. Convention used for CoreML shapes is
        0: Sequence, 1: Batch, 2: channel, 3: height, 4: width.
        For example, an input of rank 2 could be mapped as [3,4] (i.e. H,W) or [1,2] (i.e. B,C) etc.
        This is ignored if "minimum_ios_deployment_target" is set to 13.
    minimum_ios_deployment_target: str
        Target Deployment iOS Version (default: '12')
        Supported iOS version options: '11.2', '12', '13'
        CoreML model produced by the converter will be compatible with the iOS version specified in this argument.
        e.g. if minimum_ios_deployment_target = '12', the converter would only utilize CoreML features released till iOS12 (equivalently macOS 10.14, watchOS 5 etc).

        iOS 11.2 (CoreML 0.8) does not support resize_bilinear, crop_resize layers
         - (Supported features: https://github.com/apple/coremltools/releases/tag/v0.8)
        iOS 12 (CoreML 2.0)
         - (Supported features: https://github.com/apple/coremltools/releases/tag/v2.0)
        iSO 13 (CoreML 3.0)
         - (Supported features: https://github.com/apple/coremltools/releases/tag/3.0-beta6)

    Returns
    -------
    model: A coreml model.
    """
    if isinstance(model, Text):
        onnx_model = onnx.load(model)
    elif isinstance(model, onnx.ModelProto):
        onnx_model = model
    else:
        raise TypeError("Model must be file path to .onnx file or onnx loaded model")

    if not SupportedVersion.ios_support_check(minimum_ios_deployment_target):
        raise TypeError(
            "{} not supported. Please provide one of target iOS: {}",
            minimum_ios_deployment_target,
            SupportedVersion.get_supported_ios(),
        )

    global USE_SHAPE_MAPPING
    disable_coreml_rank5_mapping = False
    if SupportedVersion.is_nd_array_supported(minimum_ios_deployment_target):
        disable_coreml_rank5_mapping = True

    if disable_coreml_rank5_mapping:
        USE_SHAPE_MAPPING = False
    else:
        USE_SHAPE_MAPPING = True

    """
    First, apply a few optimizations to the ONNX graph,
    in preparation for conversion to CoreML.
    """

    # Using Dummy transformation to conditionally disable certain transformation
    class DummyTransformation(object):
        def __call__(self, graph):
            return graph

    transformers = [
        ConstantsToInitializers(),
        ShapeOpRemover(),
        ConstantRemover(),
        CastOpRemover(),
        PaddingOpRemover(),
        ReshapeInitTensorFuser(),
        DropoutRemover(),
        DeadCodeElimination(),
        ConvAddFuser(),
        BNBroadcastedMulFuser(),
        BNBroadcastedAddFuser(),
        ReshapeTransposeReshape_pattern1(),
        PixelShuffleFuser(),
        AddModelInputsOutputs()
        if not disable_coreml_rank5_mapping
        else DummyTransformation(),
        ConstantFillToInitializers(),
    ]  # type: Iterable[Transformer]

    onnx_model = onnx.shape_inference.infer_shapes(onnx_model)
    graph = _prepare_onnx_graph(onnx_model.graph, transformers, onnx_model.ir_version)

    """
    Check for ImageScalar nodes in ONNX, this will indicate whether input image preprocessing needs
    to be added to the CoreML graph or not.
    """
    # are there ImageScaler nodes in the Graph?
    # If yes then add the info from it to the "preprocessing_args" dictionary, if the dictionary is not
    # already provided by the user
    if not bool(preprocessing_args):
        for node in graph.nodes:
            if node.op_type == "ImageScaler":
                inp_name = node.inputs[0]
                scale = node.attrs.get("scale", 1.0)
                bias = node.attrs.get("bias", [0, 0, 0])
                if not (len(bias) == 1 or len(bias) == 3):
                    continue
                if "image_scale" in preprocessing_args:
                    preprocessing_args["image_scale"][inp_name] = scale
                else:
                    preprocessing_args["image_scale"] = {inp_name: scale}
                if len(bias) == 3:
                    for i, color in enumerate(["red", "green", "blue"]):
                        if color + "_bias" in preprocessing_args:
                            preprocessing_args[color + "_bias"][inp_name] = bias[i]
                        else:
                            preprocessing_args[color + "_bias"] = {inp_name: bias[i]}
                else:
                    if "gray_bias" in preprocessing_args:
                        preprocessing_args["gray_bias"][inp_name] = bias[0]
                    else:
                        preprocessing_args["gray_bias"] = {inp_name: bias[0]}
                if inp_name not in image_input_names:
                    image_input_names.append(inp_name)  # type: ignore

    # remove all ImageScaler ops
    graph = graph.transformed([ImageScalerRemover()])

    """
    Gather information (name, shape) for model inputs and outputs
    This information is then used to initialize the neural network builder object of coremltools.
    The builder object is later used to add layers to the CoreML model.
    """

    # Make CoreML input and output features by gathering shape info and
    # interpreting it for CoreML
    input_features = _make_coreml_input_features(
        graph, onnx_coreml_input_shape_map, disable_coreml_rank5_mapping
    )
    if len(image_output_names) > 0:
        output_features = _make_coreml_output_features(
            graph,
            forceShape=True,
            disable_coreml_rank5_mapping=disable_coreml_rank5_mapping,
        )
    else:
        output_features = _make_coreml_output_features(
            graph, disable_coreml_rank5_mapping=disable_coreml_rank5_mapping
        )

    builder = NeuralNetworkBuilder(
        input_features,
        output_features,
        mode=mode,
        disable_rank5_shape_mapping=disable_coreml_rank5_mapping,
    )

    # TODO: To be removed once, auto-downgrading of spec version is enabled
    builder.spec.specificationVersion = SupportedVersion.get_specification_version(
        minimum_ios_deployment_target
    )

    """
    Set CoreML input,output types (float, double, int) same as onnx types, if supported
    """
    _transform_coreml_dtypes(builder, graph.inputs, graph.outputs)

    """what follows is some book-keeping to support outputs of type image.
    """

    is_deprocess_bgr_only = (len(deprocessing_args) == 1) and (
        "is_bgr" in deprocessing_args
    )
    add_deprocess = (
        (len(image_output_names) > 0)
        and (len(deprocessing_args) > 0)
        and (not is_deprocess_bgr_only)
    )

    if add_deprocess:
        mapping = {}
        for f in output_features:
            output_name = f[0]
            mapping[output_name] = graph.get_unique_edge_name(output_name)
        graph = OutputRenamer(mapping)(graph)

    if len(image_input_names) > 0:
        builder.set_pre_processing_parameters(
            image_input_names=image_input_names,
            is_bgr=preprocessing_args.get("is_bgr", False),
            red_bias=preprocessing_args.get("red_bias", 0.0),
            green_bias=preprocessing_args.get("green_bias", 0.0),
            blue_bias=preprocessing_args.get("blue_bias", 0.0),
            gray_bias=preprocessing_args.get("gray_bias", 0.0),
            image_scale=preprocessing_args.get("image_scale", 1.0),
        )

    preprocessing_args.clear()

    if len(image_output_names) > 0:
        for f in output_features:
            f_name = f[0]
            if f_name in image_output_names:
                is_bgr = deprocessing_args.get("is_bgr", False)
                _convert_multiarray_output_to_image(builder.spec, f_name, is_bgr=is_bgr)

    """
    Iterate through all the ONNX ops and translate them to CoreML layers, one by one.
    """

    """
    before proceeding to start the layer translation process,
    check whether there is an op in the ONNX graph, whose translation function is not yet
    implemented in the converter or which is not supported in the CoreML framework. If so,
    raise an error before starting the process.
    (if the user desires to add a custom layer then this check is not required)
    """
    if not add_custom_layers:
        _check_unsupported_ops(graph.nodes, disable_coreml_rank5_mapping)

    """
    ErrorHandling is a generic class, useful to store a variety of parameters during the conversion process
    """
    err = ErrorHandling(add_custom_layers, custom_conversion_functions)

    for i, node in enumerate(graph.nodes):
        print(
            "%d/%d: Converting Node Type %s" % (i + 1, len(graph.nodes), node.op_type)
        )
        if disable_coreml_rank5_mapping:
            _convert_node_nd(builder, node, graph, err)
        else:
            _add_const_inputs_if_required(builder, node, graph, err)
            _convert_node(builder, node, graph, err)

    if DEBUG:
        plot_graph(
            graph,
            graph_img_path="/tmp/after_conversion.pdf",
            show_coreml_mapped_shapes=not disable_coreml_rank5_mapping,
        )

    if add_deprocess:
        for f in output_features:
            output_name = f[0]
            if output_name not in image_output_names:
                continue
            output_shape = f[1].dimensions
            if len(output_shape) == 2 or output_shape[0] == 1:
                is_grayscale = True
            elif output_shape[0] == 3:
                is_grayscale = False
            else:
                raise ValueError("Output must be RGB image or Grayscale")
            _set_deprocessing(
                is_grayscale,
                builder,
                deprocessing_args,
                mapping[output_name],
                output_name,
            )

    if class_labels is not None:
        if isinstance(class_labels, Text):
            labels = [
                l.strip() for l in open(class_labels).readlines()
            ]  # type: Sequence[Text]
        elif isinstance(class_labels, list):
            labels = class_labels
        else:
            raise TypeError(
                "synset variable of unknown type. Type found: {}. \
                Expected either string or list of strings.".format(
                    type(class_labels),
                )
            )

        builder.set_class_labels(
            class_labels=labels, predicted_feature_name=predicted_feature_name
        )

    def _add_informative_description(feature, raise_error=True):
        if feature.type.WhichOneof("Type") == "multiArrayType":
            if (
                feature.name in graph.onnx_coreml_shape_mapping
                and feature.name in graph.shape_dict
            ):
                mapp = graph.onnx_coreml_shape_mapping[feature.name]
                onnx_shape = graph.shape_dict[feature.name]
                if raise_error:
                    assert len(mapp) == len(onnx_shape), "Something wrong in shape"
                if len(mapp) == len(onnx_shape):
                    shape = []
                    for i in range(5):
                        if i in mapp:
                            shape += [int(onnx_shape[mapp.index(i)])]
                        else:
                            shape += [1]
                    msg = "MultiArray of shape {}. The first and second dimensions correspond to sequence and batch size, respectively".format(
                        str(tuple(shape))
                    )
                    feature.shortDescription += msg

    optional_input_names = []
    for tup in graph.optional_inputs:
        optional_input_names.append(tup[0])
    optional_output_names = []
    for tup in graph.optional_outputs:
        optional_output_names.append(tup[0])

    # add description for inputs and outputs shapes
    remove_input_id = []
    for i, input_ in enumerate(builder.spec.description.input):
        if input_.name not in optional_input_names:
            if not disable_coreml_rank5_mapping:
                _add_informative_description(input_)
        else:
            remove_input_id.append(i)
    remove_output_id = []
    for i, output_ in enumerate(builder.spec.description.output):
        if output_.name not in optional_output_names:
            if not disable_coreml_rank5_mapping:
                _add_informative_description(output_, raise_error=False)
        else:
            remove_output_id.append(i)

    for index in sorted(remove_input_id, reverse=True):
        del builder.spec.description.input[index]
    for index in sorted(remove_output_id, reverse=True):
        del builder.spec.description.output[index]

    if len(graph.optional_inputs) > 0 or len(graph.optional_outputs):
        builder.add_optionals(graph.optional_inputs, graph.optional_outputs)

    # Check for specification version and target ios compatibility
    if (
        minimum_ios_deployment_target == "11.2"
        and builder.spec.WhichOneof("Type") == "neuralNetwork"
    ):
        nn_spec = builder.spec.neuralNetwork
        for layer in nn_spec.layers:
            if (
                layer.WhichOneof("layer") == "resizeBilinear"
                or layer.WhichOneof("layer") == "cropResize"
            ):
                raise TypeError(
                    "{} not supported with target iOS 11.2 please provide higher target iOS".format(
                        layer.WhichOneof("layer")
                    )
                )

    # Optimize ML Model Spec
    ml_model_passes = [remove_disconnected_layers, transform_conv_crop]
    for opt in ml_model_passes:
        opt(builder.spec)

    print("Translation to CoreML spec completed. Now compiling the CoreML model.")
    try:
        if DEBUG:
            import coremltools

            coremltools.models.utils.save_spec(
                builder.spec, "/tmp/node_model_raw_spec.mlmodel"
            )
            from coremltools.models.neural_network.printer import print_network_spec

            print_network_spec(builder.spec, style="coding")
        mlmodel = MLModel(builder.spec)
    except RuntimeError as e:
        raise ValueError("Compilation failed: {}".format(str(e)))
    print("Model Compilation done.")

    # print information about all ops for which custom layers have been added
    if len(err.custom_layer_nodes) > 0:
        print("\n")
        print(
            "Custom layers have been added to the CoreML model "
            "corresponding to the following ops in the onnx model: "
        )
        for i, node in enumerate(err.custom_layer_nodes):
            input_info = []
            for input_ in node.inputs:
                input_info.append(
                    (
                        str(input_),
                        graph.shape_dict.get(input_, str("Shape not available")),
                    )
                )
            output_info = []
            for output_ in node.outputs:
                output_info.append(
                    (
                        str(output_),
                        graph.shape_dict.get(output_, str("Shape not available")),
                    )
                )
            print(
                "{}/{}: op type: {}, op input names and shapes: {}, op output names and shapes: {}".format(
                    i + 1,
                    len(err.custom_layer_nodes),
                    node.op_type,
                    str(input_info),
                    str(output_info),
                )
            )

    mlmodel.user_defined_metadata[_METADATA_VERSION] = ct_version
    mlmodel.user_defined_metadata[_METADATA_SOURCE] = "onnx=={0}".format(
        onnx.__version__
    )
    return mlmodel
