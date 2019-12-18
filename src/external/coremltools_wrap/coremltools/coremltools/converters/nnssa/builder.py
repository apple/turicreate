"""
Helper class for 

1. build a SSA network,
2. plug in SSA nodes into SSA graph.

"""

import sys
import numpy as np

from .nnssa import *
from .commons.parse import numpy_val_to_builtin_val


class SSABuilder(object):
    """
    A helper class to build SSA network from scratch.

    Would be useful to run with GraphBuilder together.

    Examples
    --------
    .. sourcecode:: python

        # SSABuilder initiation with some functions injected
        >>> sb = SSABuilder(functions=functions)

        # GraphBuilder builds some nodes into graph.
        >>> gb = GraphBuilder()
        >>> _ = gb.add_elementwise(...)
        >>> _ = gb.add_elementwise(...)

        # Add the graph built from GraphBuilder into SSABuilder
        >>> sb.add_graph(gb.get_graph())

        # Return a NNSSA
        >>> sb.get_ssa()
    """

    def __init__(self, functions=None, variables=None, global_resource=None):
        """
        Construct a SSABuilder object

        Parameters
        ----------
        functions: {str: SSAFunction}
            Functions that users pre-defined to plug in NetworkEnsembles.

        variables: {str: NNSSA.builtins}
            Global variables used by nodes in the network. 
            Reference to set_global / get_global.

        global_resource: dict
            Global resource used across the network.
        """
        self.ssa = NetworkEnsemble()
        if functions is None:
            self.ssa.functions = dict()
        else:
            self.ssa.functions = functions
        if variables is None:
            self.ssa.variables = dict()
        else:
            self.ssa.variables = variables
        if global_resource is None:
            self.ssa.global_resource = dict()
        else:
            self.ssa.global_resource = global_resource

    def get_ssa(self):
        """
        Obtain the NNSSA that can be used to do graph surgery/convert to backends.
        """
        return copy.deepcopy(self.ssa)

    def add_graph(self, graph={}, name="main"):
        """
        Add a function into NNSSA by a constructed graph

        Parameters
        ----------
        graph: {str: ParsedNode}
            A graph that is derived by GraphBuilder or from raw construction.

        name: str
            The name of the function that would be added.
        """
        if name in self.ssa.functions:
            print("Failed adding graph! Name already exist in the NNSSA network!")
        else:
            self.ssa.add_function(name, SSAFunction(graph))

    def add_function(self, function=None, name="main"):
        """
        Add a function into NNSSA by SSAFunction

        Parameters
        ----------
        function: SSAFunction
            A SSAFunction

        name: str
            The name of the function that would be added.
        """
        if not isinstance(function, SSAFunction):
            print("Failed adding function! The input is not a SSAFunction.")
        elif name in self.ssa.functions:
            print("Failed adding function! Name already exist in the NNSSA network!")
        else:
            self.ssa.add_function(name, function)


