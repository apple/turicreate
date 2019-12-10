"""
Shape inference functions.
"""


def _transpose(layer_spec, input_shapes):
    axes = list(layer_spec.transpose.axes)
    input_shape = input_shapes[0]
    output_shape = [None] * len(input_shape)

    for j in range(len(input_shape)):
        output_shape[j] = input_shape[axes[j]]

    return [output_shape]


def _get_shape(layer_spec, input_shapes):
    rank = len(input_shapes[0])
    return [[rank]]


def _slice_static(layer_spec, input_shapes):
    params = layer_spec.sliceStatic
    input_shape = input_shapes[0]
    rank = len(input_shape)
    output_shape = [-1] * rank
    begin_indices = params.beginIds
    end_indices = params.endIds
    begin_masks = params.beginMasks
    end_masks = params.endMasks

    for i in range(rank):
        begin_indices[i] = begin_indices[i] if begin_indices[i] >= 0 else input_shape[i] + begin_indices[i]
        end_indices[i] = end_indices[i] if end_indices[i] >= 0 else input_shape[i] + end_indices[i]
    for idx, dim in enumerate(input_shape):
        if dim > 0:  # known
            begin = None if params.beginMasks[idx] else begin_indices[idx]
            end = None if params.endMasks[idx] else end_indices[idx]
            thisslice = slice(begin, end, params.strides[idx])
            thisslicelen = len(list(range(input_shape[idx]))[thisslice])
            output_shape[idx] = thisslicelen

    return [output_shape]


def _slice_dynamic(layer_spec, input_shapes):
    input_shape = input_shapes[0]
    rank = len(input_shape)
    output_shape = [-1] * rank
    return [output_shape]


def _squeeze(layer_spec, input_shapes):
    if layer_spec.squeeze.squeezeAll:
        return [[1]]
    axes = list(layer_spec.squeeze.axes)
    input_shape = input_shapes[0]
    rank = len(input_shape)

    if axes is None or len(axes) == 0:
        raise NotImplementedError('Unspecified axes not implemented.')
    output_shape = []
    axes = [axis if axis >= 0 else rank + axis for axis in axes]
    for dim in range(rank):
        if dim not in axes:
            output_shape.append(input_shape[dim])
        elif input_shape[dim] > 0 and input_shape[dim] != 1:
            raise ValueError(
                '[Shaper] Cannot squeeze on index %d of shape %s' % (dim, str(input_shape)))
    return [output_shape] if output_shape else [[1]]


def _range_dynamic(layer_spec, input_shapes):
    if len(input_shapes) == 3:
        return [[-1]]  # 1 output containing an unknown length of vector
    else:
        raise NotImplementedError('NNSSA converter can only handle 3-input dynamic range at this time.')


def _range_static(layer_spec, input_shapes):
    if len(input_shapes) == 3:
        return [[-1]]
    else:
        params = layer_spec.rangeStatic
        start, end, step = params.startValue, params.endValue, params.stepSizeValue
        return [[int((end - start) / step)]]


def _load_constant(layer_spec, input_shapes):
    shape = list(layer_spec.loadConstant.shape)
    return [shape]


def _load_constant_nd(layer_spec, input_shapes):
    shape = list(layer_spec.loadConstantND.shape)
    return [shape]


def _add(layer_spec, input_shapes):
    if len(input_shapes) == 2:
        r = max(len(input_shapes[0]), len(input_shapes[1]))
        # broadcasting if necessary
        output_shapes = [[1] * (r - len(s)) + s for s in input_shapes]
    elif len(input_shapes) == 1:
        output_shapes = input_shapes
    else:
        raise ValueError("[Shaper] Expects _add layers having either 1 or 2 inputs")
    return output_shapes


def _broadcastable(layer_spec, input_shapes):
    def broadcast_dim(x, y):
        if x < 0 or y < 0:
            return -1
        if x == 1 or y == 1:
            return max([x, y])
        elif x == y:
            return x
        else:
            return None

    max_rank = max([len(s) for s in input_shapes])
    extended_input_shapes = [[1] * (max_rank - len(s)) + list(s) for s in input_shapes]
    output_shape = [1] * max_rank
    for i_dim in range(max_rank):
        for s in extended_input_shapes:
            output_shape[i_dim] = broadcast_dim(output_shape[i_dim], s[i_dim])
            if output_shape[i_dim] is None:
                raise ValueError('[Shaper] Cannot broadcast input_shapes %s' % (str(input_shapes)))
    return [output_shape]


