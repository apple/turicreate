import math
import numpy as np
from coremltools.proto import FeatureTypes_pb2 as _FeatureTypes_pb2
from coremltools.proto import NeuralNetwork_pb2 as _NeuralNetwork_pb2


def _get_translator_function(layer_type):
    """Get the right translator function
    """
    if layer_type in _LAYER_REGISTERY:
        return _LAYER_REGISTERY[layer_type]
    else:
        raise TypeError(
            "Shape computation function missing for layer of type %s." % layer_type
        )


def _identity(layer, shape_dict):
    shape_dict[layer.output[0]] = shape_dict[layer.input[0]]


def _convolution(layer, shape_dict):
    params = layer.convolution
    Seq, Batch, Cin, Hin, Win = shape_dict[layer.input[0]]

    n_groups = params.nGroups
    Kh = Kw = 3
    hstride = wstride = hdilation = wdilation = 1
    if len(params.kernelSize) != 0:
        Kh, Kw = params.kernelSize
    if len(params.stride) != 0:
        hstride, wstride = params.stride
    if len(params.dilationFactor) != 0:
        hdilation, wdilation = params.dilationFactor
    Kh_dilated = (Kh - 1) * hdilation + 1
    Kw_dilated = (Kw - 1) * wdilation + 1
    l = r = b = t = 0
    if params.WhichOneof("ConvolutionPaddingType") == "valid":
        if len(params.valid.paddingAmounts.borderAmounts) != 0:
            t = params.valid.paddingAmounts.borderAmounts[0].startEdgeSize
            b = params.valid.paddingAmounts.borderAmounts[0].endEdgeSize
            l = params.valid.paddingAmounts.borderAmounts[1].startEdgeSize
            r = params.valid.paddingAmounts.borderAmounts[1].endEdgeSize
        if params.isDeconvolution:
            Hout = (Hin - 1) * hstride + Kh_dilated - t - b
            Wout = (Win - 1) * wstride + Kw_dilated - r - l
        else:
            Hout = (Hin + t + b - Kh_dilated) / hstride + 1
            Wout = (Win + r + l - Kw_dilated) / wstride + 1
    else:
        if params.isDeconvolution:
            Hout = Hin * hstride
            Wout = Win * wstride
        else:
            Hout = math.ceil(Hin / float(hstride))
            Wout = math.ceil(Win / float(wstride))

    if params.isDeconvolution:
        if len(params.outputShape) != 0:
            Hout, Wout = params.outputShape

    Cout = params.outputChannels
    shape_dict[layer.output[0]] = (Seq, Batch, int(Cout), int(Hout), int(Wout))


def _pooling(layer, shape_dict):
    params = layer.pooling
    Seq, Batch, Cin, Hin, Win = shape_dict[layer.input[0]]

    Kh = Kw = 3
    hstride = wstride = 1
    if len(params.kernelSize) != 0:
        Kh, Kw = params.kernelSize
    if len(params.stride) != 0:
        hstride, wstride = params.stride
    l = r = b = t = 0
    if params.globalPooling:
        Hout = Wout = 1
    else:
        if params.WhichOneof("PoolingPaddingType") == "valid":
            if len(params.valid.paddingAmounts.borderAmounts) != 0:
                t = params.valid.paddingAmounts.borderAmounts[0].startEdgeSize
                b = params.valid.paddingAmounts.borderAmounts[0].endEdgeSize
                l = params.valid.paddingAmounts.borderAmounts[1].startEdgeSize
                r = params.valid.paddingAmounts.borderAmounts[1].endEdgeSize
            Hout = (Hin + t + b - Kh) / hstride + 1
            Wout = (Win + r + l - Kw) / wstride + 1
        elif params.WhichOneof("PoolingPaddingType") == "same":
            Hout = math.ceil(Hin / float(hstride))
            Wout = math.ceil(Win / float(wstride))
        else:
            if len(params.includeLastPixel.paddingAmounts) != 0:
                t = params.includeLastPixel.paddingAmounts[0]
                b = t
                l = params.includeLastPixel.paddingAmounts[1]
                r = l
            Hout = math.ceil((Hin + 2 * t - Kh) / float(hstride)) + 1
            Wout = math.ceil((Win + 2 * l - Kw) / float(wstride)) + 1
            if t or l:
                if (Hout - 1) * hstride >= Hin + t:
                    Hout -= 1
                if (Wout - 1) * wstride >= Win + l:
                    Wout -= 1

    shape_dict[layer.output[0]] = (Seq, Batch, int(Cin), int(Hout), int(Wout))


