# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import traceback
import numpy as np
import sympy as sm
import sys, math

PY3 = False
if sys.version_info >= (3, 0):
    PY3 = True

from ...commons import builtins
from ...commons.symbolic import *

short_var_name_cache = {}


def get_conv_outdim(in_dim, ks, stride, dl, padding_type):
    try:
        if padding_type == 'VALID':
            ks_dilated = (ks - 1) * dl + 1
            return (in_dim - ks_dilated) / stride + 1
        elif padding_type == 'SAME':
            return math.ceil(in_dim * 1.0 / stride)
        else:
            raise ValueError('[TypeInference] Unrecognized padding type.')
    except Exception as e:
        raise ValueError('[TypeInference] Error fetching padding values: {}'.format(e))


def get_short_var_name(name):
    if name in short_var_name_cache:
        return short_var_name_cache[name]
    else:
        shortname = 's_' + str(len(short_var_name_cache))
        short_var_name_cache[name] = shortname
        return shortname


def replace_neg_1_with_symbolic(val, name):
    for i in range(len(val)):
        if np.isscalar(val[i]) and val[i] == -1:
            val[i] = sm.Symbol(get_short_var_name(name + '_' + str(i)), positive=True)
    return val


def make_symbol(name):
    return sm.Symbol(get_short_var_name(name), positive=True)