def _scatter(layer_spec, input_shapes):
    # inputs: [target, source, indices]
    return [input_shapes[0]]


def _scatter_nd(layer_spec, input_shapes):
    # get the values of the shape input
    return [input_shapes[0]]


def _gather(layer_spec, input_shapes):
    if len(input_shapes) == 2:
        indices_shape = input_shapes[1]
        return [list(indices_shape) + list(input_shapes[0][1:])]
    else:
        raise ValueError("[Shaper] Gather layer accepts only 2 inputs")


def _gather_nd(layer_spec, input_shapes):
    param_shape = input_shapes[0]
    index_shape = input_shapes[1]
    index_rank = len(index_shape)
    output_shape = index_shape[:-1]
    output_shape[index_rank - 1:] = param_shape[index_shape[index_rank - 1]:]
    return [output_shape]


def _concat_nd(layer_spec, input_shapes):
    if layer_spec.WhichOneof('layer') == 'concat':
        axis = -3
    else:
        axis = layer_spec.concatND.axis
    rank = len(input_shapes[0])
    output_shape = list(input_shapes[0][:])
    if axis < 0:
        axis += rank

    for shape in input_shapes:
        if len(shape) != rank:
            raise ValueError('[Shaper] Unable to shape concatND: ranks mismatch')

    for shape in input_shapes[1:]:
        for idx, dim in enumerate(shape):
            if output_shape[idx] == -1 or dim == -1:
                output_shape[idx] = -1
                continue
            if idx == axis:
                output_shape[idx] += dim
            elif output_shape[idx] != dim:
                raise ValueError('[Shaper] Unable to shape concatND: shapes mismatch')
    return [output_shape]


def _inner_product(layer_spec, input_shapes):
    if len(input_shapes) == 1:  # static weight
        input_shape = input_shapes[0]
        in_channels = layer_spec.innerProduct.inputChannels
        out_channels = layer_spec.innerProduct.outputChannels
        if input_shape[-1] != in_channels:
            raise ValueError('[Shaper] Inner Product layer input channels mismatch')
        return [input_shape[0:-1] + [out_channels]]
    elif len(input_shapes) == 2:
        input_shape, mat_shape = input_shapes[0:2]
        in_channels = input_shape[-1]
        if in_channels != -1 and in_channels != mat_shape[-2]:
            raise ValueError('[Shaper] Inner Product layer input channels mismatch')
        out_channels = mat_shape[-1]
        return [input_shape[0:-1] + [out_channels]]
    else:
        raise ValueError('[Shaper] Inner Product needs either 1 or 2 inputs')


def _split_nd(layer_spec, input_shapes):
    if len(input_shapes) != 1:
        raise NotImplementedError('[Shaper] Dynamic split not implemented.')
    axis = layer_spec.splitND.axis
    num_splits = layer_spec.splitND.numSplits
    output_shape = input_shapes[0][:]
    output_shape[axis] /= num_splits
    if output_shape[axis] == 0:
        raise ValueError('[Shaper] Cannot split shape %s on axis %d' % (str(output_shape), axis))
    return [output_shape] * num_splits


def _identity(layer_spec, input_shapes):
    return input_shapes[:]


def _reverse_seq(layer_spec, input_shapes):
    return [input_shapes[0]]


def _expand_dims(layer_spec, input_shapes):
    input_shape = input_shapes[0]
    axes = list(layer_spec.expandDims.axes)
    target_rank = len(input_shape) + len(axes)
    axes = [axis if axis >= 0 else axis + target_rank for axis in axes]

    output_shape = input_shape[:]
    for axis in axes:
        output_shape = list(output_shape[0:axis]) + [1] + list(output_shape[axis:])
    return [output_shape]


def _where_non_zero(layer_spec, input_shapes):
    input_shape = input_shapes[0]
    rank = len(input_shape)
    output_shape = [-1, rank]
    return [output_shape]


