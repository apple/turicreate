import numpy as np

from six import string_types as _string_types

from coremltools.models import datatypes
from coremltools.proto import NeuralNetwork_pb2, Model_pb2
from coremltools.models.neural_network import NeuralNetworkBuilder
from coremltools.models.neural_network.flexible_shape_utils import set_multiarray_ndshape_range
from collections import Iterable
import coremltools

from ..commons import builtins
from ..commons.basic_graph_ops import topsort, check_connections

from .graph_pass import *

try:
    import shapes
except:
    from . import shapes

DEBUG = False


def _is_scalar(type_):
    if type_ is None:
        return False
    result = builtins.is_int(type_) or builtins.is_float(type_) or builtins.is_bool(type_)
    if builtins.is_tensor(type_) and (len(type_.get_shape()) == 0):
        result = True
    return result


def ssa_convert(ssa,
                top_func='main',
                inputs=None,
                outputs=None,
                image_input_names=None,
                is_bgr=False,
                red_bias=0.0,
                green_bias=0.0,
                blue_bias=0.0,
                gray_bias=0.0,
                image_scale=1.0,
                class_labels=None,
                predicted_feature_name=None,
                predicted_probabilities_output='',
                add_custom_layers=False,
                custom_conversion_functions={},
                custom_shape_functions={}, 
                optional_inputs = []
                ):

    """
    Convert NNSSA into Core ML spec.
    ssa : NetworkEnsemble
        Required parameter
        NNSSA to be converted to CoreML spec.
    top_func : str or 'main'
        Function entry point
    inputs : dict of str -> list/tuple or None
        Input features of CoreML specs. Must be a dictionary with
        name as key and shape as value {name: shape},
        where name is the input's name, shape is the
        shape of the feature tensor. The shape must be static - all
        dimensions of shape should be a positive integer.
        When not provided, SSA converter will treat all input nodes
        in top level NNSSA as inputs.
    outputs : list of str or None
        Output features of CoreML specs. Must be a list of [name].
        When not provided, SSA converter will treat all output nodes
        in top level NNSSA as outputs.
    add_custom_layers : bool or False
        If True, then `custom` layers will be added to the model in place
        for unsupported ops.
        Parameters for these custom layers should be filled manually by editing the mlmodel
        or the 'custom_conversion_functions' argument can be used to do the same during the process of conversion
    custom_conversion_functions : dict of str -> function or empty dict
        Specify custom function to be used for conversion for given op. User can override existing conversion
        function and provide their own custom implementation to convert certain ops. Dictionary key must be string
        specifying Op name or Op type and value must be a function implementation available in current context.
        If user provides two separate functions for node name and node type, then custom function tied to node name will be used.
        As, function tied to node type is more generic than one tied to node name.
        custom_conversion_functions option is different than add_custom_layers.
        Both options can be used in conjunction in which case, custom function will be invoked for provided ops and
        custom layer will be added for ops with no respective conversion function. This option gives finer control to user.
        One use case could be to modify input attributes or certain graph properties before calling existing conversion function.
        Note that, It is custom conversion function's responsibility to add respective Core ML layer into builder (coremltools's NeuralNetworkBuilder)
    custom_shape_functions : dict of str -> functions or empty dict
        Specify custom function to compute `output` shape given `input` shape for given custom operator
        This is required for new converter path, which maintains and propagates shapes while converting operators.
    """
    if outputs is not None:
        ssa.extract_subgraph(outputs, name=top_func)

    if DEBUG:
        import graphviz
        dot_string = ssa.get_dot_string(annotation=True, name_and_op_style=False, highlight_debug_nodes=[])
        graphviz.Source(dot_string).view(filename='/tmp/ssa')

    # apply passes on the ssa, prior to conversion
    passes = [
        constant_weight_link_removal,
        fuse_bias_add,
        onehot_matmul_to_embedding,
        fuse_layer_norm,
        fuse_gelu,
        transform_nhwc_to_nchw,
        remove_identity,
        remove_no_ops_and_shift_control_dependencies,
        remove_single_isolated_node,
        fuse_conv_mul_add_into_batchnorm,
        spatial_reduce_to_global_pool,
        fuse_pad_into_conv,
        remove_oneway_split,
    ]

    for p in passes:
        p(ssa)

    if DEBUG:
        import graphviz
        dot_string = ssa.get_dot_string(annotation=True, name_and_op_style=False, highlight_debug_nodes=[])
        graphviz.Source(dot_string).view(filename='/tmp/ssa_after_passes')

    for f in list(ssa.functions.values()):
        check_connections(f.graph)

    # Set classifier flag
    is_classifier = class_labels is not None
    neural_network_type = 'classifier' if is_classifier else None

    converter = SSAConverter(ssa,
                             top_func=top_func,
                             inputs=inputs,
                             outputs=outputs,
                             neural_network_type=neural_network_type,
                             add_custom_layers=add_custom_layers,
                             custom_conversion_functions=custom_conversion_functions,
                             custom_shape_functions=custom_shape_functions,
                             optional_inputs = optional_inputs)

    converter.convert()

    builder = converter._get_builder(func=top_func)
    # Add image input identifier
    if image_input_names is not None and isinstance(
        image_input_names, _string_types):
        image_input_names = [image_input_names]

    # Add classifier classes (if applicable)
    if is_classifier:
        classes_in = class_labels
        if isinstance(classes_in, _string_types):
            import os
            if not os.path.isfile(classes_in):
                raise ValueError("Path to class labels (%s) does not exist." % \
                    classes_in)
                with open(classes_in, 'r') as f:
                    classes = f.read()
                classes = classes.splitlines()
            elif type(classes_in) is list: # list[int or str]
                classes = classes_in
            else:
                raise ValueError('Class labels must be a list of integers / strings,'\
                    ' or a file path')

            if predicted_feature_name is not None:
                builder.set_class_labels(
                    classes, predicted_feature_name=predicted_feature_name,
                    prediction_blob=predicted_probabilities_output)
            else:
                builder.set_class_labels(classes)

    image_format = ssa.get_image_format()
    # Set pre-processing parameters
    builder.set_pre_processing_parameters(image_input_names=image_input_names,
                                          is_bgr=is_bgr,
                                          red_bias=red_bias,
                                          green_bias=green_bias,
                                          blue_bias=blue_bias,
                                          gray_bias=gray_bias,
                                          image_scale=image_scale,
                                          image_format=image_format)

    mlmodel_spec = converter.get_spec()


    # Required if an output node produces multiple outputs
    # Generate new output features
    modified_output_features_list = []
    for idx, output_feature in enumerate(mlmodel_spec.description.output):
        if output_feature.name in converter.op_tensor_map:
            atype = mlmodel_spec.description.output[idx].type
            for aname in converter.op_tensor_map[output_feature.name]:
                new_feature = Model_pb2.FeatureDescription()
                new_feature.name = aname
                new_feature.type.CopyFrom(atype)
                if aname not in [feature.name for feature in modified_output_features_list]:
                    modified_output_features_list.append(new_feature)
        else:
            modified_output_features_list.append(output_feature)


    # delete the existing output feature
    mlmodel_spec.description.ClearField('output')

    # creating new output features description
    mlmodel_spec.description.output.extend(modified_output_features_list)

    # MLModel passes
    mlmodel_passes = [remove_disconnected_constants]
    for p in mlmodel_passes:
        p(mlmodel_spec)

    
    if DEBUG:
        coremltools.models.utils.save_spec(mlmodel_spec, '/tmp/model_from_spec.mlmodel')

    return mlmodel_spec