def _inner_product(layer, shape_dict):
    params = layer.innerProduct
    Seq, Batch, Cin, Hin, Win = shape_dict[layer.input[0]]
    Cout = params.outputChannels
    shape_dict[layer.output[0]] = (Seq, Batch, int(Cout), 1, 1)


def _embedding(layer, shape_dict):
    params = layer.embedding
    Seq, Batch, Cin, Hin, Win = shape_dict[layer.input[0]]
    Cout = params.outputChannels
    shape_dict[layer.output[0]] = (Seq, Batch, int(Cout), 1, 1)


def _crop(layer, shape_dict):
    params = layer.crop
    Seq, Batch, Cin, Hin, Win = shape_dict[layer.input[0]]

    l = r = t = b = 0
    if len(layer.input) == 1:
        if len(params.cropAmounts.borderAmounts) != 0:
            t = params.cropAmounts.borderAmounts[0].startEdgeSize
            b = params.cropAmounts.borderAmounts[0].endEdgeSize
            l = params.cropAmounts.borderAmounts[1].startEdgeSize
            r = params.cropAmounts.borderAmounts[1].endEdgeSize
        Hout = Hin - t - b
        Wout = Win - l - r
    else:
        Hout = shape_dict[layer.input[1]][3]
        Wout = shape_dict[layer.input[1]][4]

    shape_dict[layer.output[0]] = (Seq, Batch, Cin, int(Hout), int(Wout))


def _padding(layer, shape_dict):
    params = layer.padding
    Seq, Batch, Cin, Hin, Win = shape_dict[layer.input[0]]

    l = r = t = b = 0
    if len(params.paddingAmounts.borderAmounts) != 0:
        t = params.paddingAmounts.borderAmounts[0].startEdgeSize
        b = params.paddingAmounts.borderAmounts[0].endEdgeSize
        l = params.paddingAmounts.borderAmounts[1].startEdgeSize
        r = params.paddingAmounts.borderAmounts[1].endEdgeSize
    Hout = Hin + t + b
    Wout = Win + l + r
    shape_dict[layer.output[0]] = (Seq, Batch, Cin, int(Hout), int(Wout))


def _upsample(layer, shape_dict):
    params = layer.upsample
    Seq, Batch, Cin, Hin, Win = shape_dict[layer.input[0]]

    sh = sw = 1
    if len(params.scalingFactor) != 0:
        sh, sw = params.scalingFactor
    Hout = Hin * sh
    Wout = Win * sw
    shape_dict[layer.output[0]] = (Seq, Batch, Cin, int(Hout), int(Wout))


def _add(layer, shape_dict):
    Seq, Batch, C, H, W = shape_dict[layer.input[0]]

    for i, inp in enumerate(layer.input):
        if i == 0:
            continue
        _, _, c, h, w = shape_dict[inp]
        C = max(C, c)
        H = max(H, h)
        W = max(W, w)
    shape_dict[layer.output[0]] = (Seq, Batch, int(C), int(H), int(W))


def _dot(layer, shape_dict):
    Seq, Batch, _, _, _ = shape_dict[layer.input[0]]
    shape_dict[layer.output[0]] = (Seq, Batch, 1, 1, 1)


def _reduce(layer, shape_dict):
    params = layer.reduce
    Seq, Batch, C, H, W = shape_dict[layer.input[0]]

    axis = _NeuralNetwork_pb2.ReduceLayerParams.ReduceAxis.Name(params.axis)
    if axis == "CHW":
        C = H = W = 1
    elif axis == "HW":
        H = W = 1
    elif axis == "C":
        C = 1
    elif axis == "H":
        H = 1
    elif axis == "W":
        W = 1

    shape_dict[layer.output[0]] = (Seq, Batch, int(C), int(H), int(W))


def _load_constant(layer, shape_dict):
    params = layer.loadConstant
    C, H, W = map(int, params.shape)
    shape_dict[layer.output[0]] = (1, 1, C, H, W)


def _reshape(layer, shape_dict):
    params = layer.reshape
    Seq, Batch, _, _, _ = shape_dict[layer.input[0]]

    if len(params.targetShape) == 3:
        C, H, W = params.targetShape
    else:
        Seq, C, H, W = params.targetShape

    shape_dict[layer.output[0]] = (int(Seq), Batch, int(C), int(H), int(W))


def _flatten(layer, shape_dict):
    params = layer.permute
    Seq, Batch, Cin, Hin, Win = shape_dict[layer.input[0]]

    shape_dict[layer.output[0]] = (int(Seq), int(Batch), int(Cin * Hin * Win), 1, 1)