def _stack(layer_spec, input_shapes):
    axis = layer_spec.stack.axis
    num_inputs = len(layer_spec.input)
    shape = input_shapes[0]
    for s in input_shapes:
        if s != shape:
            raise ValueError('[Shaper] stack input shapes mismatch')
    output_shape = shape[:axis] + [num_inputs] + shape[axis:]
    return [output_shape]


def _batched_mat_mul(layer_spec, input_shapes):
    if len(input_shapes) == 1:
        a_shape = list(input_shapes[0][:])
        a_shape[-1] = int(layer_spec.batchedMatmul.weightMatrixSecondDimension)
        return [a_shape]
    elif len(input_shapes) == 2:
        a_shape, b_shape = input_shapes
        if len(a_shape) < 2 or len(b_shape) < 2:
            raise ValueError('[Shaper] MatMul with 2 inputs require the ranks of both inputs to be no less than 2')
        tp_a = layer_spec.batchedMatmul.transposeA
        tp_b = layer_spec.batchedMatmul.transposeB
        r_x, c_x = a_shape[-2:]
        r_y, c_y = b_shape[-2:]
        r_o = c_x if tp_a else r_x
        c_o = r_y if tp_b else c_y
        output_shape = list(a_shape[0:-2]) + [r_o, c_o]
        return [output_shape]
    else:
        raise NotImplementedError('[Shaper] Batched MatMul requires either 1 or 2 inputs')


def _embedding_nd(layer_spec, input_shapes):
    input_shape = input_shapes[0]
    if input_shape[-1] != 1:
        raise ValueError('[Shaper] Last dimension of EmbeddingND input must be 1')
    vocab_size = layer_spec.embeddingND.vocabSize
    embedding_size = int(layer_spec.embeddingND.embeddingSize)
    output_shape = input_shapes[0][:]
    output_shape[-1] = embedding_size
    return [output_shape]


def _conv2d(layer_spec, input_shapes):
    raise NotImplementedError('Conv2D: shape logic not implemented')


def _reshape_static(layer_spec, input_shapes):
    target_shape = list(layer_spec.reshapeStatic.targetShape)
    return [target_shape] if target_shape else [[1]]


def _reduce(layer_spec, input_shapes):
    axis_param = layer_spec.reduce.axis
    axis = None
    if axis_param == 2:
        axis = -3
    elif axis_param == 3:
        axis = -2
    elif axis_param == 4:
        axis = -1
    else:
        raise NotImplementedError(
            '[Shaper] Reduce with axis parameter %s is not implemented.' % (str(axis_param)))
    output_shape = input_shapes[0][:]
    output_shape[axis] = 1
    return [output_shape]


def _reduce_general(params, input_shapes):
    if params.reduceAll:
        return [[1]]

    axes = list(params.axes)
    output_shape = list(input_shapes[0][:])
    if params.keepDims:
        for axis in axes:
            output_shape[axis] = 1
    else:
        for axis in axes:
            output_shape[axis] = None
        output_shape = [dim for dim in output_shape if dim is not None]

    return [output_shape] if output_shape else [[1]]


def _reduce_logsumexp(layer_spec, input_shapes):
    return _reduce_general(layer_spec.reduceLogSumExp, input_shapes)

def _reduce_prod(layer_spec, input_shapes):
    return _reduce_general(layer_spec.reduceProd, input_shapes)


def _reduce_mean(layer_spec, input_shapes):
    return _reduce_general(layer_spec.reduceMean, input_shapes)


def _reduce_sum(layer_spec, input_shapes):
    return _reduce_general(layer_spec.reduceSum, input_shapes)


def _reduce_max(layer_spec, input_shapes):
    return _reduce_general(layer_spec.reduceMax, input_shapes)


def _reduce_min(layer_spec, input_shapes):
    return _reduce_general(layer_spec.reduceMin, input_shapes)


def _argmax(layer_spec, input_shapes):
    params = layer_spec.argMax
    axis = params.axis
    keepdims = not params.removeDim

    output_shape = input_shapes[0][:]
    if keepdims:
        output_shape[axis] = 1
    else:
        output_shape[axis] = None
        output_shape = [dim for dim in output_shape if dim is not None]

    return [output_shape] if output_shape else [[1]]


