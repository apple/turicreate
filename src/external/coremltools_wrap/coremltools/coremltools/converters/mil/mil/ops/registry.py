#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import logging
from ..builder import Builder
from collections import defaultdict


class SSAOpRegistry:
    # ops is 3 nested dicts:
    # namespace (str) -> {op_type (str) -> {op_class, doc_str}}
    ops = defaultdict(dict)
    custom_ops = {}

    @staticmethod
    def register_op(doc_str="", is_custom_op=False, namespace="core"):
        """
        Registration routine for MIL Program operators
        is_custom_op: (Boolean) [Default=False]
            If True, maps current operator to `custom_op`
            `custom_op` requires additional `bindings` which should be
            specified in operator.
            Current operator is registered as `SSARegistry.custom_ops`
            Otherwise, current operator is registered as usual operator,
            i.e. registered in `SSARegistry.ops'.
        """

        def class_wrapper(op_cls):
            op_type = op_cls.__name__
            # op_cls.__doc__ = doc_str  # TODO: rdar://58622145

            # Operation specific to custom op
            op_msg = "Custom op" if is_custom_op else "op"
            op_reg = (
                SSAOpRegistry.custom_ops
                if is_custom_op
                else SSAOpRegistry.ops[namespace]
            )

            logging.debug("Registering {} {}".format(op_msg, op_type))

            if op_type in op_reg:
                raise ValueError(
                    "SSA {} {} already registered.".format(op_msg, op_type)
                )

            if namespace != "core":
                # Check that op_type is prefixed with namespace
                if op_type[: len(namespace)] != namespace:
                    msg = (
                        "Op type {} registered under {} namespace must "
                        + "prefix with {}"
                    )
                    raise ValueError(msg.format(op_type, namespace, namespace))

            op_reg[op_type] = {"class": op_cls, "doc_str": doc_str}

            @classmethod
            def add_op(cls, **kwargs):
                return cls._add_op(op_cls, **kwargs)

            setattr(Builder, op_type, add_op)
            return op_cls

        return class_wrapper
