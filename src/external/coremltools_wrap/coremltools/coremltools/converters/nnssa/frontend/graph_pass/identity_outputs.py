# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ...nnssa import ParsedNode
from ...commons.basic_graph_ops import check_connections


def add_identity_outputs(ssa, main_function='main'):
    """
    This op changes the output nodes of the main function to always be
    an "Identity" op. Essentially:

    someop(name:Result)

    gets transformed to

    someop(name:Result_orig) --> Identity(name:Result)

    This simplies operation movement since the main function is kinda
    special in that it is the only one that is not actually a function
    with a single entry and exit point.
    """
    main = ssa.functions[main_function]
    vnames = list(main.graph.keys())[:]
    for v in vnames:
        node = main.graph[v]
        if (len(node.outputs) == 0 and node.op != 'Identity'
            and node.op != "NoOp" and node.op != 'set_global'):
            # current node is appended with _orig, new node takes current name
            name = ssa._find_free_name(node.name + '_orig')
            original_name = node.name
            # rename node
            node.name = name
            main.graph[node.name] = node
            # create new Identity node
            new_node = ParsedNode()
            new_node.op = 'Identity'
            new_node.name = original_name
            new_node.datatype = node.datatype
            new_node.value = node.value
            main.graph[new_node.name] = new_node

            # modify input, control_input nodes to point to the modified name
            for i in node.inputs:
                main.graph[i].outputs = [
                    o if o != original_name else name for o in main.graph[i].outputs
                ]
            for i in node.control_inputs:
                main.graph[i].control_outputs = [
                    o if o != original_name else name for o in main.graph[i].control_outputs
                ]
            # We maintain control outputs coming from the new node.
            # Since the new node takes the original name, we don't need to modify
            # the rest of the graph
            new_node.control_outputs = node.control_outputs
            node.control_outputs = []

            # connect up old node and new node
            node.outputs = [new_node.name]
            new_node.inputs = [node.name]

    check_connections(main.graph)
