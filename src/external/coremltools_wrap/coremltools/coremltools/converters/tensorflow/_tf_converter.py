# Copyright (c) 2019, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause


import os.path
from ...models import MLModel

def convert(filename,
            inputs=None,
            outputs=None,
            image_input_names=None,
            is_bgr=False,
            red_bias=0.0,
            green_bias=0.0,
            blue_bias=0.0,
            gray_bias=0.0,
            image_scale=1.0,
            class_labels=None,
            predicted_feature_name=None,
            predicted_probabilities_output='',
            add_custom_layers=False,  # type: bool
            custom_conversion_functions={},  # type: Dict[Text, Any]
            custom_shape_functions={}, # type: Dict[Text, Any]
            **kwargs):
    
    use_cpu_only = kwargs.get('use_cpu_only')
    use_cpu_only = use_cpu_only if use_cpu_only is not None else False
    
    optional_inputs = kwargs.get('optional_inputs')
    optional_inputs = optional_inputs if optional_inputs is not None else []
    
    if not filename or not isinstance(filename, str) or not os.path.exists(filename) or not os.path.isfile(filename):
        raise ValueError('invalid input tf_model_path: {}.'.format(filename))

    if not filename.endswith('.pb'):
        raise ValueError('invalid input tf_model_path format, expecting TensorFlow frozen graph (.pb) model.')
    # convert from TensorFlow to SSA
    try:
        from ..nnssa.frontend.tensorflow import load as frontend_load
        ssa = frontend_load(filename, resume_on_errors=False, inputs=inputs, outputs=outputs, **kwargs)
    except ImportError as err:
        raise ImportError("Frontend converter not found! Error message:\n%s" % err)

    # convert from SSA to Core ML
    try:
        from ..nnssa.coreml.ssa_converter import ssa_convert
        mlmodelspec = ssa_convert(ssa,
                                  top_func='main',
                                  inputs=inputs,
                                  outputs=outputs,
                                  image_input_names=image_input_names,
                                  is_bgr=is_bgr,
                                  red_bias=red_bias,
                                  green_bias=green_bias,
                                  blue_bias=blue_bias,
                                  gray_bias=gray_bias,
                                  image_scale=image_scale,
                                  class_labels=class_labels,
                                  predicted_feature_name=predicted_feature_name,
                                  predicted_probabilities_output=predicted_probabilities_output,
                                  add_custom_layers=add_custom_layers,
                                  custom_conversion_functions=custom_conversion_functions,
                                  custom_shape_functions=custom_shape_functions,
                                  optional_inputs = optional_inputs
                                  )
    except ImportError as err:
        raise ImportError("Backend converter not found! Error message:\n%s" % err)


    return MLModel(mlmodelspec, useCPUOnly=use_cpu_only)