class GraphBuilder(object):
    """
    A helper class to plug in SSA nodes into SSA graph.

    This GraphBuilder is a helper for users to construct SSA graph from node
    level specifications.
    GraphBuilder would be helpful if you don't know the specific input order or
    required attributes for some operation nodes.
    
    The GraphBuilder only guarantees to provide attributes that are needed for
    the NEspresso backend.
    
    Examples
    --------
    .. sourcecode:: python

        >>> builder = GraphBuilder(ssa, nodename_prefix, ParsedNode)

        >>> one = np.int(1)
        >>> two = np.int(2)
        
        # returns name of the node added in ssa graph
        >>> const_1 = builder.add_const(one, name='one')
        >>> const_2 = builder.add_const(two, name='two')

        # Add a "Add" node with input "const_1" and "const_2"
        >>> const_3 = builder.add_elementwise('Add', const_1, const_2)

    """

    def __init__(self, graph=None, prefix="", node_class=ParsedNode):
        """
        Construct a GraphBuilder object

        Parameters
        ----------
        graph: dict
            The dictionary that represents the graph in NetworkEnsemble.SSAFunction.graph
        
        prefix: str
            A string that would be the prefix of the node's name constructed.

        node_class: ParsedNode or sub-class of ParsedNode
            The node class that would be plugged into graph.

        """
        if graph is None:
            self.graph = dict()
        else:
            self.graph = graph
        self.prefix = prefix
        self.node_class = node_class

    def _find_free_name(self, name):
        if name not in self.graph:
            return name
        i = 0
        while (True):
            candidate = name + '_' + str(i)
            if candidate not in self.graph:
                return candidate
            i += 1

    def _build_op(self, op, inputs, **kwargs):
        name = kwargs.get("name", None)
        value = kwargs.get("value", None)
        attr = kwargs.get("attr", {})
        datatype = kwargs.get("datatype", None)

        inserted_op = self.node_class()
        inserted_op.op = op
        inserted_op.inputs = copy.copy(inputs)
        inserted_op.attr = copy.copy(attr)
        inserted_op.value = value
        if datatype is not None:
            inserted_op.attr['dtype'] = datatype
            inserted_op.datatype = datatype
        if name is None:
            name = op
        name = self._find_free_name(self.prefix + name)
        inserted_op.name = name
        self.graph[name] = inserted_op
        for i in inputs:
            self.graph[i].outputs.append(name)
        return name

    def get_graph(self):
        """
        Obtain the graph that can be used for further surgery or initiate a SSAFunction.

        Examples
        --------
        .. sourcecode:: python

            # GraphBuilder builds some nodes into graph.
            >>> _ = builder.add_split(...)
            >>> _ = builder.add_get_tuple(...)
            >>> _ = builder.add_elementwise(...)
            >>> _ = builder.add_elementwise(...)
            >>> _ = builder.add_elementwise(...)

            # Merge two graphs together
            >>> gdict = builder.get_graph()
            >>> gdict1 = some_other_builder.get_graph()
            >>> merge_graph = {**gdict, **gdict1}

            # or to build a SSAFunction
            >>> f = SSAFunction(gdict)

        """
        return copy.deepcopy(self.graph)

    def get_function(self):
        """
        Obtain the SSAFunction that is derived from graph

        Examples
        --------
        .. sourcecode:: python

            # GraphBuilder builds some nodes into graph.
            >>> _ = builder.add_split(...)
            >>> _ = builder.add_get_tuple(...)
            >>> _ = builder.add_elementwise(...)
            >>> _ = builder.add_elementwise(...)
            >>> _ = builder.add_elementwise(...)

            # get_function returns a SSAFunction
            >>> f = builder.get_function()
        """
        return SSAFunction(self.get_graph())

    def get_ssa(self, name='main'):
        """
        Obtain the NNSSA that is derived from graph and can be used for
        graph surgery or backend conversions.

        Parameters
        ----------
        name: str
            The SSAFunction's name that would contain the graph generated by builder.

        """

        ssa = NetworkEnsemble()
        ssa.add_function(name, self.get_function())
        return ssa

    def add_placeholder(self, init_from=None, datatype=None, name=None):
        """
        Add an 'placeholder' node to the SSAFunction.

        SSANode form:
            op: 'Placeholder'
            datatype: NNSSA.builtins type

        Examples
        --------
        .. sourcecode:: python

            # Add a placeholder of float32 with tensor shape [-1, 512]
            >>> ph1 = builder.add_placeholder(builtins.tensor(builtins.fp32, [-1, 512]), name="ph1")

            # Add a placeholder of integer
            >>> ph2 = builder.add_placeholder(builtins.int32, name="ph2")

        Parameters
        ----------
        datatype: NNSSA.builtins type
            The type of placeholder

        init_from: numpy
            Can be used to derive the corresponding NNSSA builtins type. Value
            will be ignored.

        name: str
            The name of the placeholder
        """

        if init_from is None and datatype is None:
            raise ValueError("Datatype not set!")
        if init_from is not None:
            _, datatype = numpy_val_to_builtin_val(init_from)
        return self._build_op('Placeholder', [], datatype=datatype, name=name)

    def add_identity(self, input_name, name=None):
        """
        Add an 'identity' node to the SSAFunction.
        'make_tuple' is used when we need to merge multiple nodes into one for
        feeding it into a function.

        SSANode form:
            op: 'identity'
            inputs: [input_name]

        Parameters
        ----------
        input_name: str
            The input for this node.
        """
        return self._build_op('Identity', [input_name], name=name)

    def add_make_tuple(self, input_names, name=None):
        """
        Add an 'make_tuple' node to the SSAFunction.
        'make_tuple' is used when we need to merge multiple nodes into one for
        feeding it into a function.

        SSANode form:
            op: 'make_tuple'
            inputs: input_names

        Examples
        --------
        .. sourcecode:: python

            # split some_input into 5 slices.
            >>> outputs = builder.add_split(some_input, num_split=5)
            # obtain the first slice from outputs.
            >>> output = builder.add_get_tuple(outputs, index=0)

        Parameters
        ----------
        input_names: str
            The inputs for this node.
        """
        return self._build_op('make_tuple', input_names, name=name)

    def add_get_tuple(self, input_name, index, name=None):
        """
        Add an 'get_tuple' node to the SSAFunction.
        'get_tuple' is used to obtain output from a node with multiple outputs.

        i.e. A = outputs[x] translates to builder.add_get_tuple(A, index=x)

        SSANode form:
            op: 'get_tuple'
            inputs: [input_name]
            attr: 'index'

        Examples
        --------
        .. sourcecode:: python

            # split some_input into 5 slices.
            >>> outputs = builder.add_split(some_input, num_split=5)
            # obtain the first slice from outputs.
            >>> output = builder.add_get_tuple(outputs, index=0)

        Parameters
        ----------
        input_name: str
            The input for this node.
        index: int
            The index to read from.
        """
        return self._build_op('get_tuple', [input_name], name=name, attr={'index': index})

    def add_activation(self, op, input_name, name=None, attr={}):
        """
        Add an activation node to the SSAFunction.

        SSANode form:
            op: op
            inputs: [input_name]
            attr: 'alpha', 'beta'

        Parameters
        ----------
        op: str
            The activation function for this node.

            It can be one of the following:

                - 'Sigmoid': sigmoid function.
                - 'Tanh': tanh function.
                - 'Relu': Rectified Linear Unit (ReLU) function.

                - 'HardSigmoid': hard sigmoid function, defined as: 

                  `f(x) = min(max(alpha * x + beta, -1), 1)` 

                  where alpha and beta are constant scalars.
                  [default: alpha = 1.6733, beta = 1.0507]
                - 'Elu': Exponential linear unit function, defined as: 

                  `f(x) = (x >= 0) * x + (x < 0) * (alpha * exp(x) - 1)`

                  where alpha is a constant scalar.
                  [default: alpha = 1.0]
                - 'Selu': Exponential linear unit function, defined as: 

                  `f(x) = beta * ((x >= 0) * x + (x < 0) * (alpha * (exp(x) - 1)))`

                  where alpha and beta are constant scalars.
                  [default: alpha = 1.6733, beta = 1.0507]
                  Parameter of Selu is ignored in backend implementation.
                - 'ThresoldedRelu': Thresholded ReLU function, defined as: 

                  `f(x) = (x >= alpha) * x`

                  where alpha is a constant scalar.
                  [default: alpha = 1.0]
                - 'LeakyRelu': leaky relu function, defined as: 

                  `f(x) = (x >= 0) * x + (x < 0) * alpha * x`

                  where alpha is a constant scalar.
                  [default: alpha = 1.0]
                - 'Linear': linear function.
                
                   `f(x) = alpha * x + beta`    

                  where alpha and beta are constant scalars.
                  [default: alpha = 1.0, beta=1.0]

        input_name: str
            The name of the input_node

        name: str
            The name of this node

        attr: dict
            Parameters for the activation, depending on activation function chosen.

                - When activation is one of ['Relu', 'Sigmoid', 'Tanh'], attr is ignored.
                - When activation is one of ['Selu', 'HardSigmoid', 'Linear'], 'alpha' and 'beta' would be read from attr
                - When activation is one of ['Elu', 'LeakyRelu', 'ThresholdedRelu'], 'alpha' would be read from attr

        """
        attr['alpha'] = 1.0
        attr['beta'] = 1.0
        if 'op' == 'Selu':
            attr['alpha'] = 1.6732632423543772848170429916717
            attr['beta'] = 1.0507009873554804934193349852946

        return self._build_op(op, [input_name], name=name, attr=attr)

    def add_elementwise(self, op, inputs, name=None):
        """
        Add simple operation node that take 1 or 2 inputs.

        SSANode form:
            op: op
            inputs: inputs

        Parameters
        ----------
        op: str
            The operation performed on this node.
            Elementwise operations support broadcasting.
            Assume inputs = [A] for unary operations.
            Assume inputs = [A, B] for binary operations.

            It can be one of the following:
            
            Unary operations:

                - 'Cos': `f(A) = Cos(A)`

                - 'Sin': `f(A) = Sin(A)`

                - 'Square': `f(A) = A * A`

                - 'Sqrt': `f(A) = sqrt(A)`

                - 'Rsqrt': `f(A) = 1.0 / sqrt(A)`

                - 'Log': `f(A) = log(A)`

                - 'Neg': `f(A) = -A`

                - 'Floor': `f(A) = int(A)`

                - 'LogicalNot': `f(A) = !A`

            Binary operations:

                - 'Add': `f(A, B) = A + B`

                - 'Sub': `f(A, B) = A - B`

                - 'Mul': `f(A, B) = A * B`

                - 'RealDiv': `f(A, B) = A / B`

                - 'FloorDiv': `f(A, B) = floor(A / B)`

                - 'Pow': `f(A, B) = A ^ B`

                - 'Maximum': `f(A, B) = (A >= B) ? A : B`

                - 'Minimum': `f(A, B) = (A < B) ? A : B`

                - 'LogicalAnd': `f(A, B) = (A && B)`

                - 'LogicalOr': `f(A, B) = (A || B)`

                - 'Equal': `f(A, B) = (A == B)`

                - 'NotEqual': `f(A, B) = (A != B)`

                - 'Less': `f(A, B) = (A < B)`

                - 'LessEqual': `f(A, B) = (A <= B)`

                - 'Greater': `f(A, B) = (A > B)`

                - 'GreaterEqual': `f(A, B) = (A >= B)`


        input_names: [str or np.array of np.float32]
            len(inputs) must be less or equal than 2.

        name: str
            The name of this node

        """
        input_names = [self._maybe_add_const(input, "elementwise_input") \
                       for input in inputs]
        return self._build_op(op, input_names, name=name)

    def add_reshape(self, input_name, shape, name=None, attr={}):
        """
        Add a reshape blob that reshapes input_name to shape.

        SSANode form:
            op: 'Reshape'
            inputs: [input_name, shape]

        Parameters
        ----------
        input_name: str
            Name of the input blob that would be reshaped.

        shape: str
            Name of the input blob that indicates the output shape.

        name: str
            The name of this node

        """
        return self._build_op('Reshape', [input_name, shape], name=name)

    def add_matmul(self, input_names, name=None, attr={}):
        """
        Add matrix multiplication node.
        Take input_names [A, B], and rank(A) == rank(B).
        If rank(A) > 2, it's a batch MatMul (supports broadcasting).

        SSANode form:
            op: 'MatMul'
            inputs: input_names
            attr: 'transpose_a', 'transpose_b'

        Parameters
        ----------
        input_names: [str]
            len(input_names) must equal to 2.

        name: str
            The name of this node

        attr: dict
            - 'transpose_a': Transpose input_names[0]
            - 'transpose_b': Transpose input_names[1]
        """
        return self._build_op('MatMul', input_names, name=name, attr=attr)

    def add_tile(self, input_name, multiples, name=None):
        """
        Add Tile node.
        This operation creates a new tensor by replicating input multiples times.
        The output tensor's i'th dimension has input.dims(i) * multiples[i] elements,
        and the values of input are replicated multiples[i] times along the 'i'th dimension.
        For example, tiling [a b c d] by [2] produces [a b c d a b c d].

        SSANode form:
            op: 'Tile'
            inputs: [input_name, multiples]

        Parameters
        ----------
        input_name: str
            Name of the input that will be tiled.

        multiples: str
            Name of the node that indicates how many time will input be tiled.

        name: str
            The name of this node
        """
        return self._build_op('Tile', [input_name, multiples], name=name)

    def add_topk(self, input_name, k, name=None):
        """
        Add a topK node.
        Obtain the top-k along last axis.

        SSANode form:
            op: 'TopKV2'
            inputs: [input_name, k]

        Parameters
        ----------
        input_name: str
            Name of the input blob.

        k: str
            Name of the node that give top-k to extract.

        name: str
            The name of this node

        Examples
        --------
        .. sourcecode:: python

            # The tensor we are going to obtain topK
            >>> value = builder.add_const(np.random.random(size=(20,10)))

            # The value K for top-K.
            >>> k = builder.add_const(np.int32(3))

            # split_size we want.
            >>> topK = builder.add_topk(value, k)
            
            # returns of topK
            # The top-K values are in the 0-th index
            >>> topk_values = builder.add_get_tuple(topK, 0)
            # The top-K indices are in the 1-st index
            >>> topk_indices = builder.add_get_tuple(topK, 1)
        """
        return self._build_op('TopKV2', [input_name, k], name=name)

    def add_concat(self, input_names, axis, name=None):
        """
        Add Concat node.
        Take list of node inputs and a node representing the axis: a, b, axis.
        Equivalent to numpy.concatenate(input_names, axis=axis)

        SSANode form:
            op: 'ConcatV2'
            inputs: input_names + [axis]

        Parameters
        ----------
        input_names: [str or list[int] or np.ndarray]
            List of nodes that will be concatenated.

        axis: str or int
            Name of the node indicating the axis.

        name: str
            The name of this node
        """
        axis = self._maybe_add_const(axis, "concat_axis")
        input_names = [self._maybe_add_const(input, "concat_input") for input in input_names]
        return self._build_op('ConcatV2', input_names + [axis], name=name)

    def add_const(self, value, name=None):
        """
        Add a constant node.

        SSANode form:
            op: 'Const'
            inputs: []
            value: NNSSA.builtins
            datatype: NNSSA.builtins type

        Parameters
        ----------
        value: NNSSA.builtins types or numpy type.
            The value of the constant.

        name: str
            The name of this node
        """
        if isinstance(value, (np.generic, np.ndarray)):
            value, valtype = numpy_val_to_builtin_val(value)

        return self._build_op('Const', [], name=name, value=value, datatype=value.__class__)

    def add_select(self, cond, b_true, b_false, name=None):
        """
        Add a select node. i.e. The "if" statement.

        SSANode form:
            op: 'Select'
            inputs: [condition, b_true, b_false]

        Parameters
        ----------
        cond: str
        Condition node to determine which branch to output.

        b_true: str
        If cond is true, output of node would be this true branch.

        b_false: str
        Otherwise, result of node would be this false branch.

        name: str
            The name of this node
        """

        return self._build_op('Select', [cond, b_true, b_false], name=name)

    def add_split(self, split_dim, value, split_size="", num_split=0, name=None):
        """
        Add a split node.
        If num_split != 0, split_size will be ignored.
        If num_split == 0, split_size is the node name that gives how value should be split.

        Obtain result by get_tuple.

        SSANode form:
            op: 'Split'
            inputs: [split_dim, value]
            attr: 'num_split'

            op: 'SplitV'
            inputs: [value, split_size, split_dim]

        Parameters
        ----------
        split_dim: str
            Input node which indicates the splitting axis.
            This needs to be a constant node after all constant propagations.

        split_size: str
            Input node of a tensor indicating the size of each split.

        value: str
            Input node which will be splitted.

        num_split: int
            Number of splits (evenly splitting).

        name: str
            The name of this node

        Examples
        --------
        .. sourcecode:: python

            # The axis we are going to split
            >>> axis = builder.add_const(np.int(0))

            # A node we want to split.
            >>> value = builder.add_const(np.random(size=[30, 1, 512]))

            # split_size we want.
            >>> split_size_1 = builder.add_const(np.array([5, 10, 15]))
            >>> split_size_2 = builder.add_const(np.array([5, 10, 16]))
            
            # Valid split.
            >>> custom_split_1 = builder.add_split(axis, value, split_size=split_size_1)
            # custom_output_1_0 has shape = [5, 1, 512]
            >>> custom_output_1_0 = builder.add_get_tuple(custom_split_1, 0)
            # custom_output_1_1 has shape = [10, 1, 512]
            >>> custom_output_1_1 = builder.add_get_tuple(custom_split_1, 1)
            # custom_output_1_2 has shape = [15, 1, 512]
            >>> custom_output_1_2 = builder.add_get_tuple(custom_split_1, 2)

            # Invalid split. sum(split_size_2) != value.shape[axis]
            >>> custom_split_2 = builder.add_split(axis, value, split_size=split_size_2)

            # Valid split.
            >>> even_split_1 = builder.add_split(axis, value, num_split=3)
            # even_output_1_0 has shape = [10, 1, 512]
            >>> even_output_1_0 = builder.add_get_tuple(even_split_1, 0)
            # even_output_1_1 has shape = [10, 1, 512]
            >>> even_output_1_1 = builder.add_get_tuple(even_split_1, 1)
            # even_output_1_2 has shape = [10, 1, 512]
            >>> even_output_1_2 = builder.add_get_tuple(even_split_1, 2)

            # Invalid split. (value.shape[axis] % num_split) != 0
            >>> even_split_2 = builder.add_split(axis, value, num_split=4)
        """
        if num_split > 0:
            return self._build_op(
                'Split', [split_dim, value], name=name, attr={'num_split': num_split})
        else:
            return self._build_op('SplitV', [value, split_size, split_dim], name=name)

    def add_slice(
            self, input_name, begin=None, end=None, size=None, strides=None, squeeze=[], name=None):
        """
        Add a slice node.

        SSANode form:
            op: 'Slice'
            inputs: [input_name, begin, size]

            op: 'StridedSlice'
            inputs: [input_name, begin, end, strides]
            attr: 'shrink_axis_mask'

        Parameters
        ----------
        input_name: str
            Input node to slice from.

        begin: str or list of int
            The beginning index for slicing.
            Should have same length as the shape of input node.

        end: str or list of int
            The ending index for slicing.
            Should have same length as the shape of input node.

        size: str or list of int
            The size to slice for each axis.
            Should have same length as the shape of input node.

        strides: str
            The slicing stride for each axis.
            Should have same length as the input node.
            Need to be used with parameter 'begin' and 'end'.

        squeeze: [int]
            Axes that would be squeezed out.
            Need to be used with parameter 'begin' and 'end'.

        name: str
            The name of this node

        Attributes
        ----------
        shrink_axis_mask: int
            A binary mask used for determine whether to squeeze out some axis.
            Implementing it by binary masking.
            Useful for such slices: A[:, 3, :] (i.e. squeeze out axis at 1)
            Which would have mask value 0*2^0 + 1*2^1 + 0*2^2 = 2

        Examples
        --------
        .. sourcecode:: python

            # A tensor of shape (3, 2, 3)
            >>> value = np.array([[[1, 1, 1], [2, 2, 2]],
            >>>                   [[3, 3, 3], [4, 4, 4]],
            >>>                   [[5, 5, 5], [6, 6, 6]]])
            >>> A = builder.add_const(value)
            # Slicing out a fix size
            >>> begin = builder.add_const(np.array([1, 0, 0]))
            >>> size1 = builder.add_const(np.array([1, 2, 3]))
            >>> size2 = builder.add_const(np.array([2, 1, 3]))
            # Returns [[[3, 3, 3],
            #           [4, 4, 4]]]
            >>> ret1 = builder.add_slice(begin=begin, size=size1)
            # Returns [[[3, 3, 3]],
            #          [[5, 5, 5]]]
            >>> ret2 = builder.add_slice(begin=begin, size=size2)

            # Slicing out with pythonic-style
            >>> begin = builder.add_const(np.array([1, -1, 0]))
            >>> end = builder.add_const(np.array([2, -3, 3]))
            >>> strides = builder.add_const(np.array([1, -1, 1]))
            # Same as value[1:2, -1:-3:-1, 0:3]
            # Returns [[[4, 4, 4],
            #           [3, 3, 3]]]
            >>> ret3 = builder.add_slice(begin=begin, end=end, strides=strides)
            # Same as value[1, -1:-3:-1, 0:3]
            # Returns [[4, 4, 4],
            #          [3, 3, 3]]
            >>> ret4 = builder.add_slice(begin=begin, end=end, strides=strides, squeeze=[0])

        """
        begin = self._maybe_add_const(begin, "slice_begin")
        end = self._maybe_add_const(end, "slice_end")
        size = self._maybe_add_const(size, "slice_size")
        strides = self._maybe_add_const(strides, "slice_strides")
        if end is not None and size is not None:
            raise ValueError("end and size parameter in Slice cannot be used simultaneously.")
        if strides is not None and size is not None:
            raise ValueError("stride and size parameter in Slice cannot be used simultaneously.")

        if size is not None:
            return self._build_op('Slice', [input_name, begin, size], name=name)
        else:
            squeeze_mask = 0
            for i in squeeze:
                squeeze_mask += 2 ** i
            return self._build_op(
                'StridedSlice', [input_name, begin, end, strides],
                attr={'shrink_axis_mask': squeeze_mask},
                name=name)

    def add_reduction(self, op, input_name, axis=None, name=None, attr={'keep_dims': False}):
        """
        Add a reduction operation.

        SSANode form:
            op: op
            inputs: [input_name, axis]
            attr: 'keep_dims'

        Parameters
        ----------
        op: str
            The operation performed on this node.
            It can be 'Mean', 'Sum', 'Max', 'Min', 'Prod' or 'ArgMax'.

        input_name: str
            The name of the input_node

        axis (optional): list of int or node name (str)
            The axis that the operation would be performed.

        name: str
            The name of this node

        attr: dict
            Attributes for reduction node.
            If 'keep_dims' is set to True, the rank would not change, otherwise,
            rank(output) = rank(input)-1
        """
        axis = self._maybe_add_const(axis, "reduction_axis")
        return self._build_op(op, [input_name, axis], name=name, attr=attr)

    def add_squeeze(self, input_name, squeeze_dims=[], name=None):
        """
        Remove axes with size of 1.

        SSANode form:
            op: 'Squeeze'
            inputs: [input_name]
            attr: 'squeeze_dims'

        Parameters
        ----------
        input_name: str
            The name of the input_node

        squeeze_dims: [int]
            The axes that are going to be squeezed. If an empty list, all axis
            with shape=1 will be squeezed out.

        name: str
            The name of this node
        """
        return self._build_op(
            'Squeeze', [input_name], name=name, attr={'squeeze_dims': squeeze_dims})

    def add_expanddims(self, input_name, expand_dim, name=None):
        """
        Expand rank on the expand_dims axes.

        SSANode form:
            op: 'ExpandDims'
            inputs: [input_name, expand_dims]

        Parameters
        ----------
        input_name: str
            The name of the input_node

        expand_dim: str or int
            The name of the node indicating which axis is going to be expanded.
            This node needs to be constant after constant propagation.

        name: str
            The name of this node
        """
        expand_dim = self._maybe_add_const(expand_dim, "expanddim_axis")
        return self._build_op('ExpandDims', [input_name, expand_dim], name=name)

    def add_softmax(self, input_name, name=None):
        """
        Do softmax on the last axis (axis "-1")
        If axis is not the last, transpose first.

        SSANode form:
            op: 'Softmax'
            inputs: [input_name]

        Parameters
        ----------
        input_name: str
            The name of the input_node

        name: str
            The name of this node
        """
        return self._build_op('Softmax', [input_name], name=name)

    def add_logsoftmax(self, input_name, name=None):
        """
        Do logsoftmax on the last axis (axis "-1")
        If axis is not the last, transpose first.

        SSANode form:
            op: 'LogSoftmax'
            inputs: [input_name]

        Parameters
        ----------
        input_name: str
            The name of the input_node

        name: str
            The name of this node
        """
        return self._build_op('LogSoftmax', [input_name], name=name)

    def add_shape(self, input_name, attr=None, name=None):
        """
        Get shape of the input node.

        SSANode form:
            op: 'Shape'
            inputs: [input_name]

        Parameters
        ----------
        input_name: str
            The name of the input_node

        name: str
            The name of this node
        """
        if attr is None:
            attr = {}
        return self._build_op('Shape', [input_name], attr=attr, name=name)

    def add_transpose(self, input_name, axes, name=None):
        """
        Do a transpose on input_name of axes.
        Equivalent of numpy.transpose(input_name, axes)

        SSANode form:
            op: 'Transpose'
            inputs: [input_name, axes]

        Parameters
        ----------
        input_name: str
            The name of the input_node

        axes: str
            The name of the axes blobs.

        name: str
            The name of this node
        """
        return self._build_op('Transpose', [input_name, axes], name=name)

    def add_fill(self, shape, value, name=None):
        """
        This node outputs a tensor with shape filled with value.

        SSANode form:
            op: 'Fill'
            inputs: [shape, value]

        Parameters
        ----------
        shape: str
            The shape of the tensor of this node's output.

        value: str
            The name of the blob that consists the value being filled.

        name: str
            The name of this node
        """
        return self._build_op('Fill', [shape, value], name=name)

    def add_gather(self, A, indices, axis=0, name=None):
        """
        Gather node. Often used as embedding layers.
        Equivalent to numpy.take(A, indices, axis)

        SSANode form:
            op: 'Gather'
            inputs: [A, indices, axis=0]
            attr: 'axis'

        Parameters
        ----------
        A: str
            The name of the input_node, which "take" the indices slices.

        indices: str or int or list[int]
            The name of input node of indices.

        axis: str or int
            If str, it would be a node that indicates the axis to take.
            If int, this would just be the "Const" used for take.

        name: str
            The name of this node
        """
        indices = self._maybe_add_const(indices, "gather_indices")
        if isinstance(axis, int):
            return self._build_op('Gather', [A, indices], attr={'axis': axis}, name=name)
        else:
            return self._build_op('Gather', [A, indices, axis], name=name)

    def add_while(self, input_name, body_function, cond_function, name=None):
        """
        Add while node into the graph.

        SSANode form:
            op: 'while'
            inputs: [input_name]
            attr: 'body_function', 'cond_function'

        Parameters
        ----------
        input_name: str
            The name of the input_node, should be type of "make_tuple"

        body_function: str
            The body function's name.

        body_function: str
            The cond function's name.

        name: str
            The name of this node

        Examples
        --------
        .. sourcecode:: python

            # Let's build a Network that would be performing:
            # 
            # target = some_user_input
            # i = 0
            # while (i < target):
            #     i += 1
            # 
            # print(i)

            >>> converter = coremlconverters.NNSSAConverter()

            # We should try to "append" the prefix if we are building from graph level.
            # No two nodes should have same name in a NNSSA, but it's impossible to
            # check if you "bottom-top" build from GraphBuilder.
            >>> sb = coremlconverters.SSABuilder()
            # The graph builder for the main graph
            >>> gb = coremlconverters.GraphBuilder(prefix="main_")
            # The graph builder for the body function
            >>> body_builder = coremlconverters.GraphBuilder(prefix="body_")
            # The graph builder for the condition function
            >>> cond_builder = coremlconverters.GraphBuilder(prefix="cond_")

            # Let's build the main graph first.
            >>> i = gb.add_const(np.int32(0), name="i")
            >>> target = gb.add_placeholder(init_from=np.int32(5), name="target")
            # The input of the while loop needs to be a single "make_tuple" node.
            # All condition variable and input to body function should be packed here.
            >>> mt = gb.add_make_tuple([target, i], name="make_tuple_0")
            # while node takes the tuple node as input, and has the "function names" of body/cond as attributes.
            # The input "mt" here will pass through to both body and cond function_entry.
            >>> loop = gb.add_while(mt, "body_function_0", "cond_function_0", name="loop")
            # The output of the while loop needs to be a single "make_tuple" node.
            >>> out = gb.add_get_tuple(loop, index=1, name="out")

            # We need a function_entry for every function created.
            >>> b_entry = body_builder.add_function_entry(name="body_function_0")
            >>> add_one = body_builder.add_const(np.int32(1), name="one")
            # The function_entry passed in (which is mt above) is a tuple.
            >>> to_add = body_builder.add_get_tuple(b_entry, index=1)
            >>> target = body_builder.add_get_tuple(b_entry, index=0)
            >>> added = body_builder.add_elementwise("Add", [to_add, add_one])
            # We also need to pack the returns into a tuple.
            # Note that the "return" value here will be passed to body and cond's function_entry every itereation.
            >>> ret = body_builder.add_make_tuple([target, added])
            >>> body_builder.add_return(ret)

            >>> c_entry = cond_builder.add_function_entry(name="cond_function_0")
            >>> now = cond_builder.add_get_tuple(c_entry, index=1)
            >>> target = cond_builder.add_get_tuple(c_entry, index=0)
            >>> stop = cond_builder.add_elementwise("Less", [now, target], name="cond")
            # Unlike the return of body function, which will be passed back to function_entry, cond returns a bool.
            >>> cond = cond_builder.add_return(stop)

            # Let's add everything up.
            >>> sb.add_graph(gb.get_graph())
            >>> sb.add_function(body_builder.get_function(), name="body_function_0")
            >>> sb.add_function(cond_builder.get_function(), name="cond_function_0")

            # This is the NNSSA graph for a while loop.
            >>> ssa = sb.get_ssa()

        """
        return self._build_op(
            'while', [input_name],
            name=name,
            attr={
                'body_function': body_function,
                'cond_function': cond_function
            })

    def add_select(self, cond, true_branch, false_branch, name=None):
        """
        A select node. Behaves like an "if statement".

        SSANode form:
            op: 'Select'
            inputs: [cond, true_branch, false_branch]

        Parameters
        ----------
        cond: str
            The name of the condition blob.

        true_branch: str
            The name of the blob that the node would output when cond is true.

        false_branch: str
            The name of the blob that the node would output when cond is false.

        name: str
            The name of this node
        """
        return self._build_op('Select', [cond, true_branch, false_branch], name=name)

    def add_range(self, start=None, stop=None, step=None, name=None):
        """
        Construct a range tensor.
        Equivalent to numpy.arange

        SSANode form:
            op: 'Range'
            inputs: [stop, step]

            op: 'Range'
            inputs: [start, stop, step]

        Parameters
        ----------
        start: str or int
            (Optional) The name of the node that provides the starting index.
            Default is 0.

        stop: str or int
            The name of the node that provides the ending index.

        step: str or int
            The name of the node that provides the step size.

        name: str
            The name of this node
        """
        input_names = []
        start = self._maybe_add_const(start, "range_start")
        stop = self._maybe_add_const(stop, "range_stop")
        step = self._maybe_add_const(step, "range_step")
        if start is not None:
            input_names.append(start)
        input_names = input_names + [stop, step]

        return self._build_op('Range', input_names, name=name)

    def add_shape(self, input_name, name=None):
        """
        Output the shape of the input blob.

        SSANode form:
            op: 'Shape'
            inputs: [input_name]

        Parameters
        ----------
        input_name: str
            The name of the input blob.

        name: str
            The name of this node
        """
        return self._build_op('Shape', [input_name], name=name)

    def add_rank(self, input_name, name=None):
        """
        Output the rank of the input blob.

        SSANode form:
            op: 'Rank'
            inputs: [input_name]

        Parameters
        ----------
        input_name: str
            The name of the input blob.

        name: str
            The name of this node
        """
        return self._build_op('Rank', [input_name], name=name)

    def add_padding(self, input_name, paddings, constant_values=0.0, name=None):
        """
        Pads a tensor.

        SSANode form:
            op: 'Pad'
            inputs: [input_name, paddings]
            attr: 'constant_values'

        Parameters
        ----------
        input_name: str
            The name of the input_node

        paddings: str
            The name of the paddings spec.
            This should be a constant.

        constant_values: float
            Constant value for padding.

        name: str
            The name of this node
        """
        return self._build_op(
            'Pad', [input_name, paddings], name=name, attr={'constant_values': constant_values})

    def add_pooling(
            self,
            input_name,
            ksize,
            strides,
            pooling_type,
            padding="SAME",
            data_format="NHWC",
            name=None):
        """
        Add pooling node for SSAfunctions.

        SSANode form:
            op: 'MaxPool'
            inputs: [input_name]
            attr: 'ksize', 'strides', 'padding', 'data_format'

            op: 'AvgPool'
            inputs: [input_name]
            attr: 'ksize', 'strides', 'padding', 'data_format'

        Parameters
        ----------
        input_name: str
            The name of the input_node

        ksize: [int]
            A list of 4 integers.
            The size of the window for each dimension of the input tensor.

        strides: [int]
            A list of 4 integers.
            The stride of the sliding window for each dimension of the input tensor.

        pooling_type: str
            Could be "MAX" or "AVG".

        padding: str
            Could be "SAME" or "VALID".

        data_format: str
            Could be "NHWC" or "NCHW".

        name: str
            The name of this node
        """
        attr = {}
        attr['ksize'] = ksize
        attr['strides'] = strides
        attr['padding'] = padding
        attr['data_format'] = data_format
        if pooling_type == 'MAX':
            return self._build_op('MaxPool', [input_name], attr=attr, name=name)
        elif pooling_type == 'AVG':
            return self._build_op('AvgPool', [input_name], attr=attr, name=name)
        else:
            raise ValueError("Pooling type unsupported")

    def add_conv2D(
            self, input_name, filter_name, strides, padding="SAME", data_format="NHWC", name=None):
        """
        Add pooling node for SSAfunctions.

        SSANode form:
            op: 'Conv2D'
            inputs: [input_name, filter_name]
            attr: 'strides', 'padding', 'data_format'

        Parameters
        ----------
        input_name: str
            The name of the input_node, should be a 4D input.

        filter_name: str
            Should be a 4D input.
            A 4D tensor of shape [filter_height, filter_width, in_channels, out_channels]

        strides: [int]
            A list of 4 integers.
            The stride of the sliding window for each dimension of the input tensor.

        padding: str
            Could be "SAME" or "VALID".

        data_format: str
            Could be "NHWC" or "NCHW".

        name: str
            The name of this node
        """
        attr = {}
        attr['strides'] = strides
        attr['padding'] = padding
        attr['data_format'] = data_format
        attr['dilations'] = [1, 1, 1, 1]

        return self._build_op('Conv2D', [input_name, filter_name], attr=attr, name=name)

    def add_function_entry(self, name=None):
        """
        Add function_entry node for SSAfunctions.
        This is the *input*, so no input should be directed here.
        The output nodes of this op should always be get_tuple.

        SSANode form:
            op: 'function_entry'
            inputs: []

        Parameters
        ----------
        name: str
            The name of this node
        """
        return self._build_op('function_entry', [], name=name)

    def add_return(self, input_name, name=None):
        """
        Add return node for SSAfunctions.

        SSANode form:
            op: 'return'
            inputs: [input_name]

        Parameters
        ----------
        input_name: str
            The name of the input_node, should be type of "make_tuple"

        name: str
            The name of this node
        """
        return self._build_op('return', [input_name], name=name)

    def _maybe_add_const(self, var, var_name=None):
        """
        If `var` is int, float, or list, add a const node and return the ssa
        name of the added const node. If `var` is str or None, return `var`
        (no-op)

        var_name (str): The SSA name of the new const node, if created, will
        be `var_name` + incrementing counter.
        """
        if isinstance(var, str) or var is None:
            return var
        if sys.version_info < (3, 0) and isinstance(var, unicode):
            return var
        if not var_name:
            var_name = "generated_const"
        if not hasattr(self, var_name):
            setattr(self, var_name, 0)
        node_name = self.prefix + var_name + str(getattr(self, var_name))
        setattr(self, var_name, getattr(self, var_name) + 1)
        if isinstance(var, int):
            return self.add_const(np.int32(var), name=node_name)
        if isinstance(var, float):
            return self.add_const(np.float32(var), name=node_name)
        if isinstance(var, list):
            return self.add_const(np.asarray(var), name=node_name)
        if isinstance(var, np.ndarray):
            return self.add_const(var, name=node_name)
        raise RuntimeError("Unable to create const node for " + str(var))

    def add_LSTMBlock(
            self,
            input_vec,
            W,
            b,
            prev_h=None,
            prev_cs=None,
            mode='cell',
            forget_bias=0.0,
            time_major=True,
            bidirectional=False,
            output_all_states=True,
            name=None):
        """
        Build a LSTM Block.

        LSTM Cell performs the following operation:
            xh = [x, prev_h]
            [i, ci, f, o] = xh * W + b
            f = f + forget_bias
            i = sigmoid(i)
            f = sigmoid(f)
            ci = tanh(ci)
            cs = ci .* i + prev_cs .* f
            o = sigmoid(o)
            co = tanh(cs)
            h = co .* o

        SSANode form:
            op: 'LSTMBlock'
            inputs: [input_vec, W, b, prev_h, prev_cs]
            attr: 'mode', 'forget_bias', 'time_major', 'bidirectional', 'output_all_states'

        Examples
        --------
        .. sourcecode:: python

            # A sample for the LSTMBlock that can be used for the encoder (static length if input given)
            # Setup for the hidden size and input size
            >>> hidden_size = 8
            >>> input_size = 15
            >>> ph_encoder = builder.add_placeholder(datatype=builtins.tensor(builtins.fp32, [5, 1, 15]), name="ph_encoder")
            >>> W_val = np.random.random(size=(input_size+hidden_size,4*hidden_size)).astype(np.float32)
            # The weight matrix
            >>> W = builder.add_const(W_val)
            # The bias vector
            >>> b = builder.add_const(np.zeros(shape=[4*hidden_size]).astype(np.float32))
            # The previous cell state and hidden state can be None if it is in encoder mode.
            >>> prev_cs_encoder = None
            >>> prev_h_encoder = None

            >>> lstm_encoder = builder.add_LSTMBlock(ph_encoder,
            >>>                                      W,
            >>>                                      b,
            >>>                                      prev_h=prev_h_encoder,
            >>>                                      prev_cs=prev_cs_encoder,
            >>>                                      mode="encoder")

            # Fetch the output through get_tuple.
            >>> o = builder.add_get_tuple(lstm_encoder, index=0, name="o")
            >>> h = builder.add_get_tuple(lstm_encoder, index=1, name="h")
            >>> c = builder.add_get_tuple(lstm_encoder, index=2, name="c")

            # Similarly, we can just use cell for each timestamp if we want.
            # The input is [batch_size, input_size] without the sequence length.
            >>> ph_cell = builder.add_placeholder(datatype=builtins.tensor(builtins.fp32, [1, 15]), name="ph_cell")
            # The previous cell state and hidden state must be set if it is in cell mode.
            >>> prev_cs_cell = gb.add_const(np.zeros(shape=[hidden_size]).astype(np.float32))
            >>> prev_h_cell = gb.add_const(np.zeros(shape=[hidden_size]).astype(np.float32))

            # We just reuse weight/bias/sizes with the encoder over here.
            >>> lstm_cell = builder.add_LSTMBlock(ph_cell,
            >>>                                   W,
            >>>                                   b,
            >>>                                   prev_h=prev_h_cell,
            >>>                                   prev_cs=prev_cs_cell,
            >>>                                   mode="cell")

            # Fetch the output through get_tuple. 'o' is dummy in cell mode.
            >>> _ = builder.add_get_tuple(lstm_cell, index=0)
            >>> h = builder.add_get_tuple(lstm_cell, index=1, name="h")
            >>> c = builder.add_get_tuple(lstm_cell, index=2, name="c")

        Parameters
        ----------
        input_vec: str
            The input to the LSTM Block.
            Shape of input_vec should be:
                - mode == 'cell':
                    (batch size, input size)
                - mode == 'encoder':
                    time_major == True:  (seq_len, batch size, input size)
                    time_major == False: (batch size, seq_len, input size)

        W: str [input_size + hidden_size, {4, 8} * hidden_size]
            The weight matrix.
            Concatenation of [W_i, W_g, W_f, W_o], see notes on how the order should be.
            If bidirectional encoder is being used, we will have W as:
                W = np.concatenate([W_fw, W_bw], axis=-1)
            Shape should be:
                - mode == 'cell':
                    (input_size + hidden_size, 4 * hidden_size)
                - mode == 'encoder':
                    bidirectional == True:  (input_size + hidden_size, 8 * hidden_size)
                    bidirectional == False: (input_size + hidden_size, 4 * hidden_size)

        b: str [4 * hidden_size]
            The bias vector.

        prev_h: str [batch_size, {1, 2} * hidden_size], optional
            Output of the previous cell at previous time step.
            If mode == 'encoder', this is optional and will be set to zero-state if not provided.
            If bidirectional is True, concatenation should be done on last axis.

        prev_cs: str [batch_size, {1, 2} * hidden_size], optional
            Value of the cell state at previous timestamp.
            If mode == 'encoder', this is optional and will be set to zero-state if not provided.
            If bidirectional is True, concatenation should be done on last axis.

        name: str
            The name of this node

        mode: str
            'cell' or 'encoder'

        forget_bias: int
            See notes. Whether there's a bias for the forget gate.

        time_major: bool
            See notes. Only valid when mode == 'encoder'. Default True.

        bidirectional: bool
            See notes. Only valid when mode == 'encoder'. Default False.

        output_all_states: bool
            See output notes. Only valid when mode == 'encoder'. Default True.

        Outputs
        -------
        Need to use get_tuple to obtain outputs.

        - mode == 'cell':
            [Output_state, Hidden_state, Cell_state]
        - mode == 'encoder':
            - bidirectional == False
                [Output_state, Hidden_state, Cell_state]
            - bidirectional == True
                [Output_state, Hidden_state_fwd, Cell_state_fwd,
                Hidden_state_bck, Cell_state_bck]

            Output_state has shape (assuming time_major = True)
                - output_all_states = True: [seq_len, batch_size, {1, 2} * hidden_size]
                - output_all_states = False: [1, batch_size, {1, 2} * hidden_size]
            Hidden_state*, Cell_state* both have shape [1, batch_size, hidden_size]
        """

        attr = dict()
        attr['mode'] = mode
        attr['forget_bias'] = forget_bias
        if mode == 'encoder':
            attr['time_major'] = time_major
            attr['bidirectional'] = bidirectional
            attr['output_all_states'] = output_all_states

        inputs = [input_vec, W, b]
        if mode == 'cell':
            inputs += [prev_h, prev_cs]
        return self._build_op('LSTMBlock', inputs, attr=attr, name=name)
