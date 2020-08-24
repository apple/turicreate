#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import logging


class PassRegistry:
    def __init__(self):
        # str -> func (func takes Program as input and
        # modifies in-place)
        self.passes = {}

    def __getitem__(self, pass_id):
        """
        pass_id (str): namespace::func_name (e.g., 'common::const_elimination')
        """
        if pass_id not in self.passes:
            raise KeyError("Pass {} not found".format(pass_id))
        return self.passes[pass_id]

    def add(self, namespace, pass_func):
        func_name = pass_func.__name__
        pass_id = namespace + "::" + func_name
        logging.debug("Registering pass {}".format(pass_id))
        if pass_id in self.passes:
            msg = "Pass {} already registered."
            raise KeyError(msg.format(pass_id))
        self.passes[pass_id] = pass_func


PASS_REGISTRY = PassRegistry()


def register_pass(namespace):
    """
    namespaces like {'common', 'nn_backend', <other-backends>,
    <other-frontends>}
    """

    def func_wrapper(pass_func):
        PASS_REGISTRY.add(namespace, pass_func)
        return pass_func

    return func_wrapper