def _permute(layer, shape_dict):
    params = layer.permute
    Seq, Batch, Cin, Hin, Win = shape_dict[layer.input[0]]

    axis = list(map(int, params.axis))
    dims = (Seq, Cin, Hin, Win)
    Seq_out = dims[axis[0]]
    Cout = dims[axis[1]]
    Hout = dims[axis[2]]
    Wout = dims[axis[3]]
    shape_dict[layer.output[0]] = (int(Seq_out), Batch, int(Cout), int(Hout), int(Wout))


def _concat(layer, shape_dict):
    params = layer.concat
    Seq, Batch, C, H, W = shape_dict[layer.input[0]]

    if params.sequenceConcat:
        Seq = 0
        for inp in layer.input:
            Seq += shape_dict[inp][0]
    else:
        C = 0
        for inp in layer.input:
            C += shape_dict[inp][2]

    shape_dict[layer.output[0]] = (int(Seq), Batch, int(C), int(H), int(W))


def _split(layer, shape_dict):
    input_shape = shape_dict[layer.input[0]]
    Seq, Batch, C, H, W = input_shape
    for out in layer.output:
        shape_dict[out] = (Seq, Batch, C / len(layer.output), H, W)


def _sequence_repeat(layer, shape_dict):
    params = layer.sequenceRepeat
    n = params.nRepetitions
    if n == 0:
        n = 1
    Seq, Batch, C, H, W = shape_dict[layer.input[0]]
    shape_dict[layer.output[0]] = (int(Seq * n), Batch, C, H, W)


def _reorganize_data(layer, shape_dict):
    params = layer.reorganizeData
    Seq, Batch, Cin, Hin, Win = shape_dict[layer.input[0]]
    block_size = params.blockSize
    Type = _NeuralNetwork_pb2.ReorganizeDataLayerParams.ReorganizationType.Name(
        params.mode
    )
    if Type == "SPACE_TO_DEPTH":
        Cout = Cin * block_size * block_size
        Hout = Hin / block_size
        Wout = Win / block_size
    else:
        Cout = Cin / (block_size * block_size)
        Hout = Hin * block_size
        Wout = Win * block_size
    shape_dict[layer.output[0]] = (Seq, Batch, int(Cout), int(Hout), int(Wout))


def _slice(layer, shape_dict):
    params = layer.slice
    Seq, Batch, C, H, W = shape_dict[layer.input[0]]
    start = params.startIndex
    end = params.endIndex
    stride = params.stride
    axis = _NeuralNetwork_pb2.SliceLayerParams.SliceAxis.Name(params.axis)
    if axis == "CHANNEL_AXIS":
        N = C
    if axis == "HEIGHT_AXIS":
        N = H
    if axis == "WIDTH_AXIS":
        N = W
    if end < 0:
        end = end + N
    end = min(end, N)
    if start > N - 1:
        L = 0
    else:
        L = np.floor((end - 1 - start) / stride) + 1
        if L < 0:
            L = 0
    if axis == "CHANNEL_AXIS":
        C = L
    if axis == "HEIGHT_AXIS":
        H = L
    if axis == "WIDTH_AXIS":
        W = L
    shape_dict[layer.output[0]] = (Seq, Batch, int(C), int(H), int(W))


def _simple_recurrent(layer, shape_dict):
    params = layer.simpleRecurrent
    Seq, Batch, C, H, W = shape_dict[layer.input[0]]

    Cout = params.outputVectorSize
    if params.sequenceOutput:
        shape_dict[layer.output[0]] = (Seq, Batch, int(Cout), 1, 1)
    else:
        shape_dict[layer.output[0]] = (1, Batch, int(Cout), 1, 1)
    shape_dict[layer.output[1]] = (1, Batch, int(Cout), 1, 1)


def _gru(layer, shape_dict):
    params = layer.gru
    Seq, Batch, C, H, W = shape_dict[layer.input[0]]

    Cout = params.outputVectorSize
    if params.sequenceOutput:
        shape_dict[layer.output[0]] = (Seq, Batch, int(Cout), 1, 1)
    else:
        shape_dict[layer.output[0]] = (1, Batch, int(Cout), 1, 1)
    shape_dict[layer.output[1]] = (1, Batch, int(Cout), 1, 1)


def _uni_directional_lstm(layer, shape_dict):
    params = layer.uniDirectionalLSTM
    Seq, Batch, C, H, W = shape_dict[layer.input[0]]

    Cout = params.outputVectorSize
    if params.params.sequenceOutput:
        shape_dict[layer.output[0]] = (Seq, Batch, int(Cout), 1, 1)
    else:
        shape_dict[layer.output[0]] = (1, Batch, int(Cout), 1, 1)
    shape_dict[layer.output[1]] = (1, Batch, int(Cout), 1, 1)
    shape_dict[layer.output[2]] = (1, Batch, int(Cout), 1, 1)


