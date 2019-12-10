from ...commons.basic_graph_ops import topsort, delete_node, replace_source, connect_edge, check_connections
from ...nnssa import ParsedNode


def shift_get_global_to_set_global(nnssa):
    # For very set_global with only 1 get_global, shift all computations
    # which only depend on the result of get_global to be at the set_global
    # instead, even if they are across different functions
    delete_count = 0
    global_get_count = {}
    global_set_count = {}
    for fname in nnssa.functions:
        f = nnssa.functions[fname]
        for n in f.graph:
            if f.graph[n].op == 'get_global':
                v = f.graph[n].attr['variable']
                global_get_count[v] = global_get_count.get(v, []) + [(fname, n)]
            elif f.graph[n].op == 'set_global':
                v = f.graph[n].attr['variable']
                global_set_count[v] = global_set_count.get(v, []) + [(fname, n)]

    for v in global_set_count:
        varname = v
        if len(global_set_count[v]) == 1 and len(global_get_count.get(v, [])) == 1:
            set_function_name, set_node = global_set_count[v][0]
            get_function_name, get_node = global_get_count[v][0]
            get_function = nnssa.functions[get_function_name]
            set_function = nnssa.functions[set_function_name]

            get_fn_node_inputs = _trace_inputs(get_function.graph)
            nodes_to_transplant = [
                i for i, v in get_fn_node_inputs.items() if len(v) == 1 and v[0] == get_node
            ]
            nodes_to_transplant = _find_upstream_nodes(get_function.graph, nodes_to_transplant)
            nodes_to_transplant_set = set(nodes_to_transplant)
            if len(nodes_to_transplant_set) == 1:
                continue
            transplant_output_nodes = [
                i for i in nodes_to_transplant
                if len(set(get_function.graph[i].outputs) - nodes_to_transplant_set) > 0
            ]

            # create new nodes
            new_get_globals = [ParsedNode() for i in range(len(transplant_output_nodes))]
            new_set_globals = [ParsedNode() for i in range(len(transplant_output_nodes))]
            for i in range(len(new_get_globals)):
                new_get_globals[i].name = varname + '_get_global_shift_' + str(i)
                new_get_globals[i].op = 'get_global'
                new_get_globals[i].attr['variable'] = varname + '_get_global_shift_' + str(i)
                new_set_globals[i].name = varname + '_set_global_shift_' + str(i)
                new_set_globals[i].op = 'set_global'
                new_set_globals[i].attr['variable'] = varname + '_get_global_shift_' + str(i)
                get_function.graph[new_get_globals[i].name] = new_get_globals[i]
                set_function.graph[new_set_globals[i].name] = new_set_globals[i]
            for ctr, i in enumerate(transplant_output_nodes):
                onodes = list(get_function.graph[i].outputs[:])
                for o in onodes:
                    if o not in nodes_to_transplant_set:
                        replace_source(get_function.graph, i, o, new_get_globals[ctr].name)

            # transplant
            for d in nodes_to_transplant:
                n = get_function.graph[d]
                del get_function.graph[d]
                set_function.graph[d] = n

            for ctr, i in enumerate(transplant_output_nodes):
                connect_edge(set_function.graph, i, new_set_globals[ctr].name)

            connect_edge(set_function.graph, set_node, get_node)
            set_function.graph[set_node].op = 'Identity'
            set_function.graph[get_node].op = 'Identity'
            del set_function.graph[set_node].attr['variable']
            del set_function.graph[get_node].attr['variable']
            # update variables
            del nnssa.variables[varname]
            # unknown type and value
            for i in new_get_globals:
                nnssa.variables[i.name] = None
            check_connections(get_function.graph)
            check_connections(set_function.graph)


def _trace_inputs(graph):
    t = topsort(graph)
    inputs = {}
    for n in t:
        if graph[n].op == 'Const':
            inputs[n] = []
        elif len(graph[n].inputs) == 0:
            inputs[n] = [n]
        else:
            s = set()
            for i in graph[n].inputs:
                s |= set(inputs[i])
            inputs[n] = list(s)
    return inputs


def _find_upstream_nodes(graph, nodes):
    queue = nodes[:]
    visited = {}
    while len(queue) > 0:
        n = queue.pop()
        if n in visited:
            continue
        visited[n] = True
        queue = queue + graph[n].inputs
    return visited.keys()