def _argmin(layer_spec, input_shapes):
    params = layer_spec.argMin
    axis = params.axis
    keepdims = not params.removeDim

    output_shape = input_shapes[0][:]
    if keepdims:
        output_shape[axis] = 1
    else:
        output_shape[axis] = None
        output_shape = [dim for dim in output_shape if dim is not None]

    return [output_shape] if output_shape else [[1]]


def _tile(layer_spec, input_shapes):
    params = layer_spec.tile
    reps = params.reps
    assert len(reps) == len(input_shapes[0])
    return [[reps[i] * input_shapes[0][i] for i in range(len(reps))]]


def _fill_static(layer_spec, input_shapes):
    params = layer_spec.fillStatic
    output_shape = params.targetShape
    return [output_shape]


def _fill_dynamic(layer_spec, input_shapes):
    assert (len(input_shapes) == 1 and len(input_shapes[0]) == 1)
    rank = int(input_shapes[0][0])
    return [[-1] * rank]


def _broadcast_to_like(layer_spec, input_shapes):
    return [input_shapes[1]]


def _broadcast_to_static(layer_spec, input_shapes):
    params = layer_spec.broadcastToStatic
    output_shape = params.targetShape
    return [output_shape]


def _pad(layer_spec, input_shapes):
    return [[-1] * len(input_shapes[0])]


def _mirror_pad(layer_spec, input_shapes):
    params = layer_spec.padding
    pad_h = params.paddingAmounts.borderAmounts[0]
    pad_w = params.paddingAmounts.borderAmounts[1]
    output_shape = input_shapes[0]
    output_shape[2] += pad_h.startEdgeSize + pad_h.endEdgeSize
    output_shape[3] += pad_w.startEdgeSize + pad_w.endEdgeSize
    return [output_shape]


def _crop(layer_spec, input_shapes):
    return [[-1] * len(input_shapes[0])]


def _topk(layer_spec, input_shapes):
    params = layer_spec.topK
    value_shape = index_shape = input_shapes[0][:-1] + [params.K]
    output_shapes = [value_shape, index_shape]
    return output_shapes


def _unidirectional_lstm(layer_spec, input_shapes):
    shape = input_shapes[0]
    hidden_size = input_shapes[1][2]
    shape[2] = hidden_size
    return [shape] * 3


def _reorganize_data(layer_spec, input_shapes):
    block_size = layer_spec.reorganizeData.blockSize
    output_shape = input_shapes[0][:]
    if 'SpaceToDepth' in layer_spec.name or 'SpaceToBatchND' in layer_spec.name:
        output_shape[2] //= block_size
        output_shape[3] //= block_size
        output_shape[1] = output_shape[1] * block_size * block_size
    elif 'DepthToSpace' in layer_spec.name or 'BatchToSpaceND' in layer_spec.name:
        output_shape[2] *= block_size
        output_shape[3] *= block_size
        output_shape[1] = output_shape[1] // (block_size * block_size)
    return [output_shape]


