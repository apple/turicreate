# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _


# A TensorArray is essentially a runtime vector<Tensor> with
#
#  - an optional requirement "infer_shape" (True by default) that all Tensors
#    stored within the vector have the same size/shape (inferred by the
#    first element stored into the tensor)
#  - an optional "element_shape" which requires all elements to have this
#    exact shape.
#  - an optional "clear_after_read" (True by default) where read of an index
#    is destructive. (It doesn't *really* destroy, but just enables a particular
#    optimization where the tensor memory can be reused).
#  - An optional "dynamic_size" (False by default) where the vector is resized
#    automatically at runtime
#
# The way it works is rather odd. To enforce "control dependency" constraints,
# a single float (flow) variable is passed between operations that write/read
# the TensorArray. Additionally, a "Resource" variable is also passed along
# which contains the actual handle to the TensorArray.
#
# The TensorArray can therefore also be passed around as as argument to while
# loops.  Thus unlike a global "Variable", this really is better thought of as
# an additional type, a list[tensor].
#
# See:
#
# https://github.com/tensorflow/tensorflow/blob/r1.6/tensorflow/python/ops/tensor_array_ops.py
# https://github.com/tensorflow/tensorflow/blob/master/tensorflow/core/kernels/tensor_array.h
# https://github.com/tensorflow/tensorflow/blob/master/tensorflow/core/kernels/tensor_array.cc
# https://github.com/tensorflow/tensorflow/blob/master/tensorflow/core/ops/data_flow_ops.cc
# https://github.com/tensorflow/tensorflow/blob/master/tensorflow/core/kernels/tensor_array_ops.cc
#
# The way we transform it is to introduce a new type. list[tensor]
# The flow variable is the list[tensor] since that is consistently passed through
# every operation.
# The 'resource' edges then gets passed as void.
#
# We would like to delete the resource edges, but once too many graph passes are
# performed, this becomes very difficult (since tuple shapes have to be updated).
# The ideal is to perform the resource edge deletion *BEFORE* any additional
# graph transformations.
# The conversion of the flow variable to list[tensor] can be performed during
# type inference.
#
#
# After this op:
# All nodes which take a TensorArray resource input will have the resource input
# edge deleted.
#
# TensorArrayV3 op will only have 1 output, a flow variable.


def tensor_array_resource_removal(gd):
    # this should be called *BEFORE* introduction of tuples,
    # and before output edges are added (for simplicity)
    for k, node in gd.items():
        if node.op.startswith("TensorArray") and node.op != "TensorArrayV3":
            # generally the resource edge is the first edge
            # input is resource, indices, flow
            # output is generally flow
            node.inputs = node.inputs[1:]

        # TensorArrayV3 node outputs resource and flow
        # shift all flow reads from TensorArray to output 0 of TensorArray
        for i in range(len(node.inputs)):
            if ":" in node.inputs[i]:
                input_node, input_index = node.inputs[i].split(":")
                input_index = int(input_index)
            else:
                input_node = node.inputs[i]
                input_index = 0
            if gd[input_node].op == "TensorArrayV3":
                if input_index == 1:
                    node.inputs[i] = "%s" % input_node
