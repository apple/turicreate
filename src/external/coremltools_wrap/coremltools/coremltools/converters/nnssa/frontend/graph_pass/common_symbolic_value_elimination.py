import sympy as sm
import numpy as np
from ...commons.symbolic import *
from ...commons.basic_graph_ops import topsort, replace_source, disconnect_edge, connect_edge


def make_hashable(v):
    if is_symbolic(v):
        return str(v), True
    elif hasattr(v, '__iter__'):
        z = [make_hashable(h) for h in v]
        return tuple(i[0] for i in z), any(i[1] for i in z)
    else:
        return v, False


def compute_roots(gdict, topsort_order):
    # for each node, compute the list of initial inputs (inrank=0) i
    # that lead up to the node
    roots = {k: set() for k in gdict}
    for t in topsort_order:
        if len(gdict[t].inputs) == 0:
            # roots have themselves as roots
            roots[t].add(t)
            continue
        for i in gdict[t].inputs:
            roots[t] = roots[t].union(roots[i])
    return roots


def common_symbolic_value_elimination_impl(gdict):
    order = topsort(gdict)
    roots = compute_roots(gdict, order)
    values = {}
    for k in order:
        n = gdict[k]
        nodeval = n.attr.get('symbolic_value')
        try:
            if nodeval is None:
                continue
            elif isscalar(nodeval.val) and nodeval.val == -1:
                continue
            elif (not isscalar(nodeval.val)) and -1 in nodeval.val:
                continue
            elif isinstance(val, np.ndarray) and np.issctype(val.dtype) and val.size > 100:
                continue
        except:
            continue

        hashable_val, any_symbolic = make_hashable(nodeval.val)
        if any_symbolic:
            if hashable_val in values:
                # rewrite graph
                othernodes = values[hashable_val]
                for othernode in othernodes:
                    if len(roots[othernode].intersection(roots[n.name])) > 0:
                        outputs = list(n.outputs)
                        for outnode in outputs:
                            replace_source(gdict, n.name, outnode, othernode)
            else:
                values[hashable_val] = values.get(hashable_val, []) + [k]


def common_symbolic_value_elimination_impl2(gdict):
    order = topsort(gdict)
    roots = compute_roots(gdict, order)
    values = {}
    node_values = {}
    for k in order:
        n = gdict[k]
        nodeval = n.attr.get('symbolic_value')
        build_val = False
        try:
            if nodeval is None:
                build_val = True
            elif isscalar(nodeval.val) and nodeval.val == -1:
                build_val = True
            elif (not isscalar(nodeval.val)) and -1 in nodeval.val:
                build_val = True
        except:
            build_val = True

        if build_val == False:
            hashable_val, _ = make_hashable(nodeval.val)
        else:
            effective_val = [n.op, sorted(list(n.attr)), [node_values[v] for v in n.inputs]]
            hashable_val, _ = make_hashable(effective_val)

        hashable_val = hash(hashable_val)
        node_values[n.name] = hashable_val
        if hashable_val in values:
            # rewrite graph
            othernodes = values[hashable_val]
            for othernode in othernodes:
                if len(roots[othernode].intersection(roots[n.name])) > 0:
                    outputs = list(n.outputs)
                    for outnode in outputs:
                        replace_source(gdict, n.name, outnode, othernode)
        else:
            values[hashable_val] = values.get(hashable_val, []) + [k]


def common_symbolic_value_elimination(nnssa):
    for i in nnssa.functions:
        common_symbolic_value_elimination_impl(nnssa.functions[i].graph)
        #common_symbolic_value_elimination_impl2(nnssa.functions[i].graph)