# We'll enable them one by one
_LAYER_REGISTRY = {
    'transpose': _transpose,
    'getShape': _get_shape,
    'sliceStatic': _slice_static,
    'sliceDynamic': _slice_dynamic,
    'squeeze': _squeeze,
    'rangeStatic': _range_static,
    'rangeDynamic': _range_dynamic,
    'loadConstant': _load_constant,
    'loadConstantND': _load_constant_nd,
    'gather': _gather,
    'gatherND': _gather_nd,
    'scatter': _scatter,
    'scatterND': _scatter_nd,
    'logicalOr': _broadcastable,
    'logicalNot': _identity,
    'lessThan': _broadcastable,
    'lessEqual': _broadcastable,
    'greaterThan': _broadcastable,
    'greaterEqual': _broadcastable,
    'equal': _broadcastable,
    'notEqual': _broadcastable,
    'logicalAnd': _broadcastable,
    'add': _add,
    'multiply': _add,
    'concatND': _concat_nd,
    'concat': _concat_nd,
    'innerProduct': _inner_product,
    'activation': _identity,
    'reverse': _identity,
    'reverseSeq': _reverse_seq,
    'copy': _identity,
    'expandDims': _expand_dims,
    'stack': _stack,
    'whereNonZero': _where_non_zero,
    'addBroadcastable': _broadcastable,
    'subtractBroadcastable': _broadcastable,
    'divideBroadcastable': _broadcastable,
    'whereBroadcastable': _broadcastable,
    'maxBroadcastable': _broadcastable,
    'minBroadcastable': _broadcastable,
    'modBroadcastable': _broadcastable,
    'floorDivBroadcastable': _broadcastable,
    'powBroadcastable': _broadcastable,
    'conv2d': _conv2d,
    'multiplyBroadcastable': _broadcastable,
    'reshapeStatic': _reshape_static,
    # 'convolution': _convolution, # We propagate convolutional shapes by directly assigning from SSA output shape
    'embeddingND': _embedding_nd,
    'softmax': _identity,
    'softmaxND': _identity,
    'unary': _identity,
    'bias': _add,
    'max': _add,
    'min': _add,
    'reduce': _reduce,
    'argMax': _argmax,
    'argMin': _argmin,
    'reduceLogSumExp': _reduce_logsumexp,
    'reduceProd': _reduce_prod,
    'reduceMean': _reduce_mean,
    'reduceSum': _reduce_sum,
    'reduceMax': _reduce_max,
    'reduceMin': _reduce_min,
    'splitND': _split_nd,
    'batchedMatmul': _batched_mat_mul,
    'sin': _identity,
    'cos': _identity,
    'tan': _identity,
    'tile': _tile,
    'fillLike': _identity,
    'fillStatic': _fill_static,
    'fillDynamic': _fill_dynamic,
    'uniDirectionalLSTM': _unidirectional_lstm,
    'broadcastToLike': _broadcast_to_like,
    'broadcastToStatic': _broadcast_to_static,
    'constantPad': _pad,
    'padding': _mirror_pad,
    'crop': _crop,
    'sign': _identity,
    'ceil': _identity,
    'floor': _identity,
    'round': _identity,
    'topK': _topk,
    'reorganizeData': _reorganize_data,
    'batchnorm': _identity,
    'clip': _identity,
    'lrn': _identity,
}


def _get_translator_function(layer_type):
    """Get the right translator function
    """
    if layer_type in _LAYER_REGISTRY:
        return _LAYER_REGISTRY[layer_type]
    else:
        raise TypeError(
            "Shape computation function missing for layer of type %s." % type(layer_type))


def _insert_to_dict(dic, key, val):
    """ Insert key to dic, where dic[key] value is a list of unique elements
    """
    if key not in dic:
        dic[key] = []
    if val not in dic[key]:
        dic[key].append(val)


def get_common_shape(x, y):
    """ Get common shape z from two shapes, x and y.
    If x and y are of different ranks, error out.
    If x and y have the same rank, but x[i] != y[i] for some i, then z[i] = -1, indicating UNKNOWN.
    If x and y are equal, z = x
    """
    z = None
    if len(x) == len(y):
        z = list(x)
        for idx in range(len(x)):
            z[idx] = x[idx] if x[idx] == y[idx] else -1
    return z


def is_static_shape(shape):
    return not (False in [x > 0 for x in shape])


def is_a_shape_of(x, y):
    """
    True if x is a shape of y.
    y uses -1 to indicate arbitrary number.
    If y is None, then it represent a "missing" shape. In this case it will return True.
    """
    if y is None:
        return True
    x = (1,) if len(x) == 0 else x # Scalar should be interpreted as an 1-element array
    y = (1,) if len(y) == 0 else y # Scalar should be interpreted as an 1-element array
    if len(x) != len(y):
        return False
    return all([(a[0] == a[1] or a[1] == -1) for a in zip(x, y)])


