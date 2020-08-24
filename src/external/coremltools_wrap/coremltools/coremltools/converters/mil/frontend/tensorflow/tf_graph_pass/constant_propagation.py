# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import logging
import tensorflow as tf
import gc
from .delete_constant import delete_unnecessary_constant_nodes
from ..basic_graph_ops import const_determined_nodes, delete_node, disconnect_edge
from coremltools.converters.mil.mil import types
from coremltools.converters.mil.mil.types.type_mapping import numpy_val_to_builtin_val
from coremltools.converters._profile_utils import _profile
from distutils.version import StrictVersion as _StrictVersion


def _get_const_nodes(fn):
    from tensorflow.core.framework import graph_pb2
    from tensorflow.core.framework import node_def_pb2

    new_graph = graph_pb2.GraphDef()
    constant_nodes = set()
    constant_node_num_outputs = {}
    generated_nodes = [k for k, v in fn.graph.items() if v.original_node is None]
    const_nodes_in_this_graph = const_determined_nodes(fn.graph, set(generated_nodes))
    # we can only run TF on nodes with outputs since we must evaluate
    # tensors and not ops
    const_nodes_in_this_graph = [
        i for i in const_nodes_in_this_graph if fn.graph[i].op != "NoOp"
    ]
    constant_nodes = constant_nodes.union(set(const_nodes_in_this_graph))

    # topological sort const nodes
    topsort = []
    topsort_set = set()
    while len(const_nodes_in_this_graph) > 0:
        for n in const_nodes_in_this_graph:
            input_names = fn.graph[n].inputs
            if len(set(input_names).difference(topsort_set)) == 0:
                topsort.append(n)
                topsort_set.add(n)

        const_nodes_in_this_graph = set(const_nodes_in_this_graph).difference(
            topsort_set
        )

    for node in topsort:
        new_node = node_def_pb2.NodeDef()
        new_node.CopyFrom(fn.graph[node].original_node)
        if "_class" in new_node.attr:
            del new_node.attr["_class"]
        del new_node.input[:]
        new_node.input.extend(fn.graph[node].inputs)
        if "_output_shapes" in fn.graph[node].attr:
            constant_node_num_outputs[node] = len(fn.graph[node].attr["_output_shapes"])
        else:
            constant_node_num_outputs[node] = 1
        new_graph.node.extend([new_node])
        del new_node
    gc.collect()
    return new_graph, list(constant_nodes), constant_node_num_outputs


@_profile
def _constant_propagation(fn, new_graph, constant_nodes, constant_node_num_outputs):
    try:
        if len(constant_nodes) > 0:
            with tf.Graph().as_default() as graph:
                tf.import_graph_def(new_graph, name="")

                # We're only making one call to `sess.run()` in order to compute constant values.
                # In this context, the default optimization settings make everything dramatically
                # slower and more memory-intensive.
                if tf.__version__ < _StrictVersion("1.13.1"):
                    session_config = tf.ConfigProto()
                    session_config.graph_options.optimizer_options.opt_level = (
                        tf.OptimizerOptions.L0
                    )
                    sess = tf.Session(graph=graph, config=session_config)
                else:
                    session_config = tf.compat.v1.ConfigProto()
                    session_config.graph_options.optimizer_options.opt_level = (
                        tf.compat.v1.OptimizerOptions.L0
                    )
                    session_config.graph_options.rewrite_options.disable_meta_optimizer = (
                        True
                    )
                    sess = tf.compat.v1.Session(graph=graph, config=session_config)

                query_list = list()
                control_flow_ops = list()
                for c in constant_nodes:
                    for j in range(constant_node_num_outputs[c]):
                        query = c + ":" + str(j)
                        lower_query = query.lower()
                        if "switch" in lower_query or "cond" in lower_query:
                            control_flow_ops.append(query)
                        else:
                            query_list.append(query)
                result_list = sess.run(query_list)
                result = {
                    query_list[i]: result_list[i] for i in range(len(query_list))
                }
                # propagate switch one by one
                for op in control_flow_ops:
                    try:
                        res = sess.run([op])
                        result.update({op: res[0]})
                    except:
                        logging.warning(
                            '[Constant Propagation] Skip "dead" tensor: {}'.format(
                                op
                            )
                        )
                        result.update({op: None})

                sess.close()

            for k, v in fn.graph.items():
                if k in constant_node_num_outputs:
                    if constant_node_num_outputs[k] == 1:
                        result_entry = k + ":0"
                        try:
                            v.value, v.datatype = numpy_val_to_builtin_val(
                                result[result_entry]
                            )
                        except:
                            logging.error(result_entry)
                            logging.error(result[result_entry])
                    else:
                        values = [
                            result[k + ":" + str(i)]
                            for i in range(constant_node_num_outputs[k])
                        ]
                        try:
                            npval = [numpy_val_to_builtin_val(i) for i in values]
                            v.datatype = types.tuple(tuple([val[1] for val in npval]))
                            v.value = v.datatype()
                            for idx, val in enumerate(npval):
                                v.value.val[idx] = val[0]
                        except:
                            logging.error(values)
            for k, v in fn.graph.items():
                if v.op == "get_tuple":
                    inp = fn.graph[v.inputs[0]]
                    idx = v.attr["index"]
                    if inp.value is not None:
                        v.value = inp.value.val[idx]
                        v.datatype = inp.datatype.T[idx]

    except Exception as e:
        logging.exception("Constant Propagation pass failed: {}".format(e))


@_profile
def constant_propagation(tfssa):
    # we are going to rely on the TensorFlow graph to perform constant
    # propagation. For each graph, we construct a new graph comprising
    # only a subset of nodes that are constant nodes.

    for f in tfssa.functions.values():
        const_nodes_info = _get_const_nodes(f)
        _constant_propagation(f, *const_nodes_info)
    delete_unnecessary_constant_nodes(tfssa)
