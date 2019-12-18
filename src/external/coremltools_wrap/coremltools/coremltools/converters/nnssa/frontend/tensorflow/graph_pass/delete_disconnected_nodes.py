# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _


def delete_disconnected_nodes(gd):
    # delete all nodes with no inputs and outputs
    empty_nodes = []
    for k, v in gd.items():
        if len(gd[k].inputs) == 0 and \
                len(gd[k].outputs) == 0 and  \
                len(gd[k].control_inputs) == 0 and \
                len(gd[k].control_outputs) == 0:
            empty_nodes.append(k)

    for k in empty_nodes:
        del gd[k]
