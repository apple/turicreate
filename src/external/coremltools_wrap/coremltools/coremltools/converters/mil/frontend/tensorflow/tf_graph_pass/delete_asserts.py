# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ..basic_graph_ops import delete_node
import logging
import sys

sys.setrecursionlimit(5000)  # increase recursion limit to support convert large models


def _all_assert_leaves(gdict, nodename, memo):
    """
    Does the given node lead to only assertions?

    Args:
        gdict (dict): The node's graph.
        nodename (str): The name of the node to test.
        memo (dict): Storage for memoization.
    """
    work = [nodename]
    while True:
        assert len(work) <= len(gdict)  # If true, this algorithm is broken
        node = gdict[work.pop()]

        # Entries in memo have one of the following values for a given node:
        #  None: the node is in the stack; this node is downstream.
        #  True: the node is an assertion or leads only to assertions.
        # False: the node does not lead only to assertions.
        if not isinstance(memo.get(node.name), bool):
            memo[node.name] = None
            outputs = node.outputs
            if len(outputs) == 0:
                # Leaf node: stack shrinks
                memo[node.name] = node.op in ("Assert", "CheckNumerics")
            else:
                outputs_to_process = [n for n in outputs if n not in memo]
                if len(outputs_to_process) == 0:
                    # Non-leaf node with fully processed outputs: stack shrinks
                    memo[node.name] = all(memo[n] for n in outputs)
                else:
                    # Non-leaf node with unprocess outputs: stack grows
                    work.append(node.name)
                    work.extend(outputs_to_process)
        if len(work) == 0:
            return memo[node.name]


def delete_asserts(tfssa):
    """
    Delete all nodes that lead only to assertions.
    """
    delete_count = 0
    for f in tfssa.functions.values():
        memo = {}
        for n in f.graph:
            _all_assert_leaves(f.graph, n, memo)
        for m in memo:
            if memo[m]:
                delete_count += 1
                delete_node(f.graph, m)
    logging.debug("%d assert nodes deleted", delete_count)
    return delete_count
