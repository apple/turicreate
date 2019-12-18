import numpy as np
import copy

from ....builder import GraphBuilder
from ....commons import builtins
from ....commons.basic_graph_ops import replace_node, delete_node

from ..parsed_tf_node import ParsedTFNode


def expand_fusedbatchnorm_cell(graph, node):
    assert (node.op == 'FusedBatchNorm')
    assert (len(node.inputs) == 5)

    x, scale, offset, estimated_mean, estimated_variance = node.inputs

    epsilon = node.attr.get('epsilon', 1e-4)
    var_node = graph[estimated_variance]
    if var_node.value is not None:
        var_node.value.val += epsilon

    for o in node.outputs:
        assert (graph[o].op == 'get_tuple')
    original_node_outputs = list(node.outputs)

    delete_node(graph, node.name)

    builder = GraphBuilder(graph, node.name + '/', ParsedTFNode)
    x_center = builder.add_elementwise("Sub", [x, estimated_mean])
    scaling_factor = builder.add_elementwise(
        "Mul", [scale, builder.add_elementwise("Rsqrt", [estimated_variance])])
    x_scaled = builder.add_elementwise("Mul", [x_center, scaling_factor])
    x_shifted = builder.add_elementwise("Add", [x_scaled, offset])

    x_final = GraphBuilder(graph, '', ParsedTFNode).add_identity(x_shifted, node.name)

    outputs = [x_final]

    for o in original_node_outputs:
        replace_node(graph, o, outputs[graph[o].attr['index']])
        delete_node(graph, o)


def fusedbatchnorm_rewrite(nnssa):
    for i in list(nnssa.functions):
        graph = nnssa.functions[i].graph
        blockcells = [k for k, v in graph.items() if v.op == 'FusedBatchNorm']
        for b in blockcells:
            expand_fusedbatchnorm_cell(graph, graph[b])