def _propagate_shapes(nn_spec, blob_names, shapes, srcs, dsts, layer_specs):
    """
    Traverse the neural network spec. The spec may not be top level.
    This should be used as the internal recursive call. Use traverse() to do the top level traversal.
    blob_names - a list of blob names
    shapes - a dictionary of {blob_name: shape}
    srcs - a dictionary of {blob_name: layers_writing_to_it}
    dsts - a dictionary of {blob_name: layers_reading_from_it}
    layer_specs - a dictionary of {layer_name: layer_spec} for easy access to parameters.

    srcs, dsts, and layer_specs are byproducts that are not necessary for propagating the shapes.
    I made these for debugging purposes.
    """
    layers = nn_spec.layers
    for i, layer in enumerate(layers):
        # Register layer
        layer_name = layer.name
        layer_specs[layer_name] = layer
        # Register input blobs
        for j, blob_name in enumerate(layer.input):
            if blob_name not in blob_names:
                raise ValueError(
                    '[Shaper] Layer %s input[%d] (%s) has never been seen before.' %
                    (layer_name, j, blob_name))
            if blob_name not in shapes:
                raise ValueError(
                    '[Shaper] The shape of input[%d] (%s) needed for layer "%s" cannot be determined.'
                    % (j, blob_name, layer_name))
            # Mark the layer as the destination of blob
            _insert_to_dict(dsts, blob_name, layer_name)

        layer_type = layer.WhichOneof('layer')
        if layer_type not in _LAYER_REGISTRY:
            raise NotImplementedError(
                '[Shaper] Layer "{}" of type "{}" not implemented'.format(layer_name, layer_type))
        if layer_type == 'forloop':
            # If a nested network, recursively traverse into it
            _propagate_shapes(layer.condition)
            _propagate_shapes(layer.bodyNetwork)
        elif layer_type == 'branch':
            _propagate_shapes(layer.ifBranch)
            _propagate_shapes(layer.elseBranch)
        else:
            # If a regular layer, compute output blob shapes.
            layer_translator = _get_translator_function(layer_type)
            input_shapes = [shapes[b] for b in layer.input]
            output_shapes = layer_translator(layer, input_shapes)

        # Register output blobs
        for k, blob_name in enumerate(layer.output):
            if blob_name not in blob_names:
                blob_names.append(blob_name)
            _insert_to_dict(srcs, blob_name, layer_name)
            if blob_name not in shapes:
                shapes[blob_name] = output_shapes[k]
            else:
                common_shape = get_common_shape(shapes[blob_name], output_shapes[k])
                if common_shape is None:
                    raise ValueError(
                        'Unable to resolve shape for blob %s, with potential shape %s and %s' %
                        (blob_name, str(shapes[blob_name]), str(output_shapes[k])))


def _finalize_spec(nn_spec, shapes, overwrite=True):
    """
    This is the internal recursive call. Use propagate_shapes() to do the top level traversal.
    nn_spec: spec for the neural network
    shapes: a {str : shape} dictionary tracking the name -> coreml_shape pair
    overwrite: If True, will discard existing tensor shapes in the spec.
               If False, will check for tensor shape existence, write it if spec does not have tensor field,
               otherwise will check for consistency.
    """
    layers = nn_spec.layers
    for i, layer in enumerate(layers):
        layer_type = layer.WhichOneof('layer')

        if overwrite:
            del layer.inputTensor[:]
            del layer.outputTensor[:]

        # input
        if len(layer.inputTensor) == 0:
            for j, blob_name in enumerate(layer.input):
                shape = shapes[blob_name]
                ts = layer.inputTensor.add()
                ts.rank = len(shape)
                ts.dimValue.extend(list(shape))
        else:  # This does the check
            for j, blob_name in enumerate(layer.input):
                shape = shapes[blob_name]
                ts = layer.inputTensor[j]
                existing_shape = list(ts.dimValue)
                if not (is_a_shape_of(existing_shape, shape)
                        or is_a_shape_of(shape, existing_shape)):
                    raise ValueError(
                        '[Shaper] For layer %s, Existing shape %s does not match new shape %s' %
                        (layer.name, str(existing_shape), str(shape)))

        # output
        if len(layer.outputTensor) == 0:
            for j, blob_name in enumerate(layer.output):
                shape = shapes[blob_name]
                ts = layer.outputTensor.add()
                ts.rank = len(shape)
                ts.dimValue.extend(list(shape))
        else:  # This does the check
            for j, blob_name in enumerate(layer.output):
                shape = shapes[blob_name]
                ts = layer.outputTensor[j]
                existing_shape = list(ts.dimValue)
                if not (is_a_shape_of(existing_shape, shape)
                        or is_a_shape_of(shape, existing_shape)):
                    raise ValueError(
                        '[Shaper] For layer %s, Existing shape %s does not match new shape %s' %
                        (layer.name, str(existing_shape), str(shape)))

        # If a nested network, recursively traverse into it
        if layer_type == 'forloop':
            _finalize_spec(layer.condition)
            _finalize_spec(layer.bodyNetwork)
        elif layer_type == 'branch':
            _finalize_spec(layer.ifBranch)
            _finalize_spec(layer.elseBranch)
        else:
            pass


