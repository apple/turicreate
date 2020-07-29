# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _


def delete_disconnected_nodes(gd):
    # delete all nodes with no inputs and outputs
    empty_nodes = []
    for k, v in gd.items():
        if (
            len(gd[k].inputs) == 0
            and len(gd[k].outputs) == 0
            and len(gd[k].control_inputs) == 0
            and len(gd[k].control_outputs) == 0
            and gd[k].op != "Placeholder"
        ):
            empty_nodes.append(k)

    for k in empty_nodes:
        del gd[k]