class SSAConverter(object):
    def __init__(self,
                 net_ensemble, # type: NetworkEnsemble
                 top_func='main', # type: str
                 inputs=None, # type: Dict[str, tuple]
                 outputs=None, # type: List[str]
                 neural_network_type=None, # type: str
                 add_custom_layers=False,  # type: bool
                 custom_conversion_functions={},  # type: Dict[Text, Any]
                 custom_shape_functions={}, # type: Dict[Text, Any]
                 optional_inputs = [] # type: List[str]
                 ):
        self.net_ensemble = net_ensemble
        self.top_func = top_func  # string indicating the top level function
        if self.top_func not in self.net_ensemble.functions:
            raise ValueError(
                'Top level function %s not in the NetworkEnsemble Provided' % self.top_func)

        # get top level inputs and outputs to instantiate spec
        self.net_ensemble.functions[top_func].find_inputs_and_outputs()
        top_input_names = list(map(str, self.net_ensemble.functions[top_func].inputs))
        top_output_names = list(map(str, self.net_ensemble.functions[top_func].outputs))

        top_ssa = self.net_ensemble.functions[top_func]

        # custom conversion functions
        self.custom_conversion_functions = custom_conversion_functions
        self.add_custom_layers = add_custom_layers
        self.custom_shape_functions = custom_shape_functions

        # find_inputs_and_outputs() generates a list of required inputs, which
        # may not be supplied by inputs. We need to make sure that the
        # user-supplied inputs name and shape are consistent with the NNSSA.
        top_input_shapes = []
        for name in top_input_names:
            node = top_ssa.graph[name]

            shape = self._get_tensor_shape_from_type(node.datatype)

            if shape is None and inputs is None:
                raise ValueError(
                    'NNSSA input "%s" has non-static shape %s, please provide in argument "inputs"'
                    % (name, str(shape)))
            if inputs is not None:
                if name not in inputs:
                    raise ValueError(
                        'Input "%s" is required by SSAConverter, but not passed in argument "inputs"' % name)
                if shapes.is_static_shape(inputs[name]) and not shapes.is_a_shape_of(inputs[name], shape):
                    raise ValueError(
                        'Input "%s" expects a shape compatible to %s, but is given %s' %
                        (name, str(shape), inputs[name]))
                # Now that we can use the shape to create top_input_shapes
                shape = inputs[name] if inputs[name] else [1, ]
            top_input_shapes.append(shape)

        top_input_types = []
        is_input_optional = [True if name in optional_inputs else False for name in top_input_names]
        is_input_dynamic = [True if not shapes.is_static_shape(shape) else False for shape in top_input_shapes]
        for idx, dims in enumerate(top_input_shapes):
            if is_input_dynamic[idx]:
                static_shape = [dim_size if dim_size > 0 else 1 for dim_size in dims]
            else:
                static_shape = dims

            top_input_types.append(datatypes.Array(*static_shape))
        top_input_features = list(zip(top_input_names, top_input_types))

        # TODO - verify outputs
        if outputs is not None:
            top_output_features = []
            for name in outputs:
                if name in self.net_ensemble.variables.keys():  # Variable/States are optional inputs & outputs to be added later
                    continue
                elif name in top_output_names:
                    top_output_features.append((name, None))
                else:
                    if len(top_output_names) == 1:
                        raise ValueError('Output "{}" is not an output node in the source graph. Do you mean "{}"?'
                                         .format(name, top_output_names[0]))
                    else:
                        raise ValueError('Output "%s" is not an output node in the source graph.' % name)
        else:
            top_output_features = list(zip(top_output_names, [None] * len(top_output_names)))

        self.top_builder = NeuralNetworkBuilder(input_features=top_input_features,
                                                output_features=top_output_features,
                                                disable_rank5_shape_mapping=True,
                                                mode=neural_network_type)

        self.spec = self.top_builder.spec

        for idx, input in enumerate(self.spec.description.input):
            if is_input_dynamic[idx]:
                input_name = top_input_names[idx]
                dynamic_shape = top_input_shapes[idx]
                lower_bounds, upper_bounds = [], []
                for dim_size in dynamic_shape:
                    if dim_size > 0:
                        lower_bounds.append(dim_size)
                        upper_bounds.append(dim_size)
                    else:
                        lower_bounds.append(1)
                        upper_bounds.append(-1)
                set_multiarray_ndshape_range(self.spec, input_name, lower_bounds=lower_bounds, upper_bounds=upper_bounds)

            if is_input_optional[idx]:
                self.spec.description.input[idx].type.isOptional = True

        self.CONVERT_FUNCTION_MAP = {
            'Placeholder': self._convert_input,
            'Const': self._convert_const,
            'Transpose': self._convert_transpose,
            'Shape': self._convert_shape,
            'Size': self._convert_size,
            'Slice': self._convert_slice,
            'StridedSlice': self._convert_slice,
            'Range': self._convert_range,
            'TensorArrayV3': self._convert_tensorarray_alloc,
            'TensorArrayScatterV3': self._convert_array_scatter,
            'TensorArraySizeV3': self._convert_tensorarray_size,
            'TensorArrayGatherV3': self._convert_tensorarray_gather,
            'TensorArrayReadV3': self._convert_tensorarray_read,
            'TensorArrayWriteV3': self._convert_tensorarray_write,
            'while': self._convert_while,
            'function_entry': self._convert_function,
            'get_tuple': self._convert_get_tuple,
            'make_tuple': self._convert_make_tuple,
            'get_global': self._convert_get_global,
            'set_global': self._convert_set_global,
            'Greater': self._convert_binary_broadcastable,
            'GreaterEqual': self._convert_binary_broadcastable,
            'NotEqual': self._convert_binary_broadcastable,
            'Equal': self._convert_binary_broadcastable,
            'Less': self._convert_binary_broadcastable,
            'LessEqual': self._convert_binary_broadcastable,
            'LogicalAnd': self._convert_binary_broadcastable,
            'LogicalOr': self._convert_binary_broadcastable,
            'LogicalNot': self._convert_unary_logical_not,
            'LogSoftmax': self._convert_unary_log_softmax,
            'return': self._convert_return,
            'Maximum': self._convert_binary_broadcastable,
            'Minimum': self._convert_binary_broadcastable,
            'Add': self._convert_binary_broadcastable,
            'Sub': self._convert_binary_broadcastable,
            'Mul': self._convert_binary_broadcastable,
            'RealDiv': self._convert_binary_broadcastable,
            'FloorDiv': self._convert_binary_broadcastable,
            'BiasAdd': self._convert_binary_broadcastable,
            'Pow': self._convert_binary_broadcastable,
            'FloorMod': self._convert_floor_mod,
            'SquaredDifference': self._convert_squared_difference,
            'Concat': self._convert_concat_nd,
            'ConcatV2': self._convert_concat_nd,
            'MatMul': self._convert_batched_mat_mul,
            'BatchMatMul': self._convert_batched_mat_mul,
            'Embedding': self._convert_embedding,
            'Split': self._convert_split,
            'SplitV': self._convert_split,
            'Sigmoid': self._convert_unary_activation,
            'Relu': self._convert_unary_activation,
            'Relu6': self._convert_unary_activation_relu6,
            'LeakyRelu': self._convert_unary_activation,
            'Tanh': self._convert_unary_activation,
            'Elu': self._convert_unary_activation,
            'Identity': self._convert_identity,
            'Cast': self._convert_cast,
            'Pack': self._convert_pack,
            'Unpack': self._convert_unpack,
            'Gather': self._convert_gather,
            'GatherNd': self._convert_gather_nd,
            'ScatterNd': self._convert_scatter_nd,
            'Square': self._convert_unary_square,
            'Neg': self._convert_unary_neg,
            'Reciprocal': self._convert_unary_inverse,
            'Sqrt': self._convert_unary_common,
            'Rsqrt': self._convert_unary_common,
            'Exp': self._convert_unary_common,
            'Log': self._convert_unary_common,
            'Abs': self._convert_unary_common,
            'Sign': self._convert_unary_common,
            'Ceil': self._convert_unary_common,
            'Floor': self._convert_unary_common,
            'Round': self._convert_unary_common,
            'Sin': self._convert_unary_trigonometric,
            'Cos': self._convert_unary_trigonometric,
            'Tan': self._convert_unary_trigonometric,
            'GeLU': self._convert_gelu,
            'SelectMask': self._convert_select,
            'Where': self._convert_where,
            'Conv2D': self._convert_conv2d,
            'Conv2DBackpropInput': self._convert_conv2d_transpose,
            'DepthwiseConv2dNative': self._convert_conv2d,
            'MaxPool': self._convert_maxpool,
            'AvgPool': self._convert_avgpool,
            'Reshape': self._convert_reshape,
            'Softmax': self._convert_softmax,
            'Prod': self._convert_reduction,
            'Mean': self._convert_reduction,
            'Sum': self._convert_reduction,
            'Max': self._convert_reduction,
            'Min': self._convert_reduction,
            'All': self._convert_reduction,
            'Any': self._convert_reduction,
            'ArgMax': self._convert_argmax,
            'ArgMin': self._convert_argmin,
            'ReverseV2': self._convert_reverse,
            'ReverseSequence': self._convert_reverse_sequence,
            'ExpandDims': self._convert_expand_dims,
            'Squeeze': self._convert_squeeze,
            'Tile': self._convert_tile,
            'Fill': self._convert_fill,
            'LSTMBlock': self._convert_lstm_block_cell,
            'Pad': self._convert_constant_pad,
            'PadV2': self._convert_constant_pad,
            'MirrorPad': self._convert_mirror_pad,
            'TopKV2': self._convert_topk,
            'iff': self._convert_iff,
            'ResizeBilinear': self._convert_resize_bilinear,
            'ResizeNearestNeighbor': self._convert_resize_nearest_neighbor,
            'LayerNormalization': self._convert_layer_normalization,
            'SpaceToDepth': self._convert_reorganize_data,
            'DepthToSpace': self._convert_reorganize_data,
            'SpaceToBatchND': self._convert_space_to_batch_nd,
            'BatchToSpaceND': self._convert_batch_to_space_nd,
            'BatchNorm': self._convert_batchnorm,
            'LRN': self._convert_lrn,
            'ClipByValue': self._convert_clip,
        }

        # converter state variables
        # func_stack stores a list of NNSSA function names
        self.func_stack = [self.top_func]
        # Theoretically, there should be a one-to-one mapping between
        # SSA function and nn_spec, which is associated with a NeuralNetworkBuilder
        self.func_builder_map = {self.top_func: self.top_builder}
        # All the shapes of the tensor of CoreML str:shape
        self.tensor_shapes = {
            name: top_input_shapes[idx]
            for idx, name in enumerate(top_input_names)
        }
        # Map for tensors generated by special ops (make_tuple, get_tuple, function, return, etc)
        # and value is the list of node names that represent tensors
        self.op_tensor_map = {}

        # all variables/states are treated as both inputs & outputs.
        for name, aVariable in self.net_ensemble.variables.items():
            if _is_scalar(aVariable):
                shape = [1, ]
            else:
                assert builtins.is_tensor(aVariable)
                shape = list([int(i) if i and i > 0 else 1 for i in self._get_tensor_shape_from_type(aVariable)])

            self.top_builder.add_optionals([(name + '__invar__', shape)], [(name + '__outvar__', shape)])
            self.tensor_shapes[name + '__invar__'] = shape

    def get_spec(self):
        return self.spec

    def print_function_nodes(self, func_name):
        if func_name not in self.net_ensemble.functions:
            raise ValueError('%s is not a function name in NetworkEnsemble' % func_name)
        graph = self.net_ensemble.functions[func_name].graph
        for name, node in graph.items():
            if node.op == 'get_global':
                print('%s (%s) var = %s' % (name, node.op, node.attr['variable']))
            if node.op == 'set_global':
                print('%s (%s) var = %s' % (name, node.op, node.attr['variable']))

    def get_nnssa_inputs_outputs(self):
        inputs, outputs, placeholder_defaults = self.net_ensemble._get_inputs_outputs()
        print('Inputs: ')
        for i in inputs:
            print(i)
        print('Outputs: ')
        for o in outputs:
            print(o)
        print('Placeholders with default: ')
        for p in placeholder_defaults:
            print(p)

    def convert(self):
        """ Convert the NNSSA function on top of func_stack into NeuralNetworkSpec.
        """
        func_name = self.func_stack[-1]
        func = self.net_ensemble.functions[func_name]
        print('[SSAConverter] Converting function %s ...' % func_name)

        # Do a topological sort
        restricted_graph = {}
        function = self.net_ensemble.functions[func_name]
        for k, v in function.graph.items():
            if len(v.outputs) > 0 and all(
                    [function.graph[i].value is not None for i in v.outputs]):
                continue
            restricted_graph[k] = v
        instruction_order = topsort(restricted_graph)

        # Make a buffer between variable inputs
        builder = self._get_builder()
        for name, var in self.net_ensemble.variables.items():
            layer = builder.add_copy(
                name=name + '_copy',
                input_name=name+'__invar__',
                output_name=name)
            shapes.propagate_single_layer(layer, self.tensor_shapes)

        # Convert operations one by one
        for idx, node_name in enumerate(instruction_order):
            node = func.graph[node_name]
            op_type = node.op

            custom_conversion_name = None
            if node_name in self.custom_conversion_functions:
                custom_conversion_name = node_name
            elif op_type in self.custom_conversion_functions:
                custom_conversion_name = op_type

            # Set conversion function and message
            conversion_message = ''
            if custom_conversion_name is not None:
                conversion_message = ' with custom conversion function'
            elif op_type in self.CONVERT_FUNCTION_MAP:
                convert_func = self.CONVERT_FUNCTION_MAP[op_type]
            elif self.add_custom_layers:
                # Add custom layer
                convert_func = self._convert_custom_layer
                conversion_message = ' with custom layer'
            else:
                raise NotImplementedError(
                    '[SSAConverter] Conversion for op %s not implemented, terminating...' % op_type)

            print('[SSAConverter] [{}/{}] Converting op type \'{}\', of name \'{}\' {}'.format(
                idx + 1, len(instruction_order), op_type, node_name, conversion_message,
                (('(output shape: ' + str(node.datatype.get_shape()) + ')') if builtins.is_tensor(node.datatype) else '')))

            # If custom conversion method is provided, use it
            # Otherwise, invoke internal conversion method
            if custom_conversion_name is not None:
                self.custom_conversion_functions[custom_conversion_name](self, node)
            else:
                convert_func(node)

        # Make a buffer between variable inputs
        builder = self._get_builder()
        for name, var in self.net_ensemble.variables.items():
            layer = builder.add_copy(
                name=name + '_copy_r',
                input_name=name,
                output_name=name+'__outvar__')
            shapes.propagate_single_layer(layer, self.tensor_shapes)


    def _get_builder(self, func=None):
        if func is None:
            func = self.func_stack[-1]
        return self.func_builder_map[func]

    def _get_tensor_shape_from_type(self, type_):
        if _is_scalar(type_):
            shape = (1,)
        elif builtins.is_tensor(type_):
            shape = type_.get_shape()
        elif builtins.is_list(type_):
            element_shape = type_.T[0].get_shape()
            for ashape in type_.T:
                assert ashape.get_shape() == element_shape
            shape = [-1] + list(element_shape)
        else:
            shape = None
        return shape

    def _get_input_tensors(self, node, inspect_shapes=True):
        """ Get the input nodes, their names and types for a node.
        There are two cases:
        (1) (Tuple case) input is a tuple. In this case, expand that tuple input into a list of input tensors
        (2) (Regular case) input is a node name. In this case just copy it.
        (3) (Indexed tuple case) input is one element in a tuple. In this case it should be stored in op_tensor_map
        """
        input_nodes, input_names, input_types = [], [], []

        for name in node.inputs:
            if name in self.op_tensor_map:
                input_names.extend(self.op_tensor_map[name])
            else:
                input_names.append(name)

        for name in input_names:
            if name in self.net_ensemble.variables:
                input_node, _ = self.__get_node_and_type_by_name(name + "/read")
                input_type = self.net_ensemble.variables[name]
            else:
                input_node, input_type = self.__get_node_and_type_by_name(name)

            assert input_node is not None
            assert input_type is not None
            input_nodes.append(input_node)
            input_types.append(input_type)

            if inspect_shapes:
                self.__compare_propagated_and_inferred_shape(name, input_type)

        return input_nodes, input_names, input_types

    def __get_node_and_type_by_name(self, name):
        for fname in self.func_stack[::-1]:
            func = self.net_ensemble.functions[fname]
            if name in func.graph:
                node = func.graph[name]
                return node, node.datatype

        for node_name, output_names in self.op_tensor_map.items():
            if name in output_names:
                node, type_ = self.__get_node_and_type_by_name(node_name)
                if builtins.is_tuple(type_):
                    Id = output_names.index(name)
                    type_ = node.datatype.T[Id]
                return node, type_

        return None, None

    def __compare_propagated_and_inferred_shape(self, name, type_):

        propagated_shape = self.tensor_shapes[name]
        if _is_scalar(type_):
            inferred_shape = (1,)
        elif builtins.is_tensor(type_):
            inferred_shape = type_.get_shape()
        elif builtins.is_list(type_):
            element_shape = type_.T[0].get_shape()
            for ashape in type_.T:
                assert ashape.get_shape() == element_shape
            inferred_shape = [-1] + list(element_shape)
        else:
            raise ValueError('[SSAConverter] Failed to infer shape'
                             ' for tensor %s' % name)

        mismatch = '[SSAConverter] Shape mismatch between inferred {} and propagated {} for tensor {}'.format(
            inferred_shape, propagated_shape, name)

        if len(propagated_shape) != len(inferred_shape):
            raise ValueError(mismatch)

        for pdim, idim in zip(propagated_shape, inferred_shape):
            if pdim == -1 or idim == -1 or pdim == idim:
                continue
            raise ValueError(mismatch)

    def _convert_input(self, node):
        """ Convert an input node. For now, we may just need to skip it.
        """
        pass

    def _convert_const(self, node):
        """ Convert a constant node.
        """
        val = np.array(node.value.val)
        if len(val.shape) == 0:
            val = np.array([node.value.val])
        builder = self._get_builder()
        layer = builder.add_load_constant_nd(
            name=node.name, output_name=node.name, constant_value=val, shape=val.shape)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_custom_layer(self, node):
        """ Add custom layer
        """
        params = NeuralNetwork_pb2.CustomLayerParams()
        params.className = node.op
        params.description = "Custom layer that corresponds to the TensorFlow op {}".format(node.op)
        builder = self._get_builder()
        layer = builder.add_custom(name=node.name,
                                   input_names=node.inputs,
                                   output_names=[node.name],
                                   custom_proto_spec=params)

        if node.op not in self.custom_shape_functions:
            raise ValueError('Custom Shape Function for {} not provided!'.format(node.op))
        shapes.propagate_single_layer(layer, self.tensor_shapes, custom_shape_function=self.custom_shape_functions[node.op])

    def _convert_transpose(self, node):
        """ Convert a transpose op.
        """
        # permute dimensions are assumed to be a const
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        dim = input_nodes[1].value.val if len(input_names) > 1 else node.attr.get('dim')
        if dim is None:
            raise ValueError('[SSAConverter] Cannot handle dynamic Transpose')
        dim = list(dim)
        builder = self._get_builder()

        layer = builder.add_transpose(
            name=node.name, axes=dim, input_name=input_names[0], output_name=node.name)

        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_shape(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        assert (len(input_names) == 1)
        builder = self._get_builder()
        layer = builder.add_get_shape(
            name=node.name, input_name=input_names[0], output_name=node.name)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_size(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        assert (len(input_names) == 1)
        builder = self._get_builder()
        layer = builder.add_get_shape(
            name=node.name + "_shape", input_name=input_names[0], output_name=node.name + "_shape")

        layer = builder.add_reduce_prod(
            name=node.name,
            input_name=node.name + "_shape",
            output_name=node.name,
            keepdims=True,
            reduce_all=True)

        self.tensor_shapes[node.name] = [1]

    def _convert_slice(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        has_squeeze = 'squeeze' in node.attr and node.attr['squeeze']
        axes = node.attr.get('squeeze')

        if _is_scalar(node.datatype):
            output_shape = []
        elif builtins.is_tensor(node.datatype):
            output_shape = self._get_tensor_shape_from_type(node.datatype)
        else:
            output_shape = None

        if has_squeeze:
            if output_shape is None:
                raise ValueError('[SSAConverter] Unable to determine output shapes for Slice')
            if len(output_shape) == 0 and len(axes) == 1:
                has_squeeze = False

        slice_output_name = node.name + '_slice_' if has_squeeze else node.name

        builder = self._get_builder()

        rank = len(self._get_tensor_shape_from_type(input_types[0]))
        begin_masks = [True if i in node.attr['begin_masks'] else False for i in range(rank)]
        end_masks = [True if i in node.attr['end_masks'] else False for i in range(rank)]
        if 'slice' not in node.attr:
            assert node.attr["new_axis_mask"] == 0
            assert len(input_names) >= 4
            layer = builder.add_slice_dynamic(name=slice_output_name,
                                              input_names=input_names[:4],
                                              output_name=slice_output_name,
                                              begin_masks=begin_masks,
                                              end_masks=end_masks)

            if not has_squeeze and output_shape:
                self.tensor_shapes[node.name] = output_shape
            else:
                shapes.propagate_single_layer(layer, self.tensor_shapes)

        else:
            # For simple RNN, node.attr always has a 'slice'
            # This means slicing is always static
            # each slice is [begin, end, step]
            slices = node.attr['slice']
            begin_indices, end_indices, strides = [], [], []
            for s in slices:
                begin_indices.append(s[0])
                end_indices.append(s[1])
                strides.append(s[2])

            layer = builder.add_slice_static(
                name=slice_output_name,
                input_name=input_names[0],
                output_name=slice_output_name,
                begin_ids=begin_indices,
                end_ids=end_indices,
                strides=strides,
                begin_masks=begin_masks,
                end_masks=end_masks)

            shapes.propagate_single_layer(layer, self.tensor_shapes)

        if has_squeeze:
            input_shape = self._get_tensor_shape_from_type(input_types[0])
            input_rank  = len(input_shape)
            squeeze_all = (input_rank == len(axes))
            layer = builder.add_squeeze(
                name=node.name,
                input_name=slice_output_name,
                output_name=node.name,
                axes= axes if not squeeze_all else None,
                squeeze_all = squeeze_all)
            shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_range(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        if len(input_names) != 3:
            raise ValueError(
                'CoreML NeuralNetwork range layer must have 3 inputs: start, end and step')
        input_names = [input_names[1], input_names[0], input_names[2]]

        builder = self._get_builder()
        layer = builder.add_range_dynamic(name=node.name, output_name=node.name, input_names=input_names)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_tensorarray_alloc(self, node):
        # TensorArray is a list of tensors, it will be treated as a rank+1
        # tensor when converted. The shape information is stored at two
        # different places - node input specifies the length of the list
        # while the node's datatype stores the shape of each tensor allocated.
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        assert (len(input_names) == 1)

        element_shape = node.datatype.T[0].get_shape()
        if (not node.attr.get('identical_element_shapes', True) or
                not all([atype.get_shape() == element_shape for atype in node.datatype.T])):
            raise ValueError(
                '[SSAConverter] TensorArray allocation cannot handle arrays'
                'with tensors of various shapes.')

        has_static_element_shape = all([dim > 0 for dim in element_shape])

        if input_nodes[0].op == 'Const':
            length = input_nodes[0].value.val
            array_size = length if length > 0 else 1
        elif 'size' in node.attr and isinstance(node.attr['size'], int):
            array_size = node.attr['size']
        else:
            array_size = None

        # Simpler case: No dynamic shape
        if array_size is not None and has_static_element_shape:
            array_shape = [array_size] + list(element_shape)
            layer = self._get_builder().add_load_constant_nd(
                name=node.name,
                output_name=node.name,
                constant_value=np.zeros(array_shape, dtype='float'),
                shape=array_shape)
            shapes.propagate_single_layer(layer, self.tensor_shapes)
        elif has_static_element_shape:
            # Load element shape into network
            builder = self._get_builder()
            if element_shape:
                node_es_name = node.name + '__element_shape'
                layer = builder.add_load_constant_nd(
                    name=node_es_name,
                    output_name=node_es_name,
                    constant_value=np.array(element_shape, dtype='float'),
                    shape=[len(element_shape)])
                shapes.propagate_single_layer(layer, self.tensor_shapes)

                # Concatenate list length (the input, should be a constant vector of size 1) with element shape
                node_arr_shape_name = node.name + '__arr_shape'
                layer = builder.add_concat_nd(
                    name=node_arr_shape_name,
                    input_names=input_names + [node_es_name],
                    output_name=node_arr_shape_name,
                    axis=0)
                shapes.propagate_single_layer(layer, self.tensor_shapes)
            else:
                node_arr_shape_name = input_names[0]

            # Now allocate required shape
            layer = builder.add_fill_dynamic(
                name=node.name, input_name=node_arr_shape_name, output_name=node.name)
            shapes.propagate_single_layer(layer, self.tensor_shapes)
            # Overwrite the output shape with fixed element shape
            self.tensor_shapes[node.name][1:] = element_shape
            layer.outputTensor[0].dimValue[1:] = element_shape
        else:
            raise ValueError(
                '[SSAConverter] TensorArray allocation cannot determine element shapes statically'
            )

    def _convert_array_scatter(self, node):
        # NNSSA input order: indices, value, array
        # CoreML input order: container (array), indices, slices (value)

        input_nodes, input_names, input_types = self._get_input_tensors(node)
        if len(input_names) != 3:
            raise ValueError('Scatter only accepts 3 inputs')
        indices, value, array = input_names
        layer = self._get_builder().add_scatter(
            name=node.name, input_names=[array, indices, value], output_name=node.name)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_make_tuple(self, node):
        # make tuple aggregates a list of SSA nodes (which also stands for their outputs)
        # For now, I think recording the make_tuple node itself for reference would suffice.
        if node.name in self.op_tensor_map:
            raise ValueError('make_tuple node %s should not be visited twice.' % node.name)

        input_nodes, input_names, input_types = self._get_input_tensors(node)
        self.op_tensor_map[node.name] = input_names

    def _convert_while(self, node):
        # In CoreML, loops and branches should be designed such that inputs / outputs
        # should be empty, because it is not necessary and not clearly defined.
        # Should only take a tuples
        assert (len(node.inputs) == 1)
        current_graph = self.net_ensemble.functions[self.func_stack[-1]].graph
        assert (current_graph[node.inputs[0]].op == 'make_tuple')
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        self.op_tensor_map[node.name] = input_names
        builder_top = self._get_builder()
        while_layer = builder_top.add_loop(name=node.name)

        loop_param = while_layer.loop
        loop_param.maxLoopIterations = 0

        # Both body function and condition function share the same inputs (args) of the loop
        # convert the condition function
        if 'cond_function' in node.attr:
            if not loop_param.HasField('conditionNetwork'):
                loop_param.condition.MergeFromString(b'')
            cond_func_name = node.attr['cond_function']
            # TODO - need to find cond_var name
            self.func_stack.append(cond_func_name)
            self.func_builder_map[cond_func_name] = NeuralNetworkBuilder(
                nn_spec=loop_param.conditionNetwork, disable_rank5_shape_mapping=True)

            self.op_tensor_map[cond_func_name] = input_names
            self.convert()
            cond_func = self.net_ensemble.functions[cond_func_name]
            ret_node_name = cond_func.outputs[0]
            loop_param.conditionVar = cond_func.graph[ret_node_name].inputs[0]
            self.func_stack.pop()
        else:
            raise ValueError('Unable to determine condition function in the loop')

        # convert the body function
        if 'body_function' not in node.attr:
            raise ValueError('A "while" SSA node should not be empty.')
        if not loop_param.HasField('bodyNetwork'):
            loop_param.bodyNetwork.MergeFromString(b'')

        body_func_name = node.attr['body_function']
        self.func_stack.append(body_func_name)
        self.func_builder_map[body_func_name] = NeuralNetworkBuilder(
            nn_spec=loop_param.bodyNetwork, disable_rank5_shape_mapping=True)

        self.op_tensor_map[body_func_name] = input_names
        self.convert()

        # The body function should re-write variables when it returns.
        body_func = self.net_ensemble.functions[body_func_name]
        loop_var_tuple_name = None
        for k, v in body_func.graph.items():
            # k is name, v is node
            if v.op == 'make_tuple' and body_func.graph[v.outputs[0]].op == 'return':
                loop_var_tuple_name = k
                break

        loop_var_names = self.op_tensor_map[loop_var_tuple_name]
        assert len(loop_var_names) == len(input_names)

        # Loop body should have the same input and output
        builder_body = self._get_builder()
        for src, dst in zip(loop_var_names, input_names):
            # loop variables may be passed as an input to while op but unused.
            if src == dst:
                continue
            layer = builder_body.add_copy(
                name='copy_' + src + '_' + dst, input_name=src, output_name=dst)
            shapes.propagate_single_layer(layer, self.tensor_shapes)

        # Pop back into while's loop
        self.func_stack.pop()

    def _convert_function(self, node):
        # Function node is the entry point of a function
        pass

    def _convert_get_tuple(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        self.op_tensor_map[node.name] = [input_names[node.attr['index']]] if node.attr['index'] < len(input_names) else []

    def _convert_get_global(self, node):
        input_name = node.attr["variable"]
        self.op_tensor_map[node.name] = [input_name]

    def _convert_set_global(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        output_name = node.attr["variable"]

        builder = self._get_builder()

        if len(node.outputs) > 0:
            self.op_tensor_map[node.name] = [input_names[0]]

        if input_nodes[0].op == "Const" and input_nodes[0].value.val.size == 0:
            return

        if output_name != input_names[0]:
            layer = builder.add_copy(name=node.name,
                                     input_name=input_names[0],
                                     output_name=output_name)

            shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_return(self, node):
        # When converting a body function of a loop, return node should overwrite body functions' input tensors
        pass

    def _convert_unary_logical_not(self, node):
        assert len(node.inputs) == 1
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        layer = self._get_builder().add_logical(
            name=node.name,
            input_names=input_names,
            output_name=node.name,
            mode='NOT')
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_floor_mod(self, node):
        assert len(node.inputs) == 2
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        a, b = input_names
        a_div_b = node.name + "_floor_div"
        floor_a = node.name + "_floor_a"

        if builtins.is_int(node.attr['T']):
            round_a = node.name + "_round_a"
            round_b = node.name + "_round_b"

            layer = self._get_builder().add_round(name=round_a,
                                                  input_name=a,
                                                  output_name=round_a)
            shapes.propagate_single_layer(layer, self.tensor_shapes)

            layer = self._get_builder().add_round(name=round_b,
                                                  input_name=b,
                                                  output_name=round_b)
            shapes.propagate_single_layer(layer, self.tensor_shapes)

            a, b = round_a, round_b

        layer = self._get_builder().add_floor_div_broadcastable(
            name=a_div_b, input_names=[a, b], output_name=a_div_b)

        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = self._get_builder().add_multiply_broadcastable(
            name=floor_a, input_names=[a_div_b, b], output_name=floor_a)

        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = self._get_builder().add_subtract_broadcastable(
            name=node.name, input_names=[a, floor_a], output_name=node.name)

        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_squared_difference(self, node):
        assert (len(node.inputs) == 2)
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        sub_node_name = node.name + '_sub_'

        layer = self._get_builder().add_subtract_broadcastable(
            name=sub_node_name, input_names=input_names, output_name=sub_node_name)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = self._get_builder().add_unary(
            name=node.name, input_name=sub_node_name, output_name=node.name, mode='power', alpha=2.0)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_select(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        assert (len(input_names) == 3)
        cond_name, true_name, false_name = input_names

        if "expand_dims" in node.attr:
            axes = node.attr["expand_dims"]
            cond_output_name = node.name + '_expanded'
            layer = self._get_builder().add_expand_dims(
                name=cond_output_name, input_name=cond_name, output_name=cond_output_name, axes=axes)
            shapes.propagate_single_layer(layer, self.tensor_shapes)
            cond_name = cond_output_name

        layer = self._get_builder().add_where_broadcastable(
            name=node.name,
            input_names=[cond_name, true_name, false_name],
            output_name=node.name)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_where(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        if len(input_names) == 3:
            self._convert_select(node)
        else:
            assert len(input_names) == 1
            layer = self._get_builder().add_where_nonzero(name=node.name,
                                                          input_name=input_names[0],
                                                          output_name=node.name)

            self.tensor_shapes[node.name] = self._get_tensor_shape_from_type(node.datatype)

    def _convert_softmax(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        axis = -1 if 'axis' not in node.attr else node.attr['axis']
        layer = self._get_builder().add_softmax_nd(
            name=node.name, input_name=input_names[0], output_name=node.name, axis=axis)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_tensorarray_read(self, node):
        # TensorArrayReadV3 slices an element from TensorArray, which in NNSSA is a list.
        # This is equivalent to array gather
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        slice_output_name = node.name + '_slice_'
        layer = self._get_builder().add_gather(
            name=node.name + '_gather_',
            input_names=input_names[::-1],
            output_name=slice_output_name,
            axis=0)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        # tensorarray_read should generate only 1 slice, so adding a squeeze should be enough
        layer = self._get_builder().add_squeeze(
            name=node.name + '_squeeze_',
            input_name=slice_output_name,
            output_name=node.name,
            axes=[0])
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_tensorarray_write(self, node):
        """def TensorArrayWrite(index, value, array):
        array[index] = value
        return array
        """
        # node.inputs = ['index', 'value', 'array']
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        assert (len(input_names) == 3)

        index_name, value_name, array_name = input_names
        if 'dynamic_size' in input_nodes[-1].attr:
            builder = self._get_builder()
            layer = builder.add_get_shape(
                name=array_name + '_full_shape',
                input_name=array_name,
                output_name=array_name + '_full_shape')
            shapes.propagate_single_layer(layer, self.tensor_shapes)

            layer = builder.add_slice_static(
                name=array_name + '_length',
                input_name=array_name + '_full_shape',
                output_name=array_name + '_length',
                begin_ids=[0],
                end_ids=[1],
                begin_masks=[False],
                end_masks=[False],
                strides=[1])
            shapes.propagate_single_layer(layer, self.tensor_shapes)

            layer = builder.add_slice_static(
                name=array_name + '_element_shape',
                input_name=array_name + '_full_shape',
                output_name=array_name + '_element_shape',
                begin_ids=[1],
                end_ids=[1],
                begin_masks=[False],
                end_masks=[True],
                strides=[1])
            shapes.propagate_single_layer(layer, self.tensor_shapes)

            layer = builder.add_greater_than(
                name=array_name + "_is_growing",
                input_names=[index_name, array_name + '_length'],
                output_name=array_name + "_is_growing",
                use_greater_than_equal=True
            )
            shapes.propagate_single_layer(layer, self.tensor_shapes)

            layer = builder.add_branch(
                name=array_name + "_condition",
                input_name=array_name + "_is_growing")

            ifbranch = NeuralNetworkBuilder(nn_spec=layer.branch.ifBranch,
                                            disable_rank5_shape_mapping=True)

            layer = ifbranch.add_fill_dynamic(
                name=array_name + "_alloc",
                input_name=array_name + '_element_shape',
                output_name=array_name + "_alloc",
                value=0.0)
            shapes.propagate_single_layer(layer, self.tensor_shapes)

            layer = ifbranch.add_expand_dims(
                name=array_name + "_new_element",
                input_name=array_name + "_alloc",
                output_name=array_name + "_new_element",
                axes=[0])
            shapes.propagate_single_layer(layer, self.tensor_shapes)

            layer = ifbranch.add_concat_nd(
                name=array_name + "_updated",
                input_names=[array_name, array_name + "_new_element"],
                output_name=array_name + "_updated",
                axis=0)
            shapes.propagate_single_layer(layer, self.tensor_shapes)

            layer = ifbranch.add_copy(
                name=array_name + '_assign',
                input_name=array_name + "_updated",
                output_name=array_name
            )
            shapes.propagate_single_layer(layer, self.tensor_shapes)

        values_name = node.name + '_expanded'
        layer = self._get_builder().add_expand_dims(
            name=values_name, input_name=value_name, output_name=values_name, axes=[0])
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        # 3 inputs: [Scatter target, indices, scatter source]
        layer = self._get_builder().add_scatter(
            name=node.name,
            input_names=[array_name, index_name, values_name],
            output_name=node.name)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_concat_nd(self, node):
        assert len(node.inputs) > 1
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        axis = node.attr.get('axis')
        if axis is None:
            axis = input_nodes[-1].value.val if node.op == 'ConcatV2' else input_nodes[0].value.val
        if axis is None:
            raise NotImplementedError('[SSAConverter] Dynamic concatenation is not supported')
        input_names = input_names[:-1] if node.op == 'ConcatV2' else input_names[1:]
        input_types = input_types if node.op == 'ConcatV2' else input_types[1:]
        input_names = [name for i, name in enumerate(input_names) if self._get_tensor_shape_from_type(input_types[i])[axis] != 0]

        if len(input_names) == 1:
            self.op_tensor_map[node.name] = input_names
            return

        if node.attr.get('data_format', None) == 'NHWC_format_inserted' and (axis == 1 or axis == -3):
            layer = self._get_builder().add_elementwise(node.name, input_names, node.name, 'CONCAT')
        else:
            layer = self._get_builder().add_concat_nd(
                name=node.name, input_names=input_names, output_name=node.name, axis=axis)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_batched_mat_mul(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        weight, bias = None, None
        if len(input_names) == 1:
            weight = node.attr.get('W', node.attr.get('W_const'))
            bias = node.attr.get('bias')
        elif len(input_names) == 2 and input_nodes[1].op == 'Const':
            input_names = [input_names[0]]
            weight = input_nodes[1].value.val
            bias = node.attr.get('bias')

        transpose_a = node.attr.get('adj_x', False) or node.attr.get('transpose_a', False)
        transpose_b = node.attr.get('adj_y', False) or node.attr.get('transpose_b', False)
        if len(input_names) == 1 and transpose_b and weight is not None:
            weight = weight.transpose((1, 0))

        n_rows = 0 if weight is None else weight.shape[0]
        n_cols = 0 if weight is None else weight.shape[1]
        builder = self._get_builder()
        layer = builder.add_batched_mat_mul(
            name=node.name,
            input_names=input_names,
            output_name=node.name,
            W=weight,  # (batched_mat_mul requires Cin, Cout)
            weight_matrix_rows=n_rows,
            weight_matrix_columns=n_cols,
            bias=bias,
            transpose_a=transpose_a,
            transpose_b=transpose_b)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_split(self, node):
        # Only handles static splits
        axis = node.attr['split_dim']
        split = node.attr['split']
        split = [size for size in split if size != 0]
        num_splits = len(split)
        has_equal_splits = all([size == split[0] for size in split])
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        # Split output is a tuple. We need to split them into a list of tensors
        output_names = [(node.name + '_' + str(i)) for i in range(num_splits)]
        if node.name in self.op_tensor_map:
            raise ValueError(
                '[SSAConverter] split node %s should not be visited twice.' % node.name)
        self.op_tensor_map[node.name] = output_names

        tensor_id = -1 if node.op == 'Split' else 0
        if has_equal_splits:
            layer = self._get_builder().add_split_nd(
                name=node.name,
                input_name=input_names[tensor_id],
                output_names=output_names,
                axis=axis,
                num_splits=num_splits)
        else:
            layer = self._get_builder().add_split_nd(
                name=node.name,
                input_name=input_names[tensor_id],
                output_names=output_names,
                axis=axis,
                split_sizes=list(split))

        if not has_equal_splits:
            for i, name in enumerate(output_names):
                self.tensor_shapes[name] = self._get_tensor_shape_from_type(node.datatype.T[i])
        else:
            shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_identity(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        layer = self._get_builder().add_activation(
            name=node.name,
            non_linearity='LINEAR',
            input_name=input_names[0],
            output_name=node.name,
            params=(1.0, 0.0))
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_tensorarray_size(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        assert (len(input_names) == 1)

        builder = self._get_builder()
        layer = builder.add_get_shape(
            name=node.name + '_full_shape',
            input_name=input_names[0],
            output_name=node.name + '_full_shape')
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = builder.add_slice_static(
            name=node.name,
            input_name=node.name + '_full_shape',
            output_name=node.name,
            begin_ids=[0],
            end_ids=[1],
            begin_masks=[False],
            end_masks=[False],
            strides=[1])
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_tensorarray_gather(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        assert (len(input_names) == 2)

        layer = self._get_builder().add_gather(
            name=node.name, input_names=input_names[::-1], output_name=node.name, axis=0)

        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_pack(self, node):
        axis = node.attr.get('axis')
        axis = axis if axis else 0
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        if len(input_names) == 1:
            if _is_scalar(input_types[0]):  # skip /identity op in this case
                self.op_tensor_map[node.name] = input_names
            else:
                layer = self._get_builder().add_expand_dims(
                    name=node.name, input_name=input_names[0], output_name=node.name, axes=[0])
        else:
            if all([_is_scalar(input_type) for input_type in input_types]):
                layer = self._get_builder().add_concat_nd(
                    name=node.name, input_names=input_names, output_name=node.name, axis=axis)
            else:
                layer = self._get_builder().add_stack(
                    name=node.name, input_names=input_names, output_name=node.name, axis=axis)

        self.tensor_shapes[node.name] = self._get_tensor_shape_from_type(node.datatype)

    def _convert_unpack(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        output_names = [(node.name + '_' + str(i) + '_') for i in range(len(node.datatype.T))]
        self.op_tensor_map[node.name] = output_names
        num_splits = node.attr['num']
        axis = int(node.attr['axis'])
        interm_output_names = [name + '_unsqueezed_' for name in output_names]
        layer = self._get_builder().add_split_nd(
            name=node.name, input_name=input_names[0], output_names=interm_output_names, axis=axis,
            num_splits=num_splits)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        for in_name, out_name in zip(interm_output_names, output_names):
            layer = self._get_builder().add_squeeze(
                name=out_name, input_name=in_name, output_name=out_name, axes=[0])
            shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_gather(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        # NNSSA: [u'encoder/Variable/read', u'Placeholder', u'encoder/embedding_lookup/axis']
        # CoreML         Given two inputs, 'data' and 'indices', gather the slices of 'data'
        axis = node.attr['axis']
        layer = self._get_builder().add_gather(
            name=node.name, input_names=input_names[0:2], output_name=node.name, axis=axis)

        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_gather_nd(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        layer = self._get_builder().add_gather_nd(
            name=node.name,
            input_names=input_names,
            output_name=node.name
        )
        self.tensor_shapes[node.name] = self._get_tensor_shape_from_type(node.datatype)

    def _convert_scatter_nd(self, node):
        assert len(node.inputs) == 3
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        indices, updates, shape = input_names
        output_shape = input_nodes[2].value.val

        layer = self._get_builder().add_fill_static(
            name=node.name + '_tmp',
            output_name=node.name + '_tmp',
            output_shape=output_shape,
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = self._get_builder().add_scatter_nd(
            name=node.name,
            input_names=[node.name + '_tmp', indices, updates],
            output_name=node.name,
            mode='ADD'
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_unary_square(self, node):
        assert len(node.inputs) == 1
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        layer = self._get_builder().add_elementwise(
            name=node.name, input_names=input_names * 2, output_name=node.name, mode='MULTIPLY')
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_unary_neg(self, node):
        assert len(node.inputs) == 1
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        layer = self._get_builder().add_elementwise(
            name=node.name, input_names=[input_names[0]], output_name=node.name, mode='MULTIPLY', alpha=-1.0)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_conv2d(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        weight = None
        bias = None
        if len(input_names) == 1:
            weight = node.attr.get('W', node.attr.get('W_const'))
            bias = node.attr.get('bias')
        elif len(input_names) == 2:
            input_names = [input_names[0]]
            if input_nodes[1].op == 'Const':
                weight = input_nodes[1].value.val
            bias = node.attr.get('bias')

        if weight is None:
            raise NotImplementedError(
                '[SSAConverter] Dynamic weights in convolution not implemented')

        dilations = node.attr.get('dilations', [1, 1, 1, 1])
        # TF uses SpaceToBatch to implement dilated convolutions
        if any([df != 1 for df in dilations]):
            raise NotImplementedError(
                '[SSAConverter] Dilated Convolution not implemented')

        assert len(weight.shape) == 4, 'Conv2d: weight parameter not rank 4'

        data_format = node.attr.get('data_format', 'NHWC')

        conv_input_name = input_names[0]
        conv_output_name = node.name
        builder = self._get_builder()

        if data_format == 'NHWC' or data_format == 'NHWC_format_inserted':
            stride_height = node.attr.get('strides', [1, 1, 1, 1])[1]
            stride_width = node.attr.get('strides', [1, 1, 1, 1])[2]
        else:
            stride_height = node.attr.get('strides', [1, 1, 1, 1])[-2]
            stride_width = node.attr.get('strides', [1, 1, 1, 1])[-1]

        border_mode = node.attr.get('padding').lower()

        groups = 1
        kernel_height, kernel_width, kernel_channels, output_channels = weight.shape
        if node.op == 'DepthwiseConv2dNative':
            depth_multiplier = weight.shape[3]
            weight = np.reshape(weight,
                (kernel_height, kernel_width, 1, kernel_channels * depth_multiplier))
            output_channels = kernel_channels * depth_multiplier
            groups = kernel_channels
            kernel_channels = 1

        pad_h = node.attr.get('pad_h', [0, 0])
        pad_w = node.attr.get('pad_w', [0, 0])

        builder.add_convolution(
            name=conv_output_name,
            kernel_channels=kernel_channels,
            output_channels=output_channels,
            height=kernel_height,
            width=kernel_width,
            stride_height=stride_height,
            stride_width=stride_width,
            border_mode=border_mode,
            groups=groups,
            W=weight,
            b=bias,
            has_bias=(bias is not None),
            is_deconv=False,
            output_shape=None,
            input_name=conv_input_name,
            output_name=conv_output_name,
            dilation_factors=[1, 1],
            padding_bottom=pad_h[0],
            padding_top=pad_h[1],
            padding_left=pad_w[0],
            padding_right=pad_w[1]
        )

        self.tensor_shapes[node.name] = self._get_tensor_shape_from_type(node.datatype)

    def _convert_pool(self, node, layer_type):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        data_format = node.attr.get('data_format', 'NHWC')
        kernel_sizes = node.attr.get('ksize', [1, 1, 1, 1])
        stride_sizes = node.attr.get('strides', [1, 1, 1, 1])
        padding_type = node.attr.get('padding')
        global_pooling = node.attr.get('global_pooling', False)

        if data_format == 'NHWC' or data_format == 'NHWC_format_inserted':
            kernel_height = kernel_sizes[1]
            kernel_width = kernel_sizes[2]
            stride_height = stride_sizes[1]
            stride_width = stride_sizes[2]
        else:
            kernel_height = kernel_sizes[-2]
            kernel_width = kernel_sizes[-1]
            stride_height = stride_sizes[-2]
            stride_width = stride_sizes[-1]

        self._get_builder().add_pooling(
            name=node.name,
            height=kernel_height,
            width=kernel_width,
            stride_height=stride_height,
            stride_width=stride_width,
            layer_type=layer_type,
            padding_type=padding_type,
            input_name=input_names[0],
            output_name=node.name,
            exclude_pad_area=True,
            is_global=global_pooling
        )

        self.tensor_shapes[node.name] = self._get_tensor_shape_from_type(node.datatype)

    def _convert_maxpool(self, node):
        self._convert_pool(node, 'MAX')

    def _convert_avgpool(self, node):
        self._convert_pool(node, 'AVERAGE')

    def _convert_reshape(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        if _is_scalar(node.datatype) and self._get_tensor_shape_from_type(input_types[0]) == (1,):  # skip/identity op in that case
            self.op_tensor_map[node.name] = [input_names[0]]
        elif self._get_tensor_shape_from_type(input_types[0]) == self._get_tensor_shape_from_type(node.datatype) \
                and sum([i < 0 for i in self._get_tensor_shape_from_type(node.datatype)]) <= 1:
            # in this case reshape is not changing the shape
            self.op_tensor_map[node.name] = [input_names[0]]
        elif (builtins.is_tensor(node.datatype) and
              sum([i < 0 for i in self._get_tensor_shape_from_type(node.datatype)]) <= 1):

            output_shape = self._get_tensor_shape_from_type(node.datatype)
            layer = self._get_builder().add_reshape_static(
                name=node.name,
                input_name=input_names[0],
                output_name=node.name,
                output_shape=output_shape)
            shapes.propagate_single_layer(layer, self.tensor_shapes)
        else:
            layer = self._get_builder().add_reshape_dynamic(
                name=node.name, input_names=input_names, output_name=node.name)

            self.tensor_shapes[node.name] = self._get_tensor_shape_from_type(node.datatype)

    def _convert_argmax(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        axis = node.attr['reduction_indices'][0]
        layer = self._get_builder().add_argmax(
            name=node.name,
            input_name=input_names[0],
            output_name=node.name,
            axis=axis,
            keepdims=node.attr.get("keep_dims", False))
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_argmin(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        axis = node.attr['reduction_indices'][0]
        layer = self._get_builder().add_argmin(
            name=node.name,
            input_name=input_names[0],
            output_name=node.name,
            axis=axis,
            keepdims=node.attr.get("keep_dims", False))
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_reverse(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        reverse_axes = input_nodes[1].value.val
        rank = len(self.tensor_shapes[input_names[0]])
        reverse_dim = [False] * rank
        for axis in reverse_axes:
            reverse_dim[axis] = True

        layer = self._get_builder().add_reverse(
            name=node.name, input_name=input_names[0], output_name=node.name, reverse_dim=reverse_dim)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_expand_dims(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        if _is_scalar(input_types[0]):  # skip/identity op in that case
            self.op_tensor_map[node.name] = [input_names[0]]
        if len(input_names) == 2 and input_nodes[1].value.val is None:
            raise NotImplementedError("[SSAConverter] Cannot handle dynamic expandDims")

        axes = input_nodes[1].value.val
        axes = list(axes) if isinstance(axes, Iterable) else [axes]
        layer = self._get_builder().add_expand_dims(
            name=node.name, input_name=input_names[0], output_name=node.name, axes=axes)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_squeeze(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        axes = node.attr["squeeze_dims"]
        layer = self._get_builder().add_squeeze(
            name=node.name,
            input_name=input_names[0],
            output_name=node.name,
            axes=axes)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_cast(self, node):
        assert len(node.inputs) == 1
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        layer = self._get_builder().add_round(
            name=node.name,
            input_name=input_names[0],
            output_name=node.name)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_reverse_sequence(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        batch_axis = node.attr['batch_dim']
        seq_axis = node.attr['seq_dim']

        layer = self._get_builder().add_reverse_sequence(
            name=node.name,
            input_names=input_names,
            output_name=node.name,
            batch_axis=batch_axis,
            seq_axis=seq_axis)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_embedding(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        weight = None
        if len(input_names) == 1:
            weight = node.attr.get('W')
        elif len(input_names) == 2 and input_nodes[1].op == 'Const':
            weight = input_nodes[1].value.val  # (batch, depth, out_channels)

        if weight is None:
            raise ValueError('[SSAConverter] Unable to handle dynamic embedding')

        out_channels = weight.shape[-1]
        depth = node.attr['depth']
        weight = weight.reshape([depth, out_channels]).transpose((1, 0))

        expanddim_name = node.name + '_expandim_'

        layer = self._get_builder().add_expand_dims(
            name=expanddim_name, input_name=input_names[0], output_name=expanddim_name, axes=[-1])
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = self._get_builder().add_embedding_nd(
            name=node.name,
            input_name=expanddim_name,
            output_name=node.name,
            vocab_size=depth,
            embedding_size=out_channels,
            W=weight)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_tile(self, node):
        assert len(node.inputs) == 2
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        reps = input_nodes[1].value.val
        layer = self._get_builder().add_tile(
            name=node.name,
            input_name=input_names[0],
            output_name=node.name,
            reps=reps
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_lstm_block_cell(self, node):
        assert len(node.inputs) == 5
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        x, w_name, b_name, h_prev, c_prev = input_names

        weight = input_nodes[1].value.val
        bias = input_nodes[2].value.val

        builder = self._get_builder()

        def igfo_to_ifog(data):
            i, g, f, o = np.split(data, 4, axis=-1)
            return np.concatenate([i, f, o, g], axis=-1)

        hidden_size = weight.shape[-1] // 4
        input_size = weight.shape[0] - hidden_size

        W_h_fw = weight[input_size:, :4 * hidden_size]
        W_h_fw = igfo_to_ifog(W_h_fw)
        W_h_fw = np.transpose(W_h_fw, [1, 0])
        W_h_fw = np.ascontiguousarray(W_h_fw)
        W_h_fw = np.split(W_h_fw, 4, axis=0)

        W_x_fw = weight[:input_size, :4 * hidden_size]
        W_x_fw = igfo_to_ifog(W_x_fw)
        W_x_fw = np.transpose(W_x_fw, [1, 0])
        W_x_fw = np.ascontiguousarray(W_x_fw)
        W_x_fw = np.split(W_x_fw, 4, axis=0)

        b_fw = bias[:4 * hidden_size]
        b_fw = igfo_to_ifog(b_fw)
        b_fw = np.split(b_fw, 4, axis=-1)

        forget_bias = node.attr.get('forget_bias')
        has_forget_bias = forget_bias and forget_bias != 0.0
        if has_forget_bias:
            b_fw[1] += forget_bias

        layer = builder.add_expand_dims(
            name=node.name + '_in_expand',
            input_name=x,
            output_name=node.name + '_in_expand',
            axes=[-1, -2]
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = builder.add_expand_dims(
            name=node.name + '_h_prev_expand',
            input_name=h_prev,
            output_name=node.name + '_h_prev_expand',
            axes=[0, -1, -2]
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = builder.add_expand_dims(
            name=node.name + '_c_prev_expand',
            input_name=c_prev,
            output_name=node.name + '_c_prev_expand',
            axes=[0, -1, -2]
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = builder.add_unilstm(
            name=node.name + '_lstm',
            W_h=W_h_fw,
            W_x=W_x_fw,
            b=b_fw,
            hidden_size=hidden_size,
            input_size=input_size,
            input_names=[
                node.name + '_in_expand',
                node.name + '_h_prev_expand',
                node.name + '_c_prev_expand'
            ],
            output_names=[
                node.name + '_lstm_out',
                node.name + '_lstm_h',
                node.name + '_lstm_c',
            ],
            forget_bias=has_forget_bias,
            output_all=True,
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = builder.add_squeeze(
            name=node.name + '_out',
            input_name=node.name + '_lstm_out',
            output_name=node.name + '_out',
            axes=[-1, -2]
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = builder.add_copy(
            name=node.name + '_temp_h',
            input_name=node.name + '_lstm_out',
            output_name=node.name + '_temp_h'
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        # workaround: Core ML LSTM layer outputs the states on last sequence
        layer = builder.add_broadcast_to_like(
            name=node.name + '_temp_c',
            input_names=[node.name + '_lstm_c', node.name + '_lstm_out'],
            output_name=node.name + '_temp_c',
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = builder.add_squeeze(
            name=node.name + '_h',
            input_name=node.name + '_temp_h',
            output_name=node.name + '_h',
            axes=[-1, -2]
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = builder.add_squeeze(
            name=node.name + '_c',
            input_name=node.name + '_temp_c',
            output_name=node.name + '_c',
            axes=[-1, -2]
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        self.op_tensor_map[node.name] = [
            node.name + '_out', node.name + '_h', node.name + '_c'
        ]

    def _convert_constant_pad(self, node):
        # operator Pad has 2 inputs, PadV2 has 3 inputs
        assert len(node.inputs) == 2 or len(node.inputs) == 3
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        constant_value = 0
        if len(node.inputs) == 3:
            constant_value = input_nodes[2].value.val
            if constant_value == -np.inf:
                INT_MIN = - np.iinfo(np.int64).max - 1
                constant_value = np.float(INT_MIN)

            if constant_value == np.inf:
                INT_MAX = np.iinfo(np.int64).max
                constant_value = np.float(INT_MAX)

        # this layer takes at most 2 inputs
        input_names = input_names[:2]
        layer = self._get_builder().add_constant_pad(
            name=node.name,
            input_names=input_names,
            output_name=node.name,
            value=constant_value
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_mirror_pad(self, node):
        assert len(node.inputs) == 2
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        paddings = input_nodes[1].value.val  # rank 4, nhwc
        left, right = paddings[2][0], paddings[2][1]
        top, bottom = paddings[1][0], paddings[1][1]

        if node.attr.get('mode', '').lower() == 'symmetric':
            print('[SSAConverter]Warning: Symmetric MirrorPad is not supported'
                  'but can be approximated with non-symmetric padding in some'
                  'cases. Conversion will continue, but expect some loss'
                  'of model accuracy.')
        builder = self._get_builder()

        layer = builder.add_padding(
            name=node.name,
            left=left,
            right=right,
            top=top,
            bottom=bottom,
            input_name=input_names[0],
            output_name=node.name,
            padding_type='reflection'
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_topk(self, node):
        assert len(node.inputs) == 2
        if node.attr.get('sorted') is False:
            raise NotImplementedError('sorted should be set to True.')

        input_nodes, input_names, input_types = self._get_input_tensors(node)
        k = input_nodes[1].value.val
        output_names = [(node.name + '_' + str(i)) for i in range(2)]
        layer = self._get_builder().add_topk(
            name=node.name,
            input_names=[input_names[0]],
            output_names=output_names,
            k=k,
            axis=-1
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)
        self.op_tensor_map[node.name] = output_names

    def _convert_unary_log_softmax(self, node):
        assert len(node.inputs) == 1
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        axis = -1 if 'axis' not in node.attr else node.attr['axis']

        layer = self._get_builder().add_reduce_logsumexp(
            name=node.name + "_logsumexp",
            input_name=input_names[0],
            output_name=node.name + "_logsumexp",
            axes=[axis],
            keepdims=True,
            reduce_all=False
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = self._get_builder().add_subtract_broadcastable(
            name=node.name,
            input_names=input_names + [node.name + "_logsumexp"],
            output_name=node.name
        )

        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_unary_inverse(self, node):
        assert len(node.inputs) == 1
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        layer = self._get_builder().add_unary(
            name=node.name,
            input_name=input_names[0],
            output_name=node.name,
            mode='inverse'
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_batchnorm(self, node):
        assert len(node.inputs) == 1
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        if 'gamma' not in node.attr or 'beta' not in node.attr:
            raise ValueError('BatchNorm node must have attributes \'gamma\' and \'beta\'')
        gamma = node.attr.get('gamma')
        C = len(gamma)
        beta = node.attr.get('beta')
        mean = node.attr.get('mean', np.zeros((C,)))
        variance = node.attr.get('variance', np.ones((C,)))
        layer = self._get_builder().add_batchnorm(
            name=node.name,
            channels=C,
            gamma=gamma,
            beta=beta,
            mean=mean,
            variance=variance,
            input_name=input_names[0],
            output_name=node.name
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_unary_common(self, node):
        assert len(node.inputs) == 1
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        op = node.op.lower()  # type of the unary operator
        if op in ['sqrt', 'rsqrt', 'exp', 'log', 'abs']:
            layer = self._get_builder().add_unary(
                name=node.name, input_name=input_names[0], output_name=node.name, mode=op)
        else:
            # same function name for TensorFlow and Core ML
            func = getattr(self._get_builder(), 'add_' + op)
            layer = func(name=node.name, input_name=input_names[0], output_name=node.name)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_unary_trigonometric(self, node):
        assert len(node.inputs) == 1
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        op = node.op.lower()  # type of the unary operator
        # assumes TensorFlow and Core ML has same op name
        func = getattr(self._get_builder(), 'add_' + op)
        layer = func(name=node.name, input_name=input_names[0], output_name=node.name)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_unary_activation(self, node):
        assert len(node.inputs) == 1
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        op = node.op.upper()  # type of the unary operator
        params = None
        if op in ['LEAKYRELU']:
            params = ([node.attr['alpha']])
        elif op in ['ELU']:
            params = 1.0
        layer = self._get_builder().add_activation(
            name=node.name,
            input_name=input_names[0],
            output_name=node.name,
            non_linearity=op,
            params=params
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_unary_activation_relu6(self, node):
        assert len(node.inputs) == 1
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        builder = self._get_builder()
        layer = builder.add_activation(
            name=node.name + '_relu',
            input_name=input_names[0],
            output_name=node.name + '_relu',
            non_linearity='RELU',
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = builder.add_clip(
            name=node.name,
            input_name=node.name + '_relu',
            output_name=node.name,
            max_value=6.0
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_gelu(self, node):
        assert len(node.inputs) == 1
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        # Core ML has 3 modes: EXACT, TANH_APPROXIMATION,SIGMOID_APPROXIMATION
        layer = self._get_builder().add_gelu(
            name=node.name,
            input_name=input_names[0],
            output_name=node.name,
            mode='TANH_APPROXIMATION')

        output_shape = self._get_tensor_shape_from_type(node.datatype)
        shapes.propagate_single_layer(layer, self.tensor_shapes,
                                      output_shapes=[output_shape])

    def _convert_reduction(self, node):
        assert len(node.inputs) == 2
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        if len(input_names) == 2:
            axes = np.array(input_nodes[1].value.val).flatten()
            reduction_indices = list(axes) if isinstance(axes, Iterable) else [axes]
        elif 'reduction_indices' in node.attr:
            reduction_indices = node.attr['reduction_indices']
        else:
            reduction_indices = node.attr['axis']

        if 'keep_dims' in node.attr:
            keepdims = node.attr['keep_dims']
        else:
            keepdims = node.attr['keepdims']

        op = node.op.lower()  # type of the unary operator
        if op in ['all', 'any']:
            op = 'prod' if op == 'all' else 'sum'

        func = getattr(self._get_builder(), 'add_reduce_' + op)
        layer = func(
            name=node.name,
            input_name=input_names[0],
            output_name=node.name,
            axes=reduction_indices,
            keepdims=keepdims,
            reduce_all=not reduction_indices
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_resize_bilinear(self, node):
        # In TF, ResizeBilinear requires channel-last image axis order
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        if len(input_names) == 2 and input_nodes[1].op == 'Const':
            target_size = input_nodes[1].value.val
        else:
            raise ValueError('[SSAConverter] Unable to determine target size'
                             'for ResizeBilinear')

        mode = 'STRICT_ALIGN_ENDPOINTS_MODE' if node.attr.get(
            'align_corners', False) else 'UPSAMPLE_MODE'

        builder = self._get_builder()
        layer = builder.add_resize_bilinear(
            name=node.name,
            input_name=input_names[0],
            output_name=node.name,
            target_height=target_size[0],
            target_width=target_size[1],
            mode=mode)

        output_shape = self._get_tensor_shape_from_type(node.datatype)
        shapes.propagate_single_layer(layer, self.tensor_shapes,
                                      output_shapes=[output_shape])

    def _convert_resize_nearest_neighbor(self, node):
        # In TF, ResizeNearestNeighbor requires channel-last image axis order
        # During conversion, NNSSA's output shape should have been modified
        # to NCHW in transform_nhwc_to_nchw()
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        if len(input_names) == 2 and input_nodes[1].op == 'Const':
            target_size = input_nodes[1].value.val
        else:
            raise ValueError('[SSAConverter] Unable to determine target size'
                             'for ResizeNearestNeighbor')
        try:
            input_shape = self._get_tensor_shape_from_type(input_types[0])
        except:
            input_shape = None

        if input_shape is None or len(input_shape) != 4:
            raise ValueError('[SSAConverter] ResizeNearestNeighbor has invalid'
                             'input shape {}'.format(input_shape))

        if target_size[0] < input_shape[2] and target_size[1] < input_shape[3]:
            self._convert_resize_bilinear(node)

        elif target_size[0] > input_shape[2] and target_size[1] > input_shape[3]:
            if (target_size[0] % input_shape[2] > 0 or
                    target_size[1] % input_shape[3] > 0):
                raise ValueError('[SSAConverter] Unsupported fractional'
                                 'nearest-neighbor upsampling')

            scaling_factor_h = int(target_size[0] / input_shape[2])
            scaling_factor_w = int(target_size[1] / input_shape[3])

            if scaling_factor_h <= 0 or scaling_factor_w <= 0:
                raise ValueError('[SSAConverter] Invalid scaling factor.')

            if node.attr.get('align_corners', False) is True:
                raise ValueError('[SSAConverter] CoreML does not support '
                                 'ResizeNearestNeighbor with align_core.')

            builder = self._get_builder()
            layer = builder.add_upsample(
                name=node.name,
                scaling_factor_h=scaling_factor_h,
                scaling_factor_w=scaling_factor_w,
                input_name=input_names[0],
                output_name=node.name,
                mode='NN')

            output_shape = self._get_tensor_shape_from_type(node.datatype)
            shapes.propagate_single_layer(layer, self.tensor_shapes,
                                          output_shapes=[output_shape])
        else:
            raise NotImplementedError("[SSAConverter] Unsupported resizing option.")

    def _convert_layer_normalization(self, node):
        assert len(node.inputs) == 1
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        input_name = input_names[0]
        builder = self._get_builder()
        gamma = node.attr['gamma']
        beta = node.attr['beta']
        axes = node.attr['axes']
        epsilon = node.attr['epsilon']
        input_shape = list(input_types[0].get_shape())

        if (len(input_shape) in [2, 3] and len(axes) == 1 and \
                axes[0] == len(input_shape) - 1):
            # Performance enhancement for some models with layer-norm
            builder.add_reshape_static(name=input_name + '_reshape',
                                       input_name=input_name,
                                       output_name=input_name + '_reshape',
                                       output_shape=input_shape + [1, 1])

            builder.add_mvn(name=input_name + '_mvn',
                            input_name=input_name + '_reshape',
                            output_name=input_name + '_mvn', across_channels=True,
                            normalize_variance=True, epsilon=epsilon)

            builder.add_scale(name=node.name + '_5d',
                              input_name=input_name + '_mvn',
                              output_name=node.name + '_5d', W=gamma, b=beta, has_bias=True,
                              shape_scale=[len(gamma)], shape_bias=[len(beta)])

            builder.add_reshape_static(name=node.name,
                                       input_name=node.name + '_5d',
                                       output_name=node.name,
                                       output_shape=input_shape)

        else:
            # General implementation
            input_shape = input_types[0].get_shape()
            rdims = len(axes)
            normalized_shape = node.datatype.get_shape()[-rdims:]
            if gamma.shape != normalized_shape:
                gamma = np.zeros(normalized_shape) + gamma
            if beta.shape != normalized_shape:
                beta = np.zeros(normalized_shape) + beta

            builder.add_layer_normalization(node.name, input_name, node.name,
                                            normalized_shape, gamma, beta, eps=1e-5)

        self.tensor_shapes[node.name] = self._get_tensor_shape_from_type(
            node.datatype)

    def _convert_binary_broadcastable(self, node):
        assert len(node.inputs) == 2
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        builder = self._get_builder()
        op = node.op.lower()  # type of the unary operator
        compare_greater_ops = {'greater', 'greaterequal'}
        compare_equal_ops = {'equal', 'notequal'}
        compare_less_ops = {'less', 'lessequal'}
        logical_ops = {'logicaland': 'AND', 'logicalor': 'OR'}
        math_ops = {'sub': 'subtract', 'mul': 'multiply', 'realdiv': 'divide',
                    'floordiv': 'floor_div', 'maximum': 'max', 'minimum': 'min',
                    'biasadd': 'add',
                    'pow': 'pow'}
        if op in compare_greater_ops:
            layer = builder.add_greater_than(
                name=node.name,
                input_names=input_names,
                output_name=node.name,
                use_greater_than_equal='equal' in op
            )
        elif op in compare_equal_ops:
            op = 'not_equal' if op == 'notequal' else op
            func = getattr(builder, 'add_' + op)
            layer = func(
                name=node.name,
                input_names=input_names,
                output_name=node.name
            )
        elif op in compare_less_ops:
            layer = builder.add_less_than(
                name=node.name,
                input_names=input_names,
                output_name=node.name,
                use_less_than_equal='equal' in op
            )
        elif op in logical_ops.keys():
            layer = self._get_builder().add_logical(
                name=node.name,
                input_names=input_names,
                output_name=node.name,
                mode=logical_ops[op]
            )
        elif op in math_ops.keys():
            func = getattr(builder, 'add_' + math_ops[op] + '_broadcastable')
            layer = func(
                name=node.name,
                input_names=input_names,
                output_name=node.name
            )
        else:  # same function name for TensorFlow and Core ML
            func = getattr(builder, 'add_' + op + '_broadcastable')
            layer = func(
                name=node.name,
                input_names=input_names,
                output_name=node.name
            )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_fill(self, node):
        assert len(node.inputs) == 2
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        value = input_nodes[1].value.val

        layer = self._get_builder().add_fill_dynamic(name=node.name,
                                                     input_name=input_names[0],
                                                     output_name=node.name,
                                                     value=value)
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_iff(self, node):
        assert len(node.inputs) == 3
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        layer = self._get_builder().add_branch(name=node.name,
                                               input_name=input_names[0])

        ifbranch = NeuralNetworkBuilder(nn_spec=layer.branch.ifBranch,
                                        disable_rank5_shape_mapping=True)

        ifbranch.add_activation(name=node.name + "_if_",
                                non_linearity='LINEAR',
                                input_name=input_names[1],
                                output_name=node.name,
                                params=(1.0, 0.0))

        elsebranch = NeuralNetworkBuilder(nn_spec=layer.branch.elseBranch,
                                          disable_rank5_shape_mapping=True)

        elsebranch.add_activation(name=node.name + "_else_",
                                  non_linearity='LINEAR',
                                  input_name=input_names[2],
                                  output_name=node.name,
                                  params=(1.0, 0.0))

        self.tensor_shapes[node.name] = self._get_tensor_shape_from_type(node.datatype)

    def _convert_reorganize_data(self, node):
        assert len(node.inputs) == 1
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        block_size = node.attr.get('block_size', 2)
        if node.op == 'SpaceToDepth':
            mode = 'SPACE_TO_DEPTH'
        else:  # node.op == 'DepthToSpace':
            mode = 'DEPTH_TO_SPACE'

        builder = self._get_builder()

        layer = builder.add_reorganize_data(
            name=node.name,
            input_name=input_names[0],
            output_name=node.name,
            mode=mode,
            block_size=block_size
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_space_to_batch_nd(self, node):
        assert len(node.inputs) == 3
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        block_shape = input_nodes[1].value.val
        if len(block_shape.flatten()) != 2 or block_shape[0] != block_shape[1]:
            raise NotImplementedError('non-equal block shape is not yet supported')
        paddings = input_nodes[2].value.val
        needs_paddings = any(paddings.flatten())
        builder = self._get_builder()

        layer = builder.add_transpose(
            name=node.name + '_transpose1',
            input_name=input_names[0],
            output_name=node.name + '_transpose1',
            axes=[3, 0, 1, 2]
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        if needs_paddings:
            left, right = paddings[1][0], paddings[1][1]
            top, bottom = paddings[0][0], paddings[0][1]
            layer = builder.add_padding(
                name=node.name + '_padding',
                left=left,
                right=right,
                top=top,
                bottom=bottom,
                input_name=node.name + '_transpose1',
                output_name=node.name + '_padding'
            )
            shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = builder.add_reorganize_data(
            name=node.name + '_reorganize',
            input_name=node.name + '_transpose1' if not needs_paddings else node.name + '_padding',
            output_name=node.name + '_reorganize',
            mode='space_to_depth'.upper(),
            block_size=block_shape[0]
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = builder.add_transpose(
            name=node.name,
            input_name=node.name + '_reorganize',
            output_name=node.name,
            axes=[1, 2, 3, 0]
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_batch_to_space_nd(self, node):
        assert len(node.inputs) == 3
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        block_shape = input_nodes[1].value.val
        if block_shape[0] != block_shape[1]:
            raise NotImplementedError('non-equal block shape is not yet supported')
        crops = input_nodes[2].value.val
        needs_cropping = any(crops.flatten())

        builder = self._get_builder()

        layer = builder.add_transpose(
            name=node.name + '_transpose1',
            input_name=input_names[0],
            output_name=node.name + '_transpose1',
            axes=[3, 0, 1, 2]
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = builder.add_reorganize_data(
            name=node.name + '_reorganize',
            input_name=node.name + '_transpose1',
            output_name=node.name + '_reorganize',
            mode='depth_to_space'.upper(),
            block_size=block_shape[0]
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

        if needs_cropping:
            left, right = crops[1][0], crops[1][1]
            top, bottom = crops[0][0], crops[0][1]
            layer = builder.add_crop(
                name=node.name + '_cropping',
                left=left,
                right=right,
                top=top,
                bottom=bottom,
                offset=0,
                input_names=[node.name + '_reorganize'],
                output_name=node.name + '_cropping'
            )
            shapes.propagate_single_layer(layer, self.tensor_shapes)

        layer = builder.add_transpose(
            name=node.name,
            input_name=node.name + '_reorganize' if not needs_cropping else node.name + '_cropping',
            output_name=node.name,
            axes=[1, 2, 3, 0]
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_conv2d_transpose(self, node):
        assert len(node.inputs) == 3
        input_nodes, input_names, input_types = self._get_input_tensors(node)

        input_name = input_names[2]
        weight = input_nodes[1].value.val
        bias = node.attr.get('bias')

        strides = node.attr.get('strides')
        border_mode = node.attr.get('padding').lower()
        stride_height = strides[1]
        stride_width = strides[2]
        input_shape = self.tensor_shapes[input_name]
        output_shape = self.tensor_shapes[input_names[0]]

        kernel_channels = input_shape[1]
        output_channels = output_shape[-1]

        self._get_builder().add_convolution(
            name=node.name,
            kernel_channels=kernel_channels,
            output_channels=output_channels,
            height=weight.shape[0],
            width=weight.shape[1],
            stride_height=stride_height,
            stride_width=stride_width,
            border_mode=border_mode,
            groups=1,
            W=np.transpose(weight, (0, 1, 3, 2)),
            b=bias,
            has_bias=(bias is not None),
            is_deconv=True,
            output_shape=None,
            input_name=input_name,
            output_name=node.name
        )

        self.tensor_shapes[node.name] = self._get_tensor_shape_from_type(node.datatype)

    def _convert_lrn(self, node):
        input_nodes, input_names, input_types = self._get_input_tensors(node)
        alpha = node.attr.get('alpha')
        beta = node.attr.get('beta')
        bias = node.attr.get('bias')
        depth_radius = node.attr.get('depth_radius')
        n_channels = self._get_tensor_shape_from_type(input_types[-1])[-1]
        if node.attr.get('data_format') == 'NHWC_format_inserted':
            n_channels = self._get_tensor_shape_from_type(input_types[-1])[1]
        layer = self._get_builder().add_lrn(
            name=node.name,
            input_name=input_names[0],
            output_name=node.name,
            alpha=alpha * n_channels,
            beta=beta,
            local_size=depth_radius,
            k=bias
        )
        shapes.propagate_single_layer(layer, self.tensor_shapes)

    def _convert_clip(self, node):

        input_nodes, input_names, input_types = self._get_input_tensors(node)

        min_value = input_nodes[1].value.val
        max_value = input_nodes[2].value.val

        layer = self._get_builder().add_clip(name = node.name,
                                             input_name=input_names[0],
                                             output_name=node.name,
                                             min_value=min_value,
                                             max_value=max_value)

        self.tensor_shapes[node.name] = self._get_tensor_shape_from_type(node.datatype)
