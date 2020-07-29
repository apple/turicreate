from __future__ import print_function as _
from ...proto import NeuralNetwork_pb2 as _NeuralNetwork_pb2


def _get_weight_param_summary(wp):
    """Get a summary of _NeuralNetwork_pb2.WeightParams
    Args:
    wp : _NeuralNetwork_pb2.WeightParams - the _NeuralNetwork_pb2.WeightParams message to display
    Returns:
    a str summary for wp
    """
    summary_str = ""
    if wp.HasField("quantization"):
        nbits = wp.quantization.numberOfBits
        quant_type = (
            "linearly"
            if wp.quantization.HasField("linearQuantization")
            else "lookup-table"
        )
        summary_str += "{}-bit {} quantized".format(nbits, quant_type)

    if len(wp.floatValue) > 0:
        summary_str += "({} floatValues)".format(len(wp.floatValue))
    if len(wp.float16Value) > 0:
        summary_str += "({} bytes float16Values)".format(len(wp.float16Value))
    if len(wp.rawValue) > 0:
        summary_str += "({} bytes rawValues)".format(len(wp.rawValue))

    return summary_str


def _get_lstm_weight_param_summary(lstm_wp):
    weight_name_list = [
        "W_i",
        "W_f",
        "W_z",
        "W_o",
        "H_i",
        "H_f",
        "H_z",
        "H_o",
        "b_i",
        "b_f",
        "b_z",
        "b_o",
        "p_i",
        "p_f",
        "p_o",
    ]
    wp_summary_list = [
        _get_weight_param_summary(lstm_wp.inputGateWeightMatrix),
        _get_weight_param_summary(lstm_wp.forgetGateWeightMatrix),
        _get_weight_param_summary(lstm_wp.blockInputWeightMatrix),
        _get_weight_param_summary(lstm_wp.outputGateWeightMatrix),
        _get_weight_param_summary(lstm_wp.inputGateRecursionMatrix),
        _get_weight_param_summary(lstm_wp.forgetGateRecursionMatrix),
        _get_weight_param_summary(lstm_wp.blockInputRecursionMatrix),
        _get_weight_param_summary(lstm_wp.outputGateRecursionMatrix),
        _get_weight_param_summary(lstm_wp.inputGateBiasVector),
        _get_weight_param_summary(lstm_wp.forgetGateBiasVector),
        _get_weight_param_summary(lstm_wp.blockInputBiasVector),
        _get_weight_param_summary(lstm_wp.outputGateBiasVector),
        _get_weight_param_summary(lstm_wp.inputGatePeepholeVector),
        _get_weight_param_summary(lstm_wp.forgetGatePeepholeVector),
        _get_weight_param_summary(lstm_wp.outputGatePeepholeVector),
    ]
    lstm_wp_summary_list = []
    for idx, summary in enumerate(wp_summary_list):
        if len(summary) > 0:
            lstm_wp_summary_list.append(weight_name_list[idx] + ", " + summary)

    return ("\n" + " " * 8).join(lstm_wp_summary_list)


def _get_feature_description_summary(feature):
    if feature.type.HasField("multiArrayType"):
        shape = list(feature.type.multiArrayType.shape)
        int_shape = [int(x) for x in shape]
        return str(int_shape)
    else:
        return ("({})".format(str(feature.type))).replace("\n", "")


def _summarize_network_layer_info(layer):
    """
    Args:
    layer - an MLModel NeuralNetwork Layer protobuf message
    Returns:
    layer_type : str - type of layer
    layer_name : str - name of the layer
    layer_inputs : list[str] - a list of strings representing input blobs of the layer
    layer_outputs : list[str] - a list of strings representing output blobs of the layer
    layer_field_content : list[(str, str)] - a list of two-tuple of (parameter_name, content)
    """
    layer_type_str = layer.WhichOneof("layer")

    layer_name = layer.name
    layer_inputs = list(layer.input)
    layer_outputs = list(layer.output)

    typed_layer = getattr(layer, layer_type_str)
    layer_field_names = [l.name for l in typed_layer.DESCRIPTOR.fields]
    layer_field_content = []

    for name in layer_field_names:
        field = getattr(typed_layer, name)
        summary_str = ""
        if type(field) == _NeuralNetwork_pb2.LSTMWeightParams:
            summary_str = _get_lstm_weight_param_summary(field)
        elif type(field) == _NeuralNetwork_pb2.WeightParams:
            summary_str = _get_weight_param_summary(field)
        else:
            field_str = str(field)
            if len(field_str) > 0:
                summary_str = field_str.replace("\n", " ")
        if len(summary_str) > 0:
            layer_field_content.append([name, summary_str])

    return layer_type_str, layer_name, layer_inputs, layer_outputs, layer_field_content


