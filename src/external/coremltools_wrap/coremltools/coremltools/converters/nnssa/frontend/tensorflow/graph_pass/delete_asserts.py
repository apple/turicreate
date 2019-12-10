# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ....commons.basic_graph_ops import delete_node

import sys

sys.setrecursionlimit(5000)  # increase recursion limit to support convert large models


def all_assert_leaves(gdict, nodename, memo):
    if nodename in memo:
        return memo[nodename]
    memo[nodename] = None
    if len(gdict[nodename].outputs) == 0:
        if gdict[nodename].op in ['Assert', 'CheckNumerics']:
            memo[nodename] = True
        else:
            memo[nodename] = False
    else:
        memo[nodename] = all(all_assert_leaves(gdict, o, memo) for o in gdict[nodename].outputs)

    return memo[nodename]


def delete_asserts(nnssa):
    # look for nodes which only end up at asserts
    delete_count = 0
    for f in nnssa.functions.values():
        memo = {}
        for n in f.graph:
            all_assert_leaves(f.graph, n, memo)
        for m in memo:
            if memo[m] is True:
                delete_count += 1
                delete_node(f.graph, m)
    print(str(delete_count) + " assert nodes deleted")
    return delete_count
