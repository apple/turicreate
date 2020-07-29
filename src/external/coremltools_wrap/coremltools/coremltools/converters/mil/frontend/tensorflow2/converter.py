#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil.frontend.tensorflow.converter import TFConverter
from coremltools.converters.mil.frontend.tensorflow.basic_graph_ops import (
    simple_topsort,
)

from .ssa_passes.tf_passes import tensorflow_passes as tensorflow2_passes


class TF2Converter(TFConverter):
    def __init__(self, tf_ssa, inputs=None, outputs=None, **kwargs):
        TFConverter.__init__(self, tf_ssa, inputs, outputs, **kwargs)

        # Overwrite tensorflow_passes
        # TF 2.x uses different set of graph passes
        self.tensorflow_passes = tensorflow2_passes

    def _get_stack(self, tfssa, root="main"):
        """
        Overwrite TFConverter._get_stack() as TF2 generates different sub-graphs.
        """

        # We're trying to get a order of how to loop through the graphs.
        # This is NOT necessarily a DAG.
        dep = {x: [] for x in tfssa.functions}
        for fname in tfssa.functions:
            for node in tfssa.functions[fname].graph.values():
                func_x, func_y = None, None

                if node.op in {"StatelessIf", "If"}:
                    func_x = node.attr.get("then_branch")
                    func_y = node.attr.get("else_branch")
                elif node.op in {"StatelessWhile", "While"}:
                    func_x = node.attr.get("body")
                    func_y = node.attr.get("cond")

                if func_x and fname not in dep[func_x]:
                    dep[func_x].append(fname)
                if func_y and fname not in dep[func_y]:
                    dep[func_y].append(fname)

        assert len(dep[root]) == 0
        graph_stack = simple_topsort(dep)

        return graph_stack
