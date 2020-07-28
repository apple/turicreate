#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

_TF_OPS_REGISTRY = {}


def register_tf_op(_func=None, tf_alias=None, override=False):
    """
    Registration routine for TensorFlow operators
    _func: (TF conversion function) [Default=None]
        TF conversion function to register

    tf_alias: (List of string) [Default=None]
        All other TF operators that should also be mapped to
        current conversion routine.
        e.g. Sort aliased with SortV1, SortV2
        All provided alias operators must not be registered previously.

    override: (Boolean) [Default=False]
        If True, overrides earlier registration i.e. specified
        operator and alias will start pointing to current conversion
        function.
        Otherwise, duplicate registration will error out.
    """

    def func_wrapper(func):
        f_name = func.__name__

        if not override and f_name in _TF_OPS_REGISTRY:
            raise ValueError("TF op {} already registered.".format(f_name))
        _TF_OPS_REGISTRY[f_name] = func
        # If tf_alias is provided, then all the functions mentioned as aliased
        # are mapped to current function
        if tf_alias is not None:
            for name in tf_alias:
                if not override and name in _TF_OPS_REGISTRY:
                    msg = "TF op alias {} already registered."
                    raise ValueError(msg.format(name))
                _TF_OPS_REGISTRY[name] = func
        return func

    if _func is None:
        # decorator called without argument
        return func_wrapper
    return func_wrapper(_func)
