# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import traceback

from .graph_pass import *


def common_pass(ssa, resume_on_errors=False, **kwargs):
    passes = [
        trace_constants, shift_get_global_to_set_global, type_inference_pass,
        common_symbolic_value_elimination, delete_unnecessary_constant_nodes, remove_identities,
        delete_unnecessary_constant_nodes, add_identity_outputs
    ]

    omit_symbolic_pass = kwargs.get("omit_symbolic_pass", False)
    if omit_symbolic_pass:
        passes = [i for i in passes if i is not common_symbolic_value_elimination]

    omit_shift_global_pass = kwargs.get("omit_shift_global_pass", True)
    if omit_shift_global_pass:
        passes = [i for i in passes if i is not shift_get_global_to_set_global]

    if resume_on_errors is False:
        for p in passes:
            p(ssa)
    else:
        for p in passes:
            try:
                p(ssa)
            except:
                tb = traceback.format_exc()
                print("Exception in pass " + str(p))
                print(tb)
                print("Ignoring and continuing to next pass")

    return ssa