def _summarize_neural_network_spec(mlmodel_spec):
    """ Summarize network into the following structure.
    Args:
    mlmodel_spec : mlmodel spec
    Returns:
    inputs : list[(str, str)] - a list of two tuple (name, descriptor) for each input blob.
    outputs : list[(str, str)] - a list of two tuple (name, descriptor) for each output blob
    layers : list[(str, list[str], list[str], list[(str, str)])] - a list of layers represented by
        layer name, input blobs, output blobs, a list of (parameter name, content)
    """
    inputs = [
        (blob.name, _get_feature_description_summary(blob))
        for blob in mlmodel_spec.description.input
    ]
    outputs = [
        (blob.name, _get_feature_description_summary(blob))
        for blob in mlmodel_spec.description.output
    ]
    nn = None

    if mlmodel_spec.HasField("neuralNetwork"):
        nn = mlmodel_spec.neuralNetwork
    elif mlmodel_spec.HasField("neuralNetworkClassifier"):
        nn = mlmodel_spec.neuralNetworkClassifier
    elif mlmodel_spec.HasField("neuralNetworkRegressor"):
        nn = mlmodel_spec.neuralNetworkRegressor

    layers = (
        [_summarize_network_layer_info(layer) for layer in nn.layers]
        if nn != None
        else None
    )
    return (inputs, outputs, layers)


def _prRed(skk, end=None):
    print("\033[91m {}\033[00m".format(skk), end=end)


def _prLightPurple(skk, end=None):
    print("\033[94m {}\033[00m".format(skk), end=end)


def _prPurple(skk, end=None):
    print("\033[95m {}\033[00m".format(skk), end=end)


def _prGreen(skk, end=None):
    print("\033[92m {}\033[00m".format(skk), end=end)


def _print_layer_type_and_arguments(
    layer_type_str, layer_inputs, indentation, to_indent=True, shape=None, value=None
):
    if to_indent:
        _prRed(indentation * "\t" + "{}".format(layer_type_str), end="")
    else:
        _prRed("{}".format(layer_type_str), end="")

    if shape is None:
        _prLightPurple("({})".format(", ".join(layer_inputs)))
    elif value is not None:
        _prLightPurple("(shape = ", end="")
        print("{}, ".format(str(shape)), end="")
        _prLightPurple("value = ", end="")
        values = ",".join(["{0: 0.1f}".format(v) for v in value]).lstrip()
        print("[{}]".format(values), end="")
        _prLightPurple(")")
    else:
        _prLightPurple("(shape = ", end="")
        print("{}".format(str(shape)), end="")
        _prLightPurple(")")


def _find_size(arr):
    s = 1
    for a in arr:
        s *= a
    return s


def _summarize_neural_network_spec_code_style(
    nn_spec, indentation=0, input_names=None, output_names=None
):
    """
    print nn_spec as if writing code
    """
    indentation_size = 1

    if input_names:
        print("def model({}):".format(", ".join(input_names)))
        indentation += indentation_size

    for i, layer in enumerate(nn_spec.layers):
        layer_type_str = layer.WhichOneof("layer")
        layer_inputs = list(layer.input)
        layer_outputs = list(layer.output)

        if layer_type_str == "loop":
            if len(layer.loop.conditionNetwork.layers) > 0:
                _prPurple(indentation * "\t" + "Condition Network: ")
                _summarize_neural_network_spec_code_style(
                    layer.loop.conditionNetwork, indentation=indentation
                )
            if layer.loop.conditionVar:
                layer_inputs.append(layer.loop.conditionVar)
            _print_layer_type_and_arguments(layer_type_str, layer_inputs, indentation)
            indentation += indentation_size
            _summarize_neural_network_spec_code_style(
                layer.loop.bodyNetwork, indentation=indentation
            )
            if len(layer.loop.conditionNetwork.layers) > 0:
                _prPurple(indentation * "\t" + "Condition Network: ")
                _summarize_neural_network_spec_code_style(
                    layer.loop.conditionNetwork, indentation=indentation
                )
            indentation -= indentation_size
            continue

        if layer_type_str == "branch":
            _print_layer_type_and_arguments(layer_type_str, layer_inputs, indentation)
            _prRed(indentation * "\t" + "IfBranch:")
            indentation += indentation_size
            _summarize_neural_network_spec_code_style(
                layer.branch.ifBranch, indentation=indentation
            )
            indentation -= indentation_size
            if len(layer.branch.elseBranch.layers) > 0:
                _prRed(indentation * "\t" + "ElseBranch:")
                indentation += indentation_size
                _summarize_neural_network_spec_code_style(
                    layer.branch.elseBranch, indentation=indentation
                )
                indentation -= indentation_size
            continue

        if layer_type_str == "loopBreak" or layer_type_str == "loopContinue":
            _prRed(indentation * "\t" + layer_type_str)
            continue

        shape = None
        value = None
        if layer_type_str == "loadConstant":
            shape = layer.loadConstant.shape
            shape = list(shape)
            int_shape = [int(x) for x in shape]
            shape = tuple([1, 1] + int_shape)
            size = _find_size(shape)
            if size < 4 and len(layer.loadConstant.data.floatValue) > 0:
                value = map(float, list(layer.loadConstant.data.floatValue))

        if layer_type_str == "loadConstantND":
            shape = layer.loadConstantND.shape
            shape = tuple(map(int, list(shape)))
            size = _find_size(shape)
            if size < 4 and len(layer.loadConstantND.data.floatValue) > 0:
                value = map(float, list(layer.loadConstantND.data.floatValue))

        print(indentation * "\t", end="")
        print("{} =".format(", ".join(layer_outputs)), end="")
        _print_layer_type_and_arguments(
            layer_type_str,
            layer_inputs,
            indentation,
            to_indent=False,
            shape=shape,
            value=value,
        )

    if output_names:
        _prRed("\n" + indentation * "\t" + "return ", end="")
        print("{}".format(", ".join(output_names)))