class TypeInferenceVisitor(object):
    def __init__(self, graph, whole_ssa):
        # the whole ssa is needed to propagation function calls
        self.op_rules = {}
        self.gdict = graph
        self.whole_ssa = whole_ssa
        self.visited = {}

    def visit(self, node):
        # make sure node is a ParsedNode
        from ...nnssa import ParsedNode
        if not isinstance(node, ParsedNode):
            node = self.gdict[node]

        # do we already know the answer?
        if node.datatype is not None:
            # if it is a fully specified type, we just return it
            # if we seen it this round, we return it
            # otherwise we recurse
            if not builtins.is_tensor(node.datatype) or \
                    builtins.tensor_has_complete_shape(node.datatype) or \
                    node.name in self.visited:
                return node.datatype
        # look for the op's visit method
        method = 'visit_' + node.op
        visitor = getattr(self, method, None)
        if visitor is None:
            print('WARNING [TypeInference]: Op {} not implemented. Inferring shape from node attribute!'.format(node.op))
            visitor = self._get_type_from_attr

        # find the type of the node
        ret = None
        try:
            ret = visitor(node)
        except Exception as e:
            print('[TypeInference] Failed to infer type of node "%s" (%s)' % (node.name, node.op))
            print(e)
            print(traceback.format_exc())

        if ret is not None:
            self.visited[node.name] = 1
            node.datatype = ret
        else:
            print('[TypeInference] Unable to infer type of node "%s" (%s)' % (node.name, node.op))
        return ret

    def visit_all(self):
        for i in self.gdict:
            self.visit(self.gdict[i])

    def _get_type_from_attr(self, node):
        if node.datatype is not None:
            return node.datatype

        node.parse_from_attr()
        if builtins.is_tensor(node.datatype):
            s = list(node.datatype.get_shape())
            for i in range(len(s)):
                if s[i] == -1:
                    s[i] = make_symbol(node.name + '_' + str(i))
            node.datatype = builtins.tensor(node.datatype.get_primitive(), tuple(s))
        return node.datatype

    def match_shape(self, shapea, shapeb):
        if len(shapea) != len(shapeb):
            return False
        for idx in range(len(shapea)):
            if shapea[idx] != shapeb[idx] and shapea[idx] > 0 and shapeb[idx] > 0:
                return False
        return True

    def strict_shape(self, typea, typeb):
        shape = list(typea.get_shape())
        shapeb = typeb.get_shape()
        for idx in range(len(shape)):
            if is_symbolic_or_unknown(shape[idx]):
                shape[idx] = shapeb[idx]
        return builtins.tensor(typea.T[0], tuple(shape))

    # some common patterns
    def visit_unary(self, node):
        return self.visit(node.inputs[0])

    def all_inputs_have_values(self, node):
        return all(self.gdict[i].attr['symbolic_value'] is not None for i in node.inputs)

    def get_all_input_values(self, node):
        ret = []
        for i in node.inputs:
            if self.gdict[i].attr['symbolic_value'] is not None:
                ret.append(self.gdict[i].attr['symbolic_value'].val)
            else:
                ret.append(None)
        return ret

    def visit_elementwiseBinary(self, node):
        # https://www.tensorflow.org/api_docs/python/tf/add
        # returns same shape as "x" for "x+y"
        assert (len(node.inputs) == 2)
        rettype1 = self.visit(node.inputs[0])
        rettype2 = self.visit(node.inputs[1])
        good, newtype = builtins.is_tensor_and_is_compatible(rettype1, rettype2)
        if good:
            rettype = newtype
        else:
            rettype = rettype1
        if self.gdict[node.inputs[0]].attr['symbolic_value'] is not None and self.gdict[
            node.inputs[1]].attr['symbolic_value'] is not None:
            if node.op == 'Add':
                node.attr['symbolic_value'] = rettype()
                try:
                    node.attr['symbolic_value'].val = self.gdict[
                                                          node.inputs[0]].attr['symbolic_value'].val + self.gdict[
                                                          node.inputs[1]].attr['symbolic_value'].val
                except:
                    pass
            elif node.op == 'Sub':
                node.attr['symbolic_value'] = rettype()
                node.attr['symbolic_value'].val = self.gdict[node.inputs[0]].attr[
                                                      'symbolic_value'].val - self.gdict[node.inputs[1]].attr['symbolic_value'].val
            elif node.op == 'Mul':
                node.attr['symbolic_value'] = rettype()
                node.attr['symbolic_value'].val = self.gdict[node.inputs[0]].attr[
                                                      'symbolic_value'].val * self.gdict[node.inputs[1]].attr['symbolic_value'].val
            elif node.op == 'FloorMod':
                node.attr['symbolic_value'] = rettype()
                node.attr['symbolic_value'].val = self.gdict[node.inputs[0]].attr[
                                                      'symbolic_value'].val % self.gdict[node.inputs[1]].attr['symbolic_value'].val
            elif node.op == 'FloorDiv':
                node.attr['symbolic_value'] = rettype()
                node.attr['symbolic_value'].val = self.gdict[node.inputs[0]].attr[
                                                      'symbolic_value'].val // self.gdict[node.inputs[1]].attr['symbolic_value'].val
            elif node.op == 'RealDiv':
                node.attr['symbolic_value'] = rettype()
                node.attr['symbolic_value'].val = self.gdict[node.inputs[0]].attr[
                                                      'symbolic_value'].val / self.gdict[node.inputs[1]].attr['symbolic_value'].val
            elif node.op == 'Maximum':
                node.attr['symbolic_value'] = rettype()
                node.attr['symbolic_value'].val = sm.functions.Max(self.gdict[node.inputs[0]].attr['symbolic_value'].val,
                                                                   self.gdict[node.inputs[1]].attr['symbolic_value'].val)
            elif node.op == 'Equal':
                node.attr['symbolic_value'] = rettype()
                node.attr['symbolic_value'].val = (vala.val == valb.val)
            elif node.op == 'NotEqual':
                node.attr['symbolic_value'] = rettype()
                node.attr['symbolic_value'].val = (vala.val != valb.val)
        return rettype

    def visit_reduction_op(self, node):
        typea = self.visit(node.inputs[0])
        typeb = self.visit(node.inputs[1])
        reduction_indices = self.gdict[node.inputs[1]].attr['symbolic_value']
        if typea is None:
            return None
        if reduction_indices is None:
            raise TypeError(
                "Cannot infer shape of Prod because we cannot infer the value of reduction_indices")
        reduction_indices = reduction_indices.val
        # the reduction_idx node can be a scalar
        if not builtins.is_tensor(typeb):
            reduction_indices = [reduction_indices]
        keepdims = node.attr.get('keep_dims', False)
        reduced_shape = list(typea.get_shape())
        if len(reduction_indices) == 0:
            reduction_indices = list(range(len(reduced_shape)))
        if keepdims:
            for i in reduction_indices:
                reduced_shape[i] = 1
        else:
            # sort reverse so we can delete shape elements it back to front
            reduction_indices = sorted(reduction_indices)[::-1]
            for i in reduction_indices:
                reduced_shape.pop(i)
        if len(reduced_shape) == 0:
            rettype = typea.get_primitive()
        else:
            rettype = builtins.tensor(typea.get_primitive(), reduced_shape)
        node.attr['reduction_indices'] = reduction_indices
        node.attr['keep_dims'] = keepdims

        if 'symbolic_value' in self.gdict[node.inputs[0]].attr and \
                self.gdict[node.inputs[0]].attr['symbolic_value'] is not None and \
                'symbolic_value' in self.gdict[node.inputs[1]].attr and \
                self.gdict[node.inputs[1]].attr['symbolic_value'] is not None:
            if node.op == 'Prod':
                method = 'prod'
            elif node.op == 'Mean':
                method = 'mean'
            elif node.op == 'Sum':
                method = 'sum'
            elif node.op == 'ArgMax':
                method = 'argmax'
            elif node.op == 'Max':
                method = 'amax'
            elif node.op == 'Min':
                method = 'amin'
            elif node.op == 'Any':
                method = 'any'
            elif node.op == 'All':
                method = 'all'
            np_method = getattr(np, method, None)
            if np_method is not None:
                try:
                    values = self.gdict[node.inputs[0]].attr['symbolic_value'].val
                    axis = self.gdict[node.inputs[1]].attr['symbolic_value'].val
                    if not np.isscalar(axis) and len(axis) == 1:
                        axis = axis[0]
                    val = np_method(values, axis=axis)
                    node.attr['symbolic_value'] = rettype()
                    if builtins.is_tensor(rettype) and np.is_scalar(val):
                        val = np.array([val])
                    node.attr['symbolic_value'].val = val
                except:
                    pass

        return rettype

    def visit_broadcast_op(self, node):
        # this is broadcast mul
        assert (len(node.inputs) == 2)
        typea = self.visit(node.inputs[0])
        typeb = self.visit(node.inputs[1])
        if typea is not None and typeb is not None:
            if builtins.is_tensor(typea):
                if builtins.is_tensor(typeb):
                    shapea = list(typea.get_shape())
                    shapeb = list(typeb.get_shape())
                    if len(shapea) < len(shapeb):
                        shapea = ([1] * (len(shapeb) - len(shapea))) + shapea
                    if len(shapeb) < len(shapea):
                        shapeb = ([1] * (len(shapea) - len(shapeb))) + shapeb
                    # get loosest shape
                    retshape = []
                    for i in range(len(shapea)):
                        if shapea[i] == 1:
                            retshape.append(shapeb[i])
                        elif shapeb[i] == 1:
                            retshape.append(shapea[i])
                        elif not is_symbolic_or_unknown(shapeb[i]) and shapeb[i] > 1:
                            retshape.append(shapeb[i])
                        elif not is_symbolic_or_unknown(shapea[i]) and shapea[i] > 1:
                            retshape.append(shapea[i])
                        elif is_symbolic_or_unknown(shapea[i]) or is_symbolic_or_unknown(shapeb[i]):
                            retshape.append(sm.functions.Max(shapea[i], shapeb[i]))
                        else:
                            assert (shapea[i] == shapeb[i])
                            retshape.append(shapea[i])
                    self.visit_elementwiseBinary(node)
                    return builtins.tensor(typea.get_primitive(), retshape)
                else:
                    self.visit_elementwiseBinary(node)
                    return typea
            elif builtins.is_tensor(typeb):
                return typeb
            else:
                # both typea and typeb are not tensors
                return self.visit_elementwiseBinary(node)
        else:
            return self._get_type_from_attr(node)

    # The main visitors

    def visit_get_tuple(self, node):  # DO NOT PROPAGATE TYPE INFERENCE ACROSS FUNCTIONS
        assert (len(node.inputs) == 1)
        parent_type = self.visit(node.inputs[0])
        self.propagate_tensor_array(node)
        # parent_type should be an instance of tuple
        if parent_type is None:
            return None
        assert (builtins.is_tuple(parent_type))
        parent_val = self.gdict[node.inputs[0]].attr['symbolic_value']
        if parent_val is not None:
            self.gdict[node.name].attr['symbolic_value'] = parent_val[node.attr['index']]

        return parent_type.T[node.attr["index"]]

    def visit_Identity(self, node):
        ret = self.visit_unary(node)
        node.attr['symbolic_value'] = self.gdict[node.inputs[0]].attr['symbolic_value']
        if 'tensorarray_source' in self.gdict[node.inputs[0]].attr:
            node.attr['tensorarray_source'] = self.gdict[node.inputs[0]].attr['tensorarray_source']
        return ret

    def visit_ZerosLike(self, node):
        return self.visit_unary(node)

    def visit_Print(self, node):
        # this is just identity
        node.op = 'Identity'
        return self.visit_unary(node)

    def visit_Log(self, node):
        ret = self.visit_unary(node)
        return ret

    def visit_Add(self, node):
        return self.visit_broadcast_op(node)

    def visit_Maximum(self, node):
        return self.visit_broadcast_op(node)

    def visit_Minimum(self, node):
        return self.visit_broadcast_op(node)

    def visit_LogicalOr(self, node):
        return self.visit_broadcast_op(node)

    def visit_LogicalAnd(self, node):
        return self.visit_broadcast_op(node)

    def visit_LogicalNot(self, node):
        return self.visit(node.inputs[0])

    def visit_All(self, node):
        return self.visit_reduction_op(node)

    def visit_Any(self, node):
        return self.visit_reduction_op(node)

    def visit_ArgMax(self, node):
        return self.visit_reduction_op(node)

    def visit_ArgMin(self, node):
        return self.visit_reduction_op(node)

    def visit_Prod(self, node):
        return self.visit_reduction_op(node)

    def visit_Assign(self, node):
        assert (len(node.inputs) == 2)
        return self.visit(node.inputs[1])

    def visit_Assert(self, node):
        pass

    def visit_BiasAdd(self, node):
        return self.visit_broadcast_op(node)

    def visit_Cast(self, node):
        assert (len(node.inputs) == 1)
        input_type = self.visit(node.inputs[0])
        value = self.gdict[node.inputs[0]].attr['symbolic_value']
        if builtins.is_tensor(input_type):
            rettype = builtins.tensor(node.attr['DstT'], input_type.get_shape())
        else:
            rettype =  node.attr['DstT']

        if value is not None:
            node.attr['symbolic_value'] = rettype()
            node.attr['symbolic_value'].val = value.val
        return rettype

    def visit_Concat(self, node):
        return self.visit_ConcatV2(node, is_v2=False)

    def visit_ConcatV2(self, node, is_v2=True):
        # Concat takes two tensors and a "axis to be concated"
        # get most specific type of all the concated variables
        if is_v2:
            axis_node = node.inputs[-1]
        else:
            axis_node = node.inputs[0]
        self.visit(axis_node)
        if self.gdict[axis_node].attr['symbolic_value'] is not None:
            concat_axis = self.gdict[axis_node].attr['symbolic_value'].val
            # visit each the inputs
            if is_v2:
                input_types = [self.visit(inp) for inp in node.inputs[:-1]]
            else:
                input_types = [self.visit(inp) for inp in node.inputs[1:]]
            rettype = input_types[0]
            # find the most specific tensor shape
            for t in input_types[1:]:
                good, newtype = builtins.is_tensor_and_is_compatible(rettype, t)
                if good:
                    rettype = newtype

            if not builtins.is_tensor(rettype):
                return None

            # find the new axis shape
            new_axis_shape = 0
            for t in input_types:
                if builtins.is_tensor(t):
                    if len(t.get_shape()) > concat_axis:
                        taxis = t.get_shape()[concat_axis]
                        if taxis == -1:
                            new_axis_shape = make_symbol(node.name + '_new_axis')
                            break
                        else:
                            new_axis_shape += taxis
                    else:
                        new_axis_shape = make_symbol(node.name + '_new_axis')
                        break
                else:
                    new_axis_shape = make_symbol(node.name + '_new_axis')
                    break
            retshape = list(rettype.get_shape())
            retshape[concat_axis] = new_axis_shape

            rettype = builtins.tensor(rettype.get_primitive(), retshape)

            if self.all_inputs_have_values(node):
                if is_v2:
                    inputs = self.get_all_input_values(node)[:-1]
                else:
                    inputs = self.get_all_input_values(node)[1:]
                for i in range(len(inputs)):
                    if isscalar(inputs[i]):
                        inputs[i] = np.array(inputs[i])
                val = np.concatenate(inputs, axis=concat_axis)
                node.attr['symbolic_value'] = rettype()
                node.attr['symbolic_value'].val = val
            return rettype
        else:
            return None

    def visit_Const(self, node):
        assert (len(node.inputs) == 0)
        node.attr['symbolic_value'] = node.value
        if node.datatype is not None:
            return node.datatype
        return self._get_type_from_attr(node)

    def visit_Conv2D(self, node):

        output_shapes = node.attr.get('_output_shapes')
        if output_shapes is None or len(output_shapes) == 0:
            return self._get_type_from_attr(node)

        input_type = self.visit(node.inputs[0])
        filter_type = self.visit(node.inputs[1])

        if all([dim > 0 for dim in output_shapes[0]]):
            return builtins.tensor(input_type.get_primitive(), tuple(output_shapes[0]))

        if input_type is not None and filter_type is not None:
            inshape = input_type.get_shape()
            filtshape = filter_type.get_shape()
            assert len(inshape) == 4
            assert len(filtshape) == 4
            strides = node.attr['strides']
            padding = node.attr['padding']
            dilations = node.attr['dilations']
            retshape = output_shapes[0]
            if node.attr['data_format'] == 'NHWC':

                retshape[1] = get_conv_outdim(inshape[1], filtshape[0],
                                              strides[1], dilations[1], padding)
                retshape[2] = get_conv_outdim(inshape[2], filtshape[1],
                                              strides[2], dilations[2], padding)
            else:
                retshape[2] = get_conv_outdim(inshape[2], filtshape[0],
                                              strides[2], dilations[2], padding)
                retshape[3] = get_conv_outdim(inshape[3], filtshape[1],
                                              strides[3], dilations[3], padding)

            return builtins.tensor(input_type.get_primitive(), tuple(retshape))

        return self._get_type_from_attr(node)

    def visit_DepthwiseConv2dNative(self, node):
        return self.visit_Conv2D(node)

    def visit_Conv2DBackpropInput(self, node):
        attr_output_type = self._get_type_from_attr(node)

        if attr_output_type is not None:
            return attr_output_type
        else:
            raise ValueError("[Type Inference] Conv2DBackpropInput type "
                             "inference case not handled")

    def visit_ResizeBilinear(self, node):
        return self._get_type_from_attr(node)

    def visit_ResizeNearestNeighbor(self, node):
        return self._get_type_from_attr(node)

    def visit_pooling(self, node):
        input_type = self.visit(node.inputs[0])
        ksize = node.attr['ksize']
        if input_type is not None:
            # we implement shape inference for a simple case
            if node.attr['data_format'] == 'NHWC' and \
                    all(d == 1 for d in node.attr['strides']):
                if node.attr['padding'] == 'VALID':
                    inshape = input_type.get_shape()
                    retshape = []
                    filtshape = list(inshape[:])
                    if isinstance(ksize, list) and len(ksize) == 4:
                        filtshape[1] = inshape[1] + 1 - ksize[1]
                        filtshape[2] = inshape[2] + 1 - ksize[2]
                    elif isinstance(ksize, list) and len(ksize) == 2:
                        filtshape[1] = inshape[1] + 1 - ksize[0]
                        filtshape[2] = inshape[2] + 1 - ksize[1]
                    else:
                        filtshape[1] = inshape[1] - ksize + 1
                        filtshape[2] = inshape[2] - ksize + 1
                    return builtins.tensor(input_type.get_primitive(), tuple(filtshape))
                elif node.attr['padding'] == 'SAME':
                    return input_type
        return self._get_type_from_attr(node)

    def visit_MaxPool(self, node):
        return self.visit_pooling(node)

    def visit_AvgPool(self, node):
        return self.visit_pooling(node)

    def visit_Equal(self, node):
        return self.visit_broadcast_op(node)

    def visit_NotEqual(self, node):
        return self.visit_broadcast_op(node)

    def visit_ExpandDims(self, node):
        assert (len(node.inputs) == 2)
        typea = self.visit(node.inputs[0])
        if not builtins.is_tensor(typea):
            typea = builtins.tensor(typea, (1,))
            shape = []
        else:
            shape = list(typea.get_shape())  # input[0] should be a tensor.
        if self.gdict[node.inputs[1]].attr['symbolic_value'].val < 0:
            cut = len(shape) + int(self.gdict[node.inputs[1]].attr['symbolic_value'].val) + 1
        else:
            cut = int(self.gdict[node.inputs[1]].attr['symbolic_value'].val)
        shape = shape[0:cut] + [1] + shape[cut:]
        rettype = builtins.tensor(typea.get_primitive(), tuple(shape))
        input_val = self.gdict[node.inputs[0]].attr['symbolic_value']
        if input_val is not None:
            input_val = np.array(input_val.val).reshape(shape)
            node.attr['symbolic_value'] = rettype()
            node.attr['symbolic_value'].val = input_val
        return rettype

    def visit_Fill(self, node):
        assert (len(node.inputs) == 2)
        # input[0] should be the shape, input[1] should be
        # the value that would be in the tensor(shape=input[0])
        typea = self.visit(node.inputs[0])
        typeb = self.visit(node.inputs[1])
        if self.gdict[node.inputs[0]].attr['symbolic_value'] is not None:
            shape = tuple(self.gdict[node.inputs[0]].attr['symbolic_value'].val.flatten())
            rettype = builtins.tensor(typeb, shape)

            fill_value = self.gdict[node.inputs[1]].attr['symbolic_value']
            if fill_value is not None and not any_symbolic_or_unknown(shape):
                retval = rettype()
                retval.val = np.ones(shape) * fill_value.val
                node.attr['symbolic_value'] = retval
            return rettype
        else:
            # shape unknown.
            # I should be able to derive a rank
            shape = tuple(make_symbol(node.name + str(i)) for i in range(typea.get_shape()[0]))
            rettype = builtins.tensor(typeb, shape)
            return rettype

    def visit_RandomUniform(self, node):
        assert (len(node.inputs) == 1)
        # input[0] is the shape
        # the value that would be in the tensor(shape=input[0])
        typea = self.visit(node.inputs[0])
        if self.gdict[node.inputs[0]].attr['symbolic_value'] is not None:
            shape = tuple(self.gdict[node.inputs[0]].attr['symbolic_value'].val.flatten())
            rettype = builtins.tensor(node.attr['dtype'], shape)
            return rettype
        else:
            # shape unknown.
            # I should be able to derive a rank
            shape = tuple(make_symbol(node.name + str(i)) for i in range(len(typea.get_shape())))
            rettype = builtins.tensor(node.attr['dtype'], shape)
            return rettype

    def visit_FloorMod(self, node):
        return self.visit_broadcast_op(node)

    def visit_Pow(self, node):
        return self.visit_broadcast_op(node)

    def visit_function(self, node):
        pass

    def visit_function_entry(self, node):
        pass

    def visit_Gather(self, node):

        params_type = self.visit(node.inputs[0])
        indices_type = self.visit(node.inputs[1])
        axis_value = 0
        if len(node.inputs) == 3:
            axis = self.visit(node.inputs[2])
            axis_value = self.gdict[node.inputs[2]].attr['symbolic_value'].val
        node.attr['axis'] = axis_value
        if params_type is None or indices_type is None:
            return None

        params_shape = list(params_type.get_shape())
        if not builtins.is_tensor(indices_type):
            indices_shape = []
        else:
            indices_shape = list(indices_type.get_shape())
            if len(indices_shape) == 0:
                indices_shape = [1]

        retshape = params_shape[:axis_value] + indices_shape + params_shape[axis_value + 1:]
        if len(indices_shape) == 0 and len(retshape) == 0:
            rettype = params_type.get_primitive()
        else:
            rettype = builtins.tensor(params_type.get_primitive(), retshape)

        if self.gdict[node.inputs[0]].attr['symbolic_value'] is not None and \
                self.gdict[node.inputs[1]].attr['symbolic_value'] is not None and \
                axis_value is not None:
            params_val = self.gdict[node.inputs[0]].attr['symbolic_value'].val
            indices_val = self.gdict[node.inputs[1]].attr['symbolic_value'].val
            retval = np.take(params_val, indices_val, axis=axis_value)
            node.attr['symbolic_value'] = rettype()
            node.attr['symbolic_value'].val = retval
        return rettype

    def visit_GatherV2(self, node):
        node.op = 'Gather'
        return self.visit_Gather(node)

    def visit_GatherNd(self, node):
        params_type = self.visit(node.inputs[0])
        indices_type = self.visit(node.inputs[1])
        if params_type is None or indices_type is None:
            return None

        indices_shape = []
        if not builtins.is_tensor(indices_type):
            indices_shape = []
        else:
            indices_shape = list(indices_type.get_shape())
        params_shape = list(params_type.get_shape())
        retshape = indices_shape[:-1] + params_shape[indices_shape[-1]:]
        rettype = builtins.tensor(params_type.get_primitive(), retshape)

        return rettype

    def visit_ScatterNd(self, node):
        indices_type = self.visit(node.inputs[0])
        updates_type = self.visit(node.inputs[1])
        shapes_type = self.visit(node.inputs[2])
        if updates_type is None or shapes_type is None:
            return None

        retshape = []
        if 'symbolic_value' in self.gdict[node.inputs[2]].attr:
            size = list(self.gdict[node.inputs[2]].attr['symbolic_value'].val)
            for i in range(len(size)):
                if is_symbolic_or_unknown(size[i]):
                    retshape.append(make_symbol(node.name + '_' + str(i)))
                else:
                    retshape.append(size[i])
            if len(retshape) == 0:
                rettype = input_type.get_primitive()
            else:
                rettype = builtins.tensor(updates_type.get_primitive(), retshape)

        rettype = builtins.tensor(updates_type.get_primitive(), retshape)

        return rettype

    def visit_GatherTree(self, node):
        # TODO: To implement?
        return self._get_type_from_attr(node)

    def visit_GreaterEqual(self, node):
        return self.visit_broadcast_op(node)

    def visit_Greater(self, node):
        return self.visit_broadcast_op(node)

    def visit_Less(self, node):
        return self.visit_broadcast_op(node)

    def visit_LessEqual(self, node):
        return self.visit_broadcast_op(node)

    def visit_make_tuple(self, node):
        types = [self.visit(i) for i in node.inputs]
        self.propagate_tensor_array(node)

        if any([t is None for t in types]):
            print("make_tuple at %s has an unknown type %s", (node.name, str(types)))
        types = [t if t is not None else builtins.unknown for t in types]
        return builtins.tuple(types)

    def visit_BatchMatMul(self, node):
        return self.visit_MatMul(node)

    def visit_BatchMatMulV2(self, node):
        return self.visit_MatMul(node)

    def visit_MatMul(self, node):
        assert (len(node.inputs) == 2)
        # this handles the parameters from both MatMul and BatchMatMul
        transpose_a = node.attr.get('adj_x', False) or node.attr.get('transpose_a', False)
        transpose_b = node.attr.get('adj_y', False) or node.attr.get('transpose_b', False)
        typea = self.visit(node.inputs[0])
        typeb = self.visit(node.inputs[1])
        if typea is not None and typeb is not None:
            mata = self.gdict[node.inputs[0]]
            matb = self.gdict[node.inputs[1]]
            mata_shape = mata.datatype.get_shape()
            matb_shape = matb.datatype.get_shape()
            if transpose_a:
                mata_shape = list(mata_shape)
                mata_shape[-1], mata_shape[-2] = mata_shape[-2], mata_shape[-1]
                mata_shape = tuple(mata_shape)
            if transpose_b:
                matb_shape = list(matb_shape)
                matb_shape[-1], matb_shape[-2] = matb_shape[-2], matb_shape[-1]
                matb_shape = tuple(matb_shape)
            if len(mata_shape) != len(matb_shape):
                return None

            assert (mata_shape[-1] == matb_shape[-2] or \
                    is_symbolic_or_unknown(mata_shape[-1]) or \
                    is_symbolic_or_unknown(matb_shape[-2]))
            shape = list(mata_shape)
            shape[-1] = matb_shape[-1]
            if len(shape) > 2:
                node.op = 'BatchMatMul'
            return builtins.tensor(typea.get_primitive(), tuple(shape))
        return self._get_type_from_attr(node)

    def visit_Mul(self, node):
        return self.visit_broadcast_op(node)

    def visit_Neg(self, node):
        inputtype = self.visit_unary(node)
        input = self.gdict[node.inputs[0]]
        if input.attr['symbolic_value'] is not None:
            node['symbolic_value'] = inputtype()
            node['symbolic_value'].val = -input.attr['symbolic_value'].val
        return inputtype

    def visit_NoOp(self, node):
        return builtins.void

    def visit_Pack(self, node):
        input_values = []
        intype = None
        rettype = None
        for i in node.inputs:
            intype = self.visit(i)
            input_values.append(self.gdict[i].attr['symbolic_value'])
        if all(i is not None for i in input_values):
            # we can force the value!
            for i in range(len(input_values)):
                input_values[i] = input_values[i].val
                input_values[i] = np.array(input_values[i])
            val = np.stack(arrays=input_values, axis=node.attr['axis'])
            primitive = intype
            if builtins.is_tensor(intype):
                primitive = intype.get_primitive()
            rettype = builtins.tensor(primitive, tuple(val.shape))
            node.attr['symbolic_value'] = rettype()
            node.attr['symbolic_value'].val = val
        if rettype is not None:
            return rettype
        else:
            output_shapes = node.attr['_output_shapes']
            if len(output_shapes[0]) > 0:
                return builtins.tensor(node.attr['T'], tuple(output_shapes[0]))
            elif 'N' in node.attr:
                return builtins.tensor(node.attr['T'], (node.attr['N'],))

    def visit_Pad(self, node):
        lefttype = self.visit(node.inputs[0])
        self.visit(node.inputs[1])
        s = self.gdict[node.inputs[1]].attr['symbolic_value']
        if not s:
            attr_type = self._get_type_from_attr(node)
            if not attr_type and self.gdict[node.inputs[1]].datatype and not any_symbolic_or_unknown(
                self.gdict[node.inputs[1]].datatype.T[1]):
                # at least we can get a rank
                rank = self.gdict[node.inputs[1]].datatype.T[1][0]
                ret_shape = [make_symbol(node.name + "_" + str(i)) for i in range(rank)]
                return builtins.tensor(lefttype.get_primitive(), ret_shape)
            else:
                return attr_type
        s = s.val
        retshape = list(lefttype.get_shape())
        for i in range(len(retshape)):
            retshape[i] = retshape[i] + s[i][0] + s[i][1]
        return builtins.tensor(lefttype.get_primitive(), retshape)

    def visit_PadV2(self, node):
        return self.visit_Pad(node)

    def visit_MirrorPad(self, node):
        return self.visit_Pad(node)

    def visit_Placeholder(self, node):
        return self._get_type_from_attr(node)

    def visit_PlaceholderWithDefault(self, node):
        return self._get_type_from_attr(node)

    def visit_Range(self, node):
        start = 0
        delta = 1
        limit = 0
        datatype = builtins.int32

        assert (len(node.inputs) > 1)
        typea = self.visit(node.inputs[0])
        typeb = self.visit(node.inputs[1])
        if typea is None or typeb is None:
            # Non-const propagation.
            return
        if self.gdict[node.inputs[0]].attr['symbolic_value'] is None or self.gdict[
            node.inputs[1]].attr['symbolic_value'] is None:
            # Non-fixed value propagation (e.g. TensorArray)
            return builtins.tensor(datatype, [make_symbol(node.name + '_range')])
        if typea != builtins.int32:
            datatype = typea
        elif typeb != builtins.int32:
            datatype = typeb

        if len(node.inputs) == 2:
            limit = self.gdict[node.inputs[0]].attr['symbolic_value'].val
            delta = self.gdict[node.inputs[1]].attr['symbolic_value'].val
        elif len(node.inputs) == 3:
            typec = self.visit(node.inputs[2])
            if typec is None:
                return builtins.tensor(datatype, [make_symbol(node.name + '_range')])
            if self.gdict[node.inputs[2]].attr['symbolic_value'] is None:
                return builtins.tensor(datatype, [make_symbol(node.name + '_range')])
            if typec != builtins.int32:
                datatype = typec
            start = self.gdict[node.inputs[0]].attr['symbolic_value'].val
            limit = self.gdict[node.inputs[1]].attr['symbolic_value'].val
            delta = self.gdict[node.inputs[2]].attr['symbolic_value'].val
        shape = (limit - start) / delta
        try:
            shape = int(shape)
        except:
            pass
        rettype = builtins.tensor(datatype, [shape])
        if delta == 1:
            node.attr['symbolic_value'] = rettype()
            node.attr['symbolic_value'].val = sm.Interval(start, limit)
        return rettype

    def visit_Rank(self, node):
        # This is also interesting. Tensorflow will return a 0-D tensor,
        # while we transformed 0-D tensor to scalar in parsing.
        input_type = self.visit(node.inputs[0])
        input_shape = input_type.get_shape()
        node.attr['symbolic_value'] = builtins.int32()
        node.attr['symbolic_value'].val = len(input_shape)
        return builtins.int32

    def visit_Relu(self, node):
        return self.visit_unary(node)

    def visit_Relu6(self, node):
        return self.visit_unary(node)

    def visit_LeakyRelu(self, node):
        return self.visit_unary(node)

    def visit_Selu(self, node):
        return self.visit_unary(node)

    def visit_Reshape(self, node):
        def check_volumetric_constraint(left_volume, inshape):
            right_volume = 1
            left_symbols = set()
            right_symbols = set()
            try:
                left_symbols = left_volume.free_symbols
            except:
                pass
            try:
                right_symbols = right_volume.free_symbols
            except:
                pass
            # Generally, we want to solve for right in terms of left. But this
            # is kinda annoying actually.
            shape = list(inshape)
            for i in shape:
                right_volume = right_volume * i
            if is_symbolic(right_volume):
                constraints = [left_volume - right_volume]
                solve_for = [s for s in shape if is_symbolic(s)]

                for rightsym in solve_for:
                    sol = sm.solve(constraints, [rightsym], dict=True)
                    if not isinstance(sol, list):
                        sol = [sol]
                    # look for an acceptable solution
                    for s in sol:
                        if 0 in s.values():
                            continue
                        for i in range(len(shape)):
                            if shape[i] in s:
                                v = s[shape[i]]
                                if len(v.free_symbols - left_symbols) > 0:
                                    continue
                                try:
                                    shape[i] = int(v)
                                except:
                                    shape[i] = v
            return shape

        assert (len(node.inputs) == 2)
        lefttype = self.visit(node.inputs[0])
        if builtins.is_tensor(lefttype):
            left_primitive = lefttype.get_primitive()
            left_shape = lefttype.get_shape()
            left_volume = 1
            for i in left_shape:
                left_volume = left_volume * i
        else:
            left_primitive = lefttype
            left_volume = 1
        if lefttype is None:
            return None
        self.visit(node.inputs[1])
        if self.gdict[node.inputs[1]].attr['symbolic_value'] is not None:
            shape = list(self.gdict[node.inputs[1]].attr['symbolic_value'].val)
            replace_neg_1_with_symbolic(shape, node.name + '_shape')
            shape = check_volumetric_constraint(left_volume, shape)
            r = builtins.tensor(left_primitive, shape)
            if self.gdict[node.inputs[0]].attr['symbolic_value'] is not None \
                    and all(isscalar(a) for a in shape):
                node.attr['symbolic_value'] = r()
                node.attr['symbolic_value'].val = \
                    np.reshape(self.gdict[node.inputs[0]].attr['symbolic_value'].val, shape)
            return r

        # check if we have answer from attributes.
        # Otherwise the final fall back is just [-1] * rank
        try:
            attr_type = self._get_type_from_attr(node)
        except:
            attr_type = None
        if attr_type is not None:
            shape = check_volumetric_constraint(left_volume, attr_type.get_shape())
            return builtins.tensor(attr_type.get_primitive(), shape)
        elif self.gdict[node.inputs[1]].datatype is not None and not any_symbolic_or_unknown(
                self.gdict[node.inputs[1]].datatype.T[1]):
            # at least we can get a rank
            rank = self.gdict[node.inputs[1]].datatype.T[1][0]
            ret_shape = [make_symbol(node.name + "_" + str(i)) for i in range(rank)]
            return builtins.tensor(left_primitive, ret_shape)

    def visit_return(self, node):
        return self.visit_unary(node)

    def visit_ReverseSequence(self, node):
        assert (len(node.inputs) == 2)
        return self.visit(node.inputs[0])

    def visit_ReverseV2(self, node):
        assert (len(node.inputs) == 2)
        return self.visit(node.inputs[0])

    def visit_Sin(self, node):
        rettype = self.visit_unary(node)
        input = self.gdict[node.inputs[0]]
        if input.attr['symbolic_value'] is not None:
            node.attr['symbolic_value'] = rettype()
            node.attr['symbolic_value'].val = np.sin(input.attr['symbolic_value'].val)
        return rettype

    def visit_Cos(self, node):
        rettype = self.visit_unary(node)
        input = self.gdict[node.inputs[0]]
        if input.attr['symbolic_value'] is not None:
            node.attr['symbolic_value'] = rettype()
            node.attr['symbolic_value'].val = np.cos(input.attr['symbolic_value'].val)
        return rettype

    def visit_Tan(self, node):
        rettype = self.visit_unary(node)
        input = self.gdict[node.inputs[0]]
        if input.attr['symbolic_value'] is not None:
            node.attr['symbolic_value'] = rettype()
            node.attr['symbolic_value'].val = np.tan(input.attr['symbolic_value'].val)
        return rettype

    def visit_Tanh(self, node):
        rettype = self.visit_unary(node)
        input = self.gdict[node.inputs[0]]
        if input.attr['symbolic_value'] is not None:
            node.attr['symbolic_value'] = rettype()
            node.attr['symbolic_value'].val = np.tanh(input.attr['symbolic_value'].val)
        return rettype

    def visit_Sqrt(self, node):
        rettype = self.visit_unary(node)
        input = self.gdict[node.inputs[0]]
        if input.attr['symbolic_value'] is not None:
            node.attr['symbolic_value'] = rettype()
            node.attr['symbolic_value'].val = input.attr['symbolic_value'].val**(0.5)
        return rettype

    def visit_Rsqrt(self, node):
        rettype = self.visit_unary(node)
        input = self.gdict[node.inputs[0]]
        if input.attr['symbolic_value'] is not None:
            node.attr['symbolic_value'] = rettype()
            node.attr['symbolic_value'].val = input.attr['symbolic_value'].val**(-0.5)
        return rettype

    def visit_Square(self, node):
        rettype = self.visit_unary(node)
        input = self.gdict[node.inputs[0]]
        if input.attr['symbolic_value'] is not None:
            node.attr['symbolic_value'] = rettype()
            node.attr['symbolic_value'].val = input.attr['symbolic_value'].val**2
        return rettype

    def visit_Exp(self, node):
        return self.visit_unary(node)

    def visit_Shape(self, node):
        # need to parse node itself.
        parent_type = self.visit(node.inputs[0])
        shape = []
        if parent_type is None or not builtins.is_tensor(parent_type):
            return builtins.tensor(builtins.int32, [make_symbol(node.name + '_shape')])
        if parent_type is not None:
            shape = parent_type.get_shape()
            rettype = builtins.tensor(builtins.int32, [len(shape)])
        else:
            rettype = builtins.tensor(builtins.int32, [make_symbol(node.name + '_shape')])
        if len(shape) > 0 and all([dim > 0 for dim in shape]):
            # we have the true value
            node.attr['symbolic_value'] = rettype()
            node.attr['symbolic_value'].val = np.array(shape)
        return rettype

    def visit_Select(self, node):
        assert len(node.inputs) == 3
        typecond = self.visit(node.inputs[0])

        if builtins.is_tensor(typecond):
            # this is a masking op.
            # change the name
            node.op = 'SelectMask'

        typea = self.visit(node.inputs[1])
        typeb = self.visit(node.inputs[2])

        rankcond = len(self.gdict[node.inputs[0]].datatype.get_shape())
        ranka = len(self.gdict[node.inputs[1]].datatype.get_shape())
        rankb = len(self.gdict[node.inputs[2]].datatype.get_shape())

        assert (ranka == rankb)
        if rankcond == 1 and ranka > 1:
            node.attr['expand_dims'] = [-i - 1 for i in range(ranka - rankcond)]

        if typea is not None and typeb is not None:
            compatible, restype = builtins.is_tensor_and_is_compatible_general_shape(typea, typeb)
            if compatible:
                return restype
            elif typea.get_shape() == typeb.get_shape():
                return typea
            else:
                print(builtins.get_type_info(typea), " != ", builtins.get_type_info(typeb))
                assert (typea == typeb)

        if typea is not None:
            return typea
        else:
            return typeb

    def visit_SelectMask(self, node):
        return self.visit_Select(node)

    def visit_SelectV2(self, node):
        return self.visit_Select(node)

    def visit_iff(self, node):
        # an op we inserted. equivalent to the functional IF
        # IF cond: true: false
        assert (len(node.inputs) == 3)
        typecond = self.visit(node.inputs[0])
        # assert (builtins.is_tensor(typecond) == False)

        typea = self.visit(node.inputs[1])
        typeb = self.visit(node.inputs[2])
        if typea is not None and typeb is not None:

            compatible, restype = builtins.is_tensor_and_is_compatible_general_shape(typea, typeb)
            if compatible:
                return restype
            elif typea == typeb:
                return typea
            else:
                print(
                    "In an IFF node ", builtins.get_type_info(typea), " != ",
                    builtins.get_type_info(typeb))
                return typea

        if typea is not None:
            return typea
        else:
            return typeb

    def visit_Where(self, node):
        if len(node.inputs) == 3:
            return self.visit_Select(node)
        assert (len(node.inputs) == 1)
        self.visit(node.inputs[0])
        rank = len(self.gdict[node.inputs[0]].datatype.get_shape())
        ret_shape = [make_symbol(node.name + "_" + str(0)), rank]
        return builtins.tensor(builtins.int32, ret_shape)

    def visit_Sigmoid(self, node):
        return self.visit_unary(node)

    def visit_Elu(self, node):
        return self.visit_unary(node)

    def visit_Slice(self, node):
        for i in node.inputs:
            self.visit(i)
        input_type = self.visit(node.inputs[0])
        input_shape = input_type.get_shape()
        input_value = self.gdict[node.inputs[0]].attr['symbolic_value']
        try:
            begin = list(self.gdict[node.inputs[1]].attr['symbolic_value'].val)
            size = list(self.gdict[node.inputs[2]].attr['symbolic_value'].val)
            end = [
                int(begin[i] + size[i]) if size[i] != -1 else 2147483647 for i in range(len(begin))
            ]
            slices = [[int(begin[i]), int(end[i]), 1] for i in range(len(begin))]
            node.attr['slice'] = slices
            node.attr['begin_masks'] = [idx for idx, value in enumerate(begin) if value == 0]
            node.attr['end_masks'] = [idx for idx, value in enumerate(end) if value == 2147483647]
            output_value = None
            if input_value is not None:
                slices = [slice(*i) for i in slices]
                slices = tuple(slices)
                res = input_value.val[slices]

                if isscalar(res):
                    rettype = input_type.get_primitive()
                    output_value = rettype
                    output_value.val = res
                elif not isscalar(res):
                    rettype = builtins.tensor(input_type.get_primitive(), res.shape)
                    output_value = rettype()
                    output_value.val = res
            else:
                retshape = []
                for i in range(len(begin)):
                    if is_symbolic_or_unknown(size[i]):
                        if is_symbolic_or_known(input_shape[i]) and is_symbolic_or_known(begin[i]):
                            retshape.append(input_shape[i] - begin[i])
                        else:
                            retshape.append(make_symbol(node.name + '_' + str(i)))
                    else:
                        retshape.append(size[i])
                if len(retshape) == 0:
                    rettype = input_type.get_primitive()
                else:
                    rettype = builtins.tensor(input_type.get_primitive(), retshape)
            node.attr['symbolic_value'] = output_value
        except:
            # unable to infer shape
            if 'slice' in node.attr:
                del node.attr['slice']
            node.attr['squeeze'] = []
            try:
                begin = list(self.gdict[node.inputs[1]].attr['symbolic_value'].val)
                size = list(self.gdict[node.inputs[2]].attr['symbolic_value'].val)
                size = [input_shape[i] if s == -1 else s for i, s in enumerate(size)]

                if len(size) == 1 and size[0] == 1:
                    rettype = input_type.get_primitive()
                else:
                    rettype = builtins.tensor(input_type.get_primitive(), size)
                node.attr['generic_slice'] = True
            except:
                retshape = []
                for i in range(len(input_shape)):
                    retshape.append(make_symbol(node.name + '_' + str(i)))
                if len(retshape) == 0:
                    rettype = input_type.get_primitive()
                else:
                    rettype = builtins.tensor(input_type.get_primitive(), retshape)
        return rettype

    def visit_Softmax(self, node):
        return self.visit_unary(node)

    def visit_LogSoftmax(self, node):
        return self.visit_unary(node)

    def visit_Split(self, node, mode='Split'):
        datatype = None
        if 'T' in node.attr and node.attr['T'] is not None:
            datatype = node.attr['T']
        elif 'dtype' in node.attr and node.attr['dtype'] is not None:
            datatype = node.attr['dtype']
        # try to fill unknown output shapes from the input
        shapes = None
        num_split = None
        if 'num_split' in node.attr:
            num_split = node.attr['num_split']
        if '_output_shapes' in node.attr:
            shapes = node.attr['_output_shapes']
        split_dim_idx = 2 if mode == 'SplitV' else 0
        value_idx = 0 if mode == 'SplitV' else 1
        self.visit(node.inputs[split_dim_idx])
        if mode == 'SplitV':
            self.visit(node.inputs[1])
            if self.gdict[node.inputs[1]].attr['symbolic_value'] is not None:
                split_size_type = self.gdict[node.inputs[1]].datatype
                split_size = self.gdict[node.inputs[1]].attr['symbolic_value'].val
                if not builtins.is_tensor(split_size_type):
                    mode = 'Split'
        # this *must!* be constant
        if self.gdict[node.inputs[split_dim_idx]].attr['symbolic_value'] is not None:
            split_dim = self.gdict[node.inputs[split_dim_idx]].attr['symbolic_value'].val
            input_type = self.visit(node.inputs[value_idx])
            if datatype is None:
                datatype = input_type.get_primitive()
            node.attr['split_dim'] = int(split_dim)
            if input_type is not None:
                input_shape = input_type.get_shape()
                from_shapes_ok = False
                try:
                    if shapes is not None:
                        # use the type infered shapes as much as possible
                        for s in shapes:
                            for k in range(len(input_shape)):
                                if k != split_dim and is_symbolic_or_unknown(s[k]):
                                    s[k] = input_shape[k]
                                elif k == split_dim and is_symbolic_or_unknown(s[k]):
                                    s[k] = input_shape[k] // num_split
                        node.attr['split'] = [s[split_dim] for s in shapes]
                        from_shapes_ok = True
                except:
                    pass
                if not from_shapes_ok:
                    output_shape = list(input_shape[:])
                    idim = input_shape[split_dim]
                    if mode == 'Split':
                        assert (idim % num_split == 0)
                        if is_symbolic_or_known(idim):
                            node.attr['split'] = [idim // num_split] * num_split
                            output_shape[split_dim] = idim // num_split
                        else:
                            node.attr['split'] = [-1] * num_split
                            output_shape[split_dim] = -1
                        shapes = [output_shape] * num_split
                    else:
                        assert (np.sum(split_size) == idim or is_symbolic_or_unknown(idim))
                        node.attr['split'] = list(split_size)
                        shapes = [output_shape] * len(split_size)
                        for idx, s in enumerate(split_size):
                            shapes[idx][split_dim] = s

            types = [builtins.tensor(datatype, tuple(shape)) for shape in shapes]
        else:
            types = [
                builtins.tensor(datatype, tuple(shape)) for shape in node.attr['_output_shapes']
            ]
        return builtins.tuple(types)

    def visit_SplitV(self, node):
        # this is like split but has shapes
        # implemented in Split
        return self.visit_Split(node, mode='SplitV')

    def visit_MatrixBandPart(self, node):
        assert (len(node.inputs) == 3)
        return self.visit(node.inputs[0])

    def visit_Unpack(self, node):
        input_type = self.visit(node.inputs[0])
        input_shape = input_type.get_shape()
        axis = node.attr['axis']
        assert (_ > 0 for _ in input_shape[:axis])
        length = input_shape[axis]
        retshape = input_shape[:axis] + input_shape[axis + 1:]
        return builtins.tuple([builtins.tensor(input_type.get_primitive(), tuple(retshape))] *
                              length)

    def visit_StopGradient(self, node):
        # this is just identity
        node.op = 'Identity'
        return self.visit_unary(node)

    def visit_Mean(self, node):
        return self.visit_reduction_op(node)

    def visit_Squeeze(self, node):
        sourcetype = self.visit(node.inputs[0])
        if sourcetype is not None:
            squeezed_shape = list(sourcetype.T[1])
            d = sorted(node.attr['squeeze_dims'])
            if len(d) > 0:
                d = d[::-1]
                for i in d:
                    squeezed_shape.pop(i)
            else:
                squeezed_shape = [s for s in squeezed_shape if s != 1]
            rettype = builtins.tensor(sourcetype.get_primitive(), tuple(squeezed_shape))
            if self.gdict[node.inputs[0]].attr['symbolic_value'] is not None:
                val = self.gdict[node.inputs[0]].attr['symbolic_value'].val
                retval = np.squeeze(val, axis=tuple(d))
                node.attr['symbolic_value'] = rettype()
                node.attr['symbolic_value'].val = retval
            return rettype
        datatype = self._get_type_from_attr(node)
        if datatype is not None:
            return datatype

    def _bitstring_to_reverse_indices(self, i):
        # returns indices in reverse order
        indices = []
        ctr = 0
        if isinstance(i, list):
            return i
        while (i > 0):
            if i % 2 == 1:
                indices.append(ctr)
            i = i // 2
            ctr += 1
        return indices

    def _isKthBitSet(self, n, k):
        if n & (1 << (k)):
            return True
        else:
            return False

    def visit_StridedSlice(self, node):
        # this is massively complicated
        # https://www.tensorflow.org/api_docs/python/tf/strided_slice
        for i in node.inputs:
            self.visit(i)
        input_type = self.visit(node.inputs[0])
        input_shape = input_type.get_shape()
        # unknown input shape. not common. should not happen really.
        if len(input_shape) == 0:
            return input_type

        input_value = self.gdict[node.inputs[0]].attr['symbolic_value']

        begin_value = self.gdict[node.inputs[1]].attr['symbolic_value']
        end_value = self.gdict[node.inputs[2]].attr['symbolic_value']
        stride_value = self.gdict[node.inputs[3]].attr['symbolic_value']

        # these masks here are really really complicated
        assert node.attr.get('new_axis_mask', 0) == 0

        if all([begin_value, end_value, stride_value]):
            input_rank = len(input_shape)
            num_spec = len(begin_value.val)
            assert input_rank >= num_spec

            dim = 0
            begin_mask, end_mask, shrink_axes = [], [], []
            begin_ids, end_ids, strides = [], [], []
            for spec_id in range(num_spec):
                if self._isKthBitSet(node.attr.get('ellipsis_mask', 0), spec_id):
                    num_ellipsis_dims = input_rank - num_spec + 1
                    for _ in range(num_ellipsis_dims):
                        begin_mask.append(dim)
                        end_mask.append(dim)
                        begin_ids.append(0)
                        end_ids.append(0)
                        strides.append(1)
                        dim += 1
                elif self._isKthBitSet(node.attr.get('shrink_axis_mask', 0), spec_id):
                    shrink_axes.append(dim)
                    begin_ids.append(begin_value.val[spec_id])
                    end_ids.append(end_value.val[spec_id])
                    strides.append(stride_value.val[spec_id])
                    dim += 1
                else:
                    if self._isKthBitSet(node.attr.get('begin_mask', 0), spec_id):
                        begin_mask.append(dim)

                    if self._isKthBitSet(node.attr.get('end_mask', 0), spec_id):
                        end_mask.append(dim)

                    begin_ids.append(begin_value.val[spec_id])
                    end_ids.append(end_value.val[spec_id])
                    strides.append(stride_value.val[spec_id])
                    dim += 1

            begin_value = builtins.tensor(begin_value.get_primitive(), (input_rank,))()
            begin_value.val = np.array(begin_ids)

            end_value   = builtins.tensor(end_value.get_primitive(), (input_rank,))()
            end_value.val = np.array(end_ids)

            stride_value = builtins.tensor(stride_value.get_primitive(), (input_rank,))()
            stride_value.val = np.array(strides)
        else:
            assert node.attr.get('ellipsis_mask', 0) == 0
            shrink_axes = self._bitstring_to_reverse_indices(node.attr.get('shrink_axis_mask', 0))
            begin_mask = self._bitstring_to_reverse_indices(node.attr.get('begin_mask', 0))
            end_mask = self._bitstring_to_reverse_indices(node.attr.get('end_mask', 0))

        # try to solve for value if possible
        output_value = None
        rettype = None
        if not None in [input_value, begin_value, end_value, stride_value]:
            begin = [int(i) for i in list(begin_value.val[:])]
            end = [int(i) for i in list(end_value.val[:])]
            for i in begin_mask:
                begin[i] = 0
            for i in end_mask:
                end[i] = None
            # Similar issue to https://github.com/tensorflow/tensorflow/issues/19260
            for i in shrink_axes:
                if begin[i] is None:
                    end[i] = 1
                elif begin[i] == -1:
                    end[i] = None
                else:
                    end[i] = begin[i] + 1
            slices = [slice(*i) for i in zip(begin, end, stride_value.val)]
            # insert missing slices
            for i in range(len(slices), len(input_shape)):
                slices.append(slice(None, None, None))

            slices = tuple(slices)
            res = input_value.val[slices]

            # remove shrink axes
            if len(shrink_axes) > 0:
                if len(shrink_axes) == len(res.shape):
                    if len(res) == 0:
                        print("Warning: %s:%s seems to be a 0 sized tensor" % (node.name, node.op))
                        return builtins.tensor(input_type.get_primitive(), [])
                    res = res.tolist()[0]
                else:
                    res = np.squeeze(res, axis=tuple(shrink_axes))
            # if we have a complete value, we can force it

            slicesv = [[begin[i], end[i], stride_value.val[i]] for i in range(len(begin))]
            for idx, s in enumerate(slicesv):
                if s[0] is None:
                    s[0] = 0
                    begin_mask.append(idx)
                if s[1] is None:
                    s[1] = 2147483647
                    end_mask.append(idx)
                if s[2] is None:
                    s[2] = 1
                s[0] = int(s[0])
                s[1] = int(s[1])
                s[2] = int(s[2])
            # insert missing slices
            for i in range(len(slicesv), len(input_shape)):
                slicesv.append([0, 2147483647, 1])
                if i not in begin_mask:
                    begin_mask.append(i)
                if i not in end_mask:
                    end_mask.append(i)
            node.attr['slice'] = slicesv
            node.attr['squeeze'] = list(int(i) for i in shrink_axes)
            node.attr['begin_masks'] = list(int(i) for i in begin_mask)
            node.attr['end_masks'] = list(int(i) for i in end_mask)
            if isscalar(res):
                rettype = input_type.get_primitive()
                output_value = rettype()
                output_value.val = res
            elif not isscalar(res):
                rettype = builtins.tensor(input_type.get_primitive(), res.shape)
                output_value = rettype()
                output_value.val = res

        # solve for type
        if rettype is None:
            # try to derive entirely from input_shape
            if (None in [begin_value, end_value, stride_value]):
                if len(input_shape) == len(shrink_axes):
                    # we are removing all axes. i.e. we are indexing a
                    # specific element
                    rettype = input_type.get_primitive()
                else:
                    new_shape = [
                        make_symbol(node.name + "_s_" + str(i))
                        for i in range(len(input_shape) - len(shrink_axes))
                    ]
                    rettype = builtins.tensor(input_type.get_primitive(), new_shape)
                # we have a non-constant shaped slice
                # store the sqeeze
                node.attr['squeeze'] = list(int(i) for i in shrink_axes)
                node.attr['begin_masks'] = list(int(i) for i in begin_mask)
                node.attr['end_masks'] = list(int(i) for i in end_mask)
            else:
                retshape = []
                begin = list(begin_value.val[:])
                end = list(end_value.val[:])
                for i in range(len(begin)):
                    try:
                        begin[i] = int(begin[i])
                    except:
                        pass
                for i in range(len(end)):
                    try:
                        end[i] = int(end[i])
                    except:
                        pass
                for i in begin_mask:
                    begin[i] = None
                for i in end_mask:
                    end[i] = None
                for i in shrink_axes:
                    if begin[i] is None:
                        end[i] = 1
                    elif begin[i] == -1:
                        end[i] = None
                    else:
                        end[i] = begin[i] + 1
                if stride_value is not None:
                    stride_value = list(stride_value.val[:])

                for i in range(len(begin)):
                    if i in shrink_axes:
                        retshape.append(1)
                    elif is_symbolic_or_unknown(input_shape[i]):
                        if np.isscalar(begin[i]) and np.isscalar(
                                end[i]) and np.isscalar(stride_value):
                            retshape.append(len(list(range(begin[i], end[i], stride_value[i]))))
                        elif (is_symbolic_or_unknown(begin[i])
                              or is_symbolic_or_unknown(end[i])) and stride_value[i] == 1:
                            if end[i] is None:
                                retshape.append(input_shape[i] - begin[i])
                            else:
                                retshape.append(end[i] - begin[i])
                        else:
                            retshape.append(make_symbol(node.name + '_' + str(i)))
                    else:
                        if begin[i] is not None and begin[i] < 0:
                            try:
                                begin[i] += input_shape[i]
                            except:
                                pass
                        if end[i] is None:
                            end[i] = None # used to be input_shape[i]
                        elif end[i] < 0:
                            try:
                                end[i] += input_shape[i]
                            except:
                                pass
                        thisslice = slice(begin[i], end[i], stride_value[i])
                        thisslicelen = len(list(range(input_shape[i]))[thisslice])
                        retshape.append(thisslicelen)
                slices = [[begin[i], end[i], stride_value[i]] for i in range(len(begin))]
                has_symbolic_slices = False
                for idx, s in enumerate(slices):
                    if s[0] is None:
                        s[0] = 0
                        begin_mask.append(idx)
                    if s[1] is None:
                        s[1] = 2147483647
                        end_mask.append(idx)
                    if s[2] is None:
                        s[2] = 1
                    try:
                        s[0] = int(s[0])
                    except:
                        has_symbolic_slices = True
                        pass
                    try:
                        s[1] = int(s[1])
                    except:
                        has_symbolic_slices = True
                        pass
                    try:
                        s[2] = int(s[2])
                    except:
                        has_symbolic_slices = True
                        pass
                # insert missing slices
                for i in range(len(slices), len(input_shape)):
                    slices.append([0, 2147483647, 1])
                    retshape.append(input_shape[i])
                    if i not in begin_mask:
                        begin_mask.append(i)
                    if i not in end_mask:
                        end_mask.append(i)

                if not has_symbolic_slices:
                    node.attr['slice'] = slices
                node.attr['squeeze'] = list(int(i) for i in shrink_axes)
                node.attr['begin_masks'] = list(int(i) for i in begin_mask)
                node.attr['end_masks'] = list(int(i) for i in end_mask)
                # drop removed axes
                for a in shrink_axes:
                    assert (retshape[a] == 1 or is_symbolic_or_unknown(retshape[a]))
                retshape = [s for i, s in enumerate(retshape) if i not in shrink_axes]
                if len(retshape) == 0:
                    rettype = input_type.get_primitive()
                else:
                    rettype = builtins.tensor(input_type.get_primitive(), retshape)

        node.attr['symbolic_value'] = output_value
        return rettype

    def visit_Max(self, node):
        return self.visit_reduction_op(node)

    def visit_Min(self, node):
        return self.visit_reduction_op(node)

    def visit_Ceil(self, node):
        return self.visit_unary(node)

    def visit_Floor(self, node):
        return self.visit_unary(node)

    def visit_Round(self, node):
        return self.visit_unary(node)

    def visit_Abs(self, node):
        return self.visit_unary(node)

    def visit_Tile(self, node):
        for i in node.inputs:
            self.visit(i)
        input_type = self.visit(node.inputs[0])
        input_shape = input_type.get_shape()
        if len(input_shape) == 0:
            return input_type
        input_value = self.gdict[node.inputs[0]].attr['symbolic_value']
        if input_value is not None:
            input_value = input_value.val

        tile_value = self.gdict[node.inputs[1]].attr['symbolic_value']
        if tile_value is None:
            ret_shape = [make_symbol(node.name + "_" + str(i)) for i in range(len(input_shape))]
            return builtins.tensor(input_type.get_primitive(), ret_shape)
        tile_value = tile_value.val
        assert (len(tile_value) == len(input_shape))
        rettype = builtins.tensor(
            input_type.get_primitive(),
            [input_shape[i] * tile_value[i] for i in range(len(tile_value))])
        if input_value is not None and tile_value is not None and not any_symbolic_or_unknown(tile_value):
            node.attr['symbolic_value'] = rettype()
            node.attr['symbolic_value'].val = np.tile(input_value, tile_value)
        return rettype

    def visit_FloorDiv(self, node):
        return self.visit_broadcast_op(node)

    def visit_RealDiv(self, node):
        return self.visit_broadcast_op(node)

    def visit_OneHot(self, node):
        indices_type = self.visit(node.inputs[0])
        depth_type = self.visit(node.inputs[1])
        on_type = self.visit(node.inputs[2])
        off_type = self.visit(node.inputs[3])
        if builtins.is_tensor(indices_type):
            indices_shape = list(indices_type.get_shape())
        else:
            indices_shape = [1]
        axis = node.attr['axis']
        depth_value = self.gdict[node.inputs[1]].attr['symbolic_value'].val

        if depth_value is None:
            depth_value = make_symbol(node.name + '_depth')
        if 'dtype' in node.attr:
            ret_primitive = node.attr['T']
        else:
            ret_primitive = on_type

        if len(indices_shape) == 0:
            return builtins.tensor(ret_primitive, tuple())
        retshape = indices_shape
        if axis == -1:
            retshape.append(depth_value)
        else:
            retshape.insert(axis, depth_value)
        return builtins.tensor(ret_primitive, retshape)

    def visit_SquaredDifference(self, node):
        # some kind of binary op
        return self.visit_broadcast_op(node)

    def visit_Sub(self, node):
        # sub is a broadcast op
        rettype = self.visit_broadcast_op(node)
        # we compute it for basic scalar cases
        if not (builtins.is_tensor(rettype)) and self.all_inputs_have_values(node):
            val = self.gdict[node.inputs[0]].attr['symbolic_value'].val - self.gdict[
                node.inputs[1]].attr['symbolic_value'].val
            node.attr['symbolic_value'] = rettype()
            node.attr['symbolic_value'].val = val
        return rettype

    def visit_Sum(self, node):
        return self.visit_reduction_op(node)

    def visit_Tanh(self, node):
        return self.visit_unary(node)

    def find_tensor_array_source_node(self, node):
        if 'tensorarray_source' in node.attr:
            loc = node.attr['tensorarray_source']
            if loc in self.whole_ssa.global_resource:
                return self.whole_ssa.global_resource[loc]
        elif '_class' in node.attr:
            loc = node.attr['_class'][0][5:]
            if loc in self.whole_ssa.global_resource:
                return self.whole_ssa.global_resource[loc]

        return None

    def propagate_tensor_array(self, node):
        if node.op == 'make_tuple':
            tensorarray_source = [
                self.gdict[i].attr.get('tensorarray_source', None) for i in node.inputs
            ]
            node.attr['tensorarray_source'] = tensorarray_source
        elif node.op == 'get_tuple':
            if 'tensorarray_source' in self.gdict[node.inputs[0]].attr:
                tensorarray_source = self.gdict[node.inputs[0]].attr['tensorarray_source'][
                    node.attr['index']]
                node.attr['tensorarray_source'] = tensorarray_source
        else:
            self.visit(node.inputs[-1])
            if 'tensorarray_source' in self.gdict[node.inputs[-1]].attr:
                node.attr['tensorarray_source'] = self.gdict[
                    node.inputs[-1]].attr['tensorarray_source']

    def visit_TensorArrayV3(self, node):

        # input is size
        assert (len(node.inputs) <= 1)
        self.visit(node.inputs[0])
        # the input is an int32 which is the size of the tensor
        sizeval = self.gdict[node.inputs[0]].attr['symbolic_value']

        if sizeval is not None and node.attr['dynamic_size'] == False:
            assert isscalar(sizeval.val)
            node.attr['size'] = sizeval.val

        if 'infer_shape' in node.attr:
            # We only support fix size of TensorArray.
            assert (node.attr['infer_shape'])

        if isinstance(node.attr.get('element_shape', []), list):
            shape = []
            if 'element_shape' in node.attr:
                shape = node.attr['element_shape']
            node.attr['element_shape'] = builtins.tensor(node.attr['dtype'], shape)
        self.whole_ssa.global_resource[node.name] = node
        node.attr['tensorarray_source'] = node.name

        return builtins.list(node.attr['element_shape'])

    def visit_TensorArrayGatherV3(self, node):
        # input is resource, indices, flow
        assert (len(node.inputs) == 2)
        indices_type = self.visit(node.inputs[0])

        self.propagate_tensor_array(node)

        if indices_type is None:
            return builtins.tensor(node.attr['dtype'], [-1] + node.attr['element_shape'])
        else:
            indiceslen = indices_type.get_shape()[0]
            return builtins.tensor(node.attr['dtype'], [indiceslen] + node.attr['element_shape'])

    def visit_TensorArrayReadV3(self, node):
        # input is resource, idx, flow
        assert (len(node.inputs) == 2)

        self.propagate_tensor_array(node)
        tanode = self.find_tensor_array_source_node(node)

        ta_type = self.visit(node.inputs[1])
        if tanode is not None:
            ta_type = tanode.datatype
        return ta_type.T[0]

    def visit_TensorArrayScatterV3(self, node):
        # input is resource, indices, values , flow
        self.propagate_tensor_array(node)
        tanode = self.find_tensor_array_source_node(node)

        tensor_put_type = self.visit(node.inputs[1])
        assert (builtins.is_tensor(tensor_put_type))
        tensor_put_type = builtins.tensor(
            tensor_put_type.get_primitive(),
            tensor_put_type.get_shape()[1:])

        # Overide the shape in the node attributes
        if len(tensor_put_type.get_shape()) > 0 and tanode is not None:
            el_shape = tanode.attr.get('element_shape')
            es = None if el_shape is None else el_shape.get_shape()
            if (es is None or len(es) == 0 \
                or (-1 in es and -1 not in tensor_put_type.get_shape())):
                tanode.attr['element_shape'] = tensor_put_type

        # output is flow
        assert (len(node.inputs) == 3)
        return self.visit(node.inputs[2])

    def visit_TensorArraySizeV3(self, node):

        self.propagate_tensor_array(node)
        tanode = self.find_tensor_array_source_node(node)
        for inputnodes in node.inputs:
            self.visit(inputnodes)

        if tanode is not None and 'size' in tanode.attr and tanode.attr.get('dynamic_size',
                                                                            True) == False:
            node.attr['symbolic_value'] = builtins.int32()
            node.attr['symbolic_value'].val = tanode.attr['size']

        # https://github.com/tensorflow/tensorflow/blob/master/tensorflow/core/ops/data_flow_ops.cc
        return builtins.int32

    def visit_TensorArrayWriteV3(self, node):
        # input is resource, index, value, flow
        # output is flow
        # try to infer tensor array element type if possible
        self.propagate_tensor_array(node)
        tanode = self.find_tensor_array_source_node(node)

        tensor_put_type = self.visit(node.inputs[1])
        # Overide the shape in the node attributes

        if hasattr(tensor_put_type, 'get_shape') and \
            len(tensor_put_type.get_shape()) > 0 and tanode is not None:
            el_shape = tanode.attr.get('element_shape')
            es = None if el_shape is None else el_shape.get_shape()
            if (es is None or len(es) == 0 \
                or (-1 in es and -1 not in tensor_put_type.get_shape())):
                tanode.attr['element_shape'] = tensor_put_type

        assert (len(node.inputs) == 3)
        return self.visit(node.inputs[2])

    def visit_TopKV2(self, node):
        # K appears to not be constant
        input_type = self.visit(node.inputs[0])
        self.visit(node.inputs[1])
        ret_shape = list(input_type.get_shape())
        k = -1
        if self.gdict[node.inputs[1]].attr['symbolic_value'] is not None:
            k = self.gdict[node.inputs[1]].attr['symbolic_value'].val
        if k == -1:
            k = make_symbol(node.name + '_k')
        ret_shape[-1] = k
        return builtins.tuple((
            builtins.tensor(input_type.get_primitive(),
                            ret_shape), builtins.tensor(builtins.int32, ret_shape)))

    def visit_Transpose(self, node):
        assert (len(node.inputs) == 2)
        inputtype = self.visit(node.inputs[0])
        self.visit(node.inputs[1])
        assert (builtins.is_tensor(inputtype))
        shape = inputtype.get_shape()
        primitive = inputtype.get_primitive()
        if len(shape) == 0:
            return builtins.tensor(primitive, node.attr['_output_shapes'][0])
        transpose_axes = self.gdict[node.inputs[1]].attr['symbolic_value']
        assert (len(transpose_axes.val) == len(shape))
        new_shape = []
        for ax in transpose_axes.val:
            new_shape.append(shape[ax])
        return builtins.tensor(primitive, new_shape)

    def visit_VariableV2(self, node):
        return None

    def visit_while(self, node):
        assert ("cond_function" in node.attr)
        assert ("body_function" in node.attr)
        assert (len(node.inputs) == 1)

        mytype = self.visit(node.inputs[0])

        functions_called = [node.attr[i] for i in ["cond_function", "body_function"]]
        for f in functions_called:
            if self.whole_ssa is not None and f in self.whole_ssa.functions:
                # look for the function entry point
                entrypoint = [
                    n for n in self.whole_ssa.functions[f].graph.values()
                    if n.op == 'function_entry'
                ]
                entrypoint[0].datatype = mytype
                if 'tensorarray_source' in self.gdict[node.inputs[0]].attr:
                    entrypoint[0].attr['tensorarray_source'] = self.gdict[
                        node.inputs[0]].attr['tensorarray_source']
        if 'tensorarray_source' in self.gdict[node.inputs[0]].attr:
            node.attr['tensorarray_source'] = self.gdict[node.inputs[0]].attr['tensorarray_source']

        return mytype

    def visit_get_global(self, node):
        assert ("variable" in node.attr)
        assert (node.attr['variable'] in self.whole_ssa.variables)
        return self.whole_ssa.variables[node.attr['variable']].__class__

    def visit_set_global(self, node):
        assert ("variable" in node.attr)
        assert (node.attr['variable'] in self.whole_ssa.variables)
        input_type = self.visit(node.inputs[0])
        variable_type = self.whole_ssa.variables[node.attr['variable']].__class__
        if input_type is not None:
            if not (input_type is variable_type
                    or builtins.is_tensor_and_is_compatible_general_shape(input_type,
                                                                          variable_type)[0]):
                print(
                    "Possible incompatible type in set_global: %s. expected %s" %
                    (builtins.get_type_info(input_type), builtins.get_type_info(variable_type)))
        return input_type

    def visit_LSTMBlock(self, node):
        input_type = self.visit(node.inputs[0])
        weight_type = self.visit(node.inputs[1])

        mode = node.attr['mode']
        input_shape = list(input_type.get_shape())
        weight_shape = list(weight_type.get_shape())
        hidden_size = weight_shape[-1] // 4
        if mode == 'encoder' and node.attr.get('bidirectional'):
            hidden_size /= 2
        input_shape[-1] = hidden_size
        if mode == 'cell':
            # returns (output, hidden state, cell state)
            types = [builtins.tensor(input_type.get_primitive(), tuple(input_shape)) for _ in range(3)]
        elif mode == 'encoder':
            hidden_size = input_shape[:]
            output_shape = input_shape[:]
            if not node.attr['output_all_states']:
                if node.attr['time_major']:
                    output_shape[0] = 1
                else:
                    output_shape[1] = 1

            if node.attr.get('bidirectional'):
                output_shape[-1] *= 2
                output_type = builtins.tensor(input_type.get_primitive(), tuple(output_shape))
                hidden_type = builtins.tensor(input_type.get_primitive(), tuple(hidden_size))
                types = [output_type] + [hidden_type] * 4
            else:
                output_type = builtins.tensor(input_type.get_primitive(), tuple(output_shape))
                hidden_type = builtins.tensor(input_type.get_primitive(), tuple(hidden_size))
                types = [output_type] + [hidden_type] * 2
        else:
            raise ValueError('Unknown mode type for LSTMBlock')

        return builtins.tuple(types)

    def visit_Size(self, node):
        self.visit(node.inputs[0])
        parenttype = self.gdict[node.inputs[0]].datatype
        rettype = node.attr["out_type"]
        if parenttype is not None:
            input_shape = parenttype.get_shape()
            node.attr['symbolic_value'] = rettype()
            node.attr['symbolic_value'].val = np.prod(input_shape)
        return rettype

    def visit_Sign(self, node):
        input_type = self.visit(node.inputs[0])
        return input_type

    def visit_Cumsum(self, node):
        assert (len(node.inputs) == 2)
        return self.visit(node.inputs[0])

    def visit_ClipByValue(self, node):
        assert len(node.inputs) == 3

        type_min = self.visit(node.inputs[1])
        type_max = self.visit(node.inputs[2])
        if not (builtins.is_tensor(type_max) or builtins.is_tensor(type_min)):
            node.attr["min_value"] = self.gdict[node.inputs[1]].attr['value'].val
            node.attr["max_value"] = self.gdict[node.inputs[2]].attr['value'].val

        return self.visit(node.inputs[0])

    def visit_SpaceToDepth(self, node):
        return self._get_type_from_attr(node)

    def visit_DepthToSpace(self, node):
        return self._get_type_from_attr(node)

    def visit_SpaceToBatchND(self, node):
        return self._get_type_from_attr(node)

    def visit_BatchToSpaceND(self, node):
        return self._get_type_from_attr(node)

    def visit_LRN(self, node):
        return self.visit_unary(node)

    def visit_Reciprocal(self, node):
        return self.visit_unary(node)


def type_is_unknown(t):
    if builtins.is_tuple(t):
        return any(type_is_unknown(a) for a in t.T)
    elif builtins.is_tensor(t):
        return type_is_unknown(t.get_primitive()) or \
               t.get_shape() is None or \
               len(t.get_shape()) == 0 or \
               any_symbolic_or_unknown(t.get_shape())
    elif builtins.is_list(t):
        return type_is_unknown(t.T[0])
    elif t is builtins.unknown:
        return True
    else:
        return t is None


def type_inference_pass_impl(nnssa):
    """
    Takes an NetworkEnsemble object and performs recursive type inference
    on all the nodes in the graph
    """
    function_names = list(nnssa.functions.keys())
    function_names = sorted(function_names)
    # stick the main functions at the start
    if "main" in function_names:
        function_names = ["main"] + [i for i in function_names if i != "main"]

    import copy
    # try to infer all the set_global types first
    changed_variables = []
    for k in function_names:
        graph = copy.copy(nnssa.functions[k].graph)
        for v in graph.values():
            if v.op == 'set_global':
                rettype = TypeInferenceVisitor(graph, nnssa).visit(v)
                variable = v.attr['variable']
                validate_shape =  v.attr.get('validate_shape', True)
                if (variable in changed_variables) and validate_shape:
                    if builtins.get_type_info(
                            nnssa.variables[variable]) == builtins.get_type_info(rettype):
                        continue
                    else:
                        raise TypeError(
                            "Varable %s changes type several times from %s to %s" % (
                                variable, builtins.get_type_info(
                                    nnssa.variables[variable]), builtins.get_type_info(rettype)))
                if rettype != type(nnssa.variables[variable]):
                    nnssa.variables[variable] = rettype()
                    if variable not in changed_variables:
                        changed_variables.append(variable)
                    print(
                        "Changing variable %s to type %s" %
                        (variable, builtins.get_type_info(rettype)))
        nnssa.functions[k].find_inputs_and_outputs()

    # reinfer unknown shapes and types
    for k in function_names:
        graph = copy.copy(nnssa.functions[k].graph)
        for v in graph.values():
            if type_is_unknown(v.datatype):
                v.datatype = None

    # run it for real
    for k in function_names:
        TypeInferenceVisitor(nnssa.functions[k].graph, nnssa).visit_all()


def recursive_replace_symbols_in_type_with_unknown(dtype):
    if builtins.is_list(dtype):
        return builtins.list(recursive_replace_symbols_in_type_with_unknown(dtype.T[0]))
    elif builtins.is_tuple(dtype):
        return builtins.tuple(
            tuple(recursive_replace_symbols_in_type_with_unknown(t) for t in dtype.T))
    elif builtins.is_tensor(dtype):
        return builtins.tensor(
            dtype.get_primitive(),
            tuple(-1 if issubclass(type(t), sm.Basic) else int(t) for t in dtype.get_shape()))
    else:
        return dtype


def recursive_replace_symbols_in_values(val):
    # try some things in sympy.core.numbers
    if issubclass(type(val), sm.Basic):
        return int(val)
    elif isinstance(val, list):
        return [recursive_replace_symbols_in_values(i) for i in val]
    elif isinstance(val, tuple):
        return tuple([recursive_replace_symbols_in_values(i) for i in val])
    elif isinstance(val, np.ndarray):
        if np.issctype(val.dtype):
            return val
        else:
            return np.array([recursive_replace_symbols_in_values(i)
                             for i in val.flatten()]).reshape(val.shape)
    else:
        return val


def graph_replace_symbolic_values(gdict):
    for k in gdict:
        v = gdict[k]
        if v.value is None and v.attr['symbolic_value'] is not None and not any_symbolic_or_unknown(
                v.attr['symbolic_value'].val):
            v.value = v.attr['symbolic_value']
            v.value.val = recursive_replace_symbols_in_values(v.value.val)
        v.attr['symbolic_datatype'] = v.datatype
        v.datatype = recursive_replace_symbols_in_type_with_unknown(v.datatype)


def graph_make_symbolic_values(gdict):
    for k in gdict:
        gdict[k].attr['symbolic_value'] = gdict[k].value


def type_inference_pass(nnssa):
    # repeat for as many times as there are functions
    # this is the maximum number of times required for convergence
    for i in nnssa.functions:
        graph_make_symbolic_values(nnssa.functions[i].graph)
    for i in range(len(nnssa.functions)):
        type_inference_pass_impl(nnssa)
    for i in nnssa.functions:
        graph_replace_symbolic_values(nnssa.functions[i].graph)
    for i in nnssa.variables:
        nnssa.variables[i] = recursive_replace_symbols_in_type_with_unknown(nnssa.variables[i])