def _bi_directional_lstm(layer, shape_dict):
    params = layer.biDirectionalLSTM
    Seq, Batch, C, H, W = shape_dict[layer.input[0]]
    Cout = params.outputVectorSize
    if params.params.sequenceOutput:
        shape_dict[layer.output[0]] = (Seq, Batch, 2 * int(Cout), 1, 1)
    else:
        shape_dict[layer.output[0]] = (1, Batch, 2 * int(Cout), 1, 1)
    shape_dict[layer.output[1]] = (1, Batch, int(Cout), 1, 1)
    shape_dict[layer.output[2]] = (1, Batch, int(Cout), 1, 1)
    shape_dict[layer.output[3]] = (1, Batch, int(Cout), 1, 1)
    shape_dict[layer.output[4]] = (1, Batch, int(Cout), 1, 1)


_LAYER_REGISTERY = {
    "convolution": _convolution,
    "pooling": _pooling,
    "activation": _identity,
    "innerProduct": _inner_product,
    "embedding": _embedding,
    "batchnorm": _identity,
    "mvn": _identity,
    "l2normalize": _identity,
    "softmax": _identity,
    "lrn": _identity,
    "crop": _crop,
    "padding": _padding,
    "upsample": _upsample,
    "unary": _identity,
    "add": _add,
    "multiply": _add,
    "average": _add,
    "scale": _add,
    "bias": _add,
    "max": _add,
    "min": _add,
    "dot": _dot,
    "reduce": _reduce,
    "loadConstant": _load_constant,
    "reshape": _reshape,
    "flatten": _flatten,
    "permute": _permute,
    "concat": _concat,
    "split": _split,
    "sequenceRepeat": _sequence_repeat,
    "reorganizeData": _reorganize_data,
    "slice": _slice,
    "simpleRecurrent": _simple_recurrent,
    "gru": _gru,
    "uniDirectionalLSTM": _uni_directional_lstm,
    "biDirectionalLSTM": _bi_directional_lstm,
}


def infer_shapes(nn_spec, input_spec, input_shape_dict=None):
    """
    Input:

        spec : mlmodel spec
        input_shape_dict: dictionary of  string --> tuple
                      string:  input name
                      tuple: input shape as a 5 length tuple in order (Seq, Batch, C, H, W)

        If input_shape_dict is not provided, input shapes are inferred from the input description in the mlmodel.
        Since the description in the specification only contains values of C,H,W; Seq and Batch dimensions are set to 1.

    Output:

        shape_dict:  dictionary containing all the blobs in the neural network and their shapes, expressed as length 5 tuples,
                     to be interpreted in order (Seq, Batch, C, H, W).
    """

    shape_dict = {}
    if input_shape_dict:
        for key, value in input_shape_dict.items():
            assert len(value) == 5, "Shape of the input must be of length 5"
            shape_dict[key] = value

    # construct input_shape_dict from the model description
    else:
        for inp in input_spec:
            input_name = inp.name
            C = H = W = 1
            if inp.type.WhichOneof("Type") == "imageType":
                W = int(inp.type.imageType.width)
                H = int(inp.type.imageType.height)
                colorspace = _FeatureTypes_pb2.ImageFeatureType.ColorSpace.Name(
                    inp.type.imageType.colorSpace
                )
                if colorspace == "GRAYSCALE":
                    C = 1
                elif colorspace == "RGB" or colorspace == "BGR":
                    C = 3
                else:
                    raise ValueError("Input %s : Invalid Colorspace" % (input_name))
            elif inp.type.WhichOneof("Type") == "multiArrayType":
                array_shape = inp.type.multiArrayType.shape
                if len(array_shape) == 1:
                    C = array_shape[0]
                elif len(array_shape) == 3:
                    C, H, W = map(int, array_shape)
                else:
                    raise ValueError(
                        "Input %s : Multi array must be of length 1 or 3" % (input_name)
                    )
            else:
                raise ValueError(
                    "Input %s : Input type must be image or multi-array" % (input_name)
                )
            shape_dict[input_name] = (1, 1, C, H, W)

    layers = nn_spec.layers

    for i, layer in enumerate(layers):
        for inp in layer.input:
            assert inp in shape_dict, "Input %s shape not cannot be determined" % (inp)
        layer_type = layer.WhichOneof("layer")
        if layer_type == "custom":
            break
        layer_translator = _get_translator_function(layer_type)
        layer_translator(layer, shape_dict)

    return shape_dict
