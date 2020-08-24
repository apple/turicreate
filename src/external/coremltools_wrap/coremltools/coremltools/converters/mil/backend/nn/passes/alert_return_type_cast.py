# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from coremltools.converters.mil.mil.passes.pass_registry import register_pass
from coremltools.converters.mil.mil import Var, types
import logging


@register_pass(namespace="nn_backend")
def alert_return_type_cast(prog):
    """
    prog: Program

    # NN always implicitly cast return types to fp32. Detect any return
    # types that are not builtin.fp32 and alert user of the implicit
    # casting. This pass must be at the end. Example:
    #
    # Given:
    #
    #    main(%x: (2, 3, fp32)) {
    #      block0() {
    #        %shape_0: (2,i32)* = const(val=[4, 7])
    #      } -> (%shape_0)
    #    }
    #
    # (Notice that %shape_0 is i32, not fp32)
    #
    # Result:
    #
    # The same program.
    #
    # Alert messages about %shape_0 being implicitly cast from i32 to fp32.
    #
    # Comment: This pass should do more proper casting as backend supports more types.
    """
    for f_name, f in prog.functions.items():
        for v in f.outputs:
            if isinstance(v, Var) and v.dtype != types.fp32:
                msg = (
                    "Output var {} of type {} in function {} is " + "cast to type fp32"
                )
                logging.warning(
                    msg.format(v.name, types.builtin_to_string(v.dtype), f_name)
                )
