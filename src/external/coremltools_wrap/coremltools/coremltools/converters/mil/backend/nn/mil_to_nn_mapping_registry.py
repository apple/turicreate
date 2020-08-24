#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

MIL_TO_NN_MAPPING_REGISTRY = {}

def register_mil_to_nn_mapping(func=None, override=False):
    def func_wrapper(_func):
        f_name = _func.__name__
        if not override and f_name in MIL_TO_NN_MAPPING_REGISTRY:
            raise ValueError("MIL to NN mapping for MIL op {} is already registered.".format(f_name))
        MIL_TO_NN_MAPPING_REGISTRY[f_name] = _func
        return _func
    
    if func is None:
        # decorator called without argument
        return func_wrapper
    return func_wrapper(func)