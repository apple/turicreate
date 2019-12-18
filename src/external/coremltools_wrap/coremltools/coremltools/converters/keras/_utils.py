# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause


def raise_error_unsupported_categorical_option(option_name, option_value, layer_type, layer_name):
    """
    Raise an error if an option is not supported.
    """
    raise RuntimeError("Unsupported option %s=%s in layer %s(%s)" % (option_name, option_value,
        layer_type, layer_name))

def raise_error_unsupported_option(option, layer_type, layer_name):
    """
    Raise an error if an option is not supported.
    """
    raise RuntimeError("Unsupported option =%s in layer %s(%s)" % (option,
        layer_type, layer_name))

def raise_error_unsupported_scenario(message, layer_type, layer_name):
    """
    Raise an error if an scenario is not supported.
    """
    raise RuntimeError("Unsupported scenario '%s' in layer %s(%s)" % (message, 
        layer_type, layer_name))