def propagate_shapes(mlmodel_spec, overwrite=True):
    """
    Propagate input shapes in the spec into every layer
    This changes the mlmodel_spec!!
    mlmodel_spec - the MLModel spec with the model descriptions
    overwrite - if True, will overwrite existing tensor shapes
    """
    blob_names = []
    srcs = {}
    dsts = {}
    shapes = {}
    layer_specs = {}

    # put the inputs into Shaper
    for feature in mlmodel_spec.description.input:
        name = feature.name
        blob_names.append(name)
        srcs[name] = []
        shapes[name] = list(feature.type.multiArrayType.shape)

    top_nn_spec = mlmodel_spec.neuralNetwork
    _propagate_shapes(top_nn_spec, blob_names, shapes, srcs, dsts, layer_specs)
    _finalize_spec(top_nn_spec, shapes, overwrite=overwrite)

    output_names = [output.name for output in mlmodel_spec.description.output]

    if overwrite:
        del mlmodel_spec.description.output[:]

    if len(mlmodel_spec.description.output) == 0:
        for name in output_names:
            output_ = mlmodel_spec.description.output.add()
            output_.name = name
            shape = shapes[name]
            for n in shape:
                output_.type.multiArrayType.shape.append(n)
    else:
        for output_ in mlmodel_spec.description.output:
            existing_shape = list(output_.type.multiArrayType.shape)
            shape = shapes[output_.name]

            if not (is_a_shape_of(existing_shape, shape) or is_a_shape_of(shape, existing_shape)):
                raise ValueError(
                    '[Shaper] For layer %s, Existing shape %s does not match new shape %s' %
                    (layer.name, str(existing_shape), str(shape)))


def propagate_single_layer(layer, shapes, output_shapes=None, custom_shape_function=None):
    """
    Propagate input shape to output shape for a single layer, which could have nested networks
    layer : a layer spec
    shapes : a dictionary that stores all known shapes
    output_shapes : if None, the output tensors' shapes are computed by its shape propagation function,
        defined by _get_translator_function(layer_type). If not None, will force output_shapes to be
        written as the output spec of the layer.
    custom_shape_function : if None, shape function from _LAYER_REGISTRY will be used to infer shape,
        If not None, provided function will be used to compute output shape.
    """
    for j, blob_name in enumerate(layer.input):
        if blob_name not in shapes:
            raise ValueError(
                '[Shaper] The shape of input[%d] (%s) needed for layer "%s" cannot be determined.' %
                (j, blob_name, layer.name))

    layer_type = layer.WhichOneof('layer')
    if output_shapes is None:
        if layer_type not in _LAYER_REGISTRY and custom_shape_function is None:
            raise NotImplementedError(
                '[Shaper] Layer "{}" of type "{}" not implemented'.format(layer.name, layer_type))
        layer_translator = _get_translator_function(layer_type) if layer_type in _LAYER_REGISTRY else custom_shape_function
        input_shapes = [list(shapes[b]) for b in layer.input]
        output_shapes = layer_translator(layer, input_shapes)

    # Register output blobs
    for k, blob_name in enumerate(layer.output):
        if blob_name not in shapes:
            shapes[blob_name] = output_shapes[k]
        else:
            common_shape = get_common_shape(shapes[blob_name], output_shapes[k])
            if common_shape is None:
                raise ValueError(
                    'Unable to resolve shape for blob %s, with potential shape %s and %s' %
                    (blob_name, str(shapes[blob_name]), str(output_shapes[k])))

    # Write into layer spec
    del (layer.inputTensor[:])
    for j, blob_name in enumerate(layer.input):
        shape = shapes[blob_name]
        ts = layer.inputTensor.add()
        ts.rank = len(shape)
        ts.dimValue.extend(list(map(int, shape)))

    del (layer.outputTensor[:])
    for j, blob_name in enumerate(layer.output):
        shape = shapes[blob_name]
        ts = layer.outputTensor.add()
        ts.rank = len(shape)
        ts.dimValue.extend(list(map(int, shape)))
